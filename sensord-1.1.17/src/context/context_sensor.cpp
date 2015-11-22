/*
 * sensord
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <common.h>
#include <sf_common.h>

#include <context_sensor.h>
#include <sensor_plugin_loader.h>

#include <algorithm>

using std::equal;

#define SENSOR_NAME "CONTEXT_SENSOR"

context_sensor::context_sensor()
: m_sensor_hal(NULL)
, m_lcd_on_noti_id(0)
, m_lcd_off_noti_id(0)
, m_is_listen_display_state(false)
, m_display_state(false)
, m_charger_state(false)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(CONTEXT_EVENT_REPORT);

	physical_sensor::set_poller(context_sensor::working, this);
}

context_sensor::~context_sensor()
{
	if (m_is_listen_display_state)
		stop_listen_display_state();
	INFO("context_sensor is destroyed!\n");
}

bool context_sensor::init()
{
	m_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(CONTEXT_SENSOR);

	if (!m_sensor_hal) {
		ERR("cannot load sensor_hal[%s]", sensor_base::get_name());
		return false;
	}

	start_listen_display_state();
	m_is_listen_display_state = true;

	set_privilege(SENSOR_PRIVILEGE_INTERNAL);

	INFO("%s is created!\n", sensor_base::get_name());

	return true;
}

sensor_type_t context_sensor::get_type(void)
{
	return CONTEXT_SENSOR;
}

bool context_sensor::working(void *inst)
{
	context_sensor *sensor = (context_sensor*)inst;
	return sensor->process_event();;
}

bool context_sensor::process_event(void)
{
	if (m_sensor_hal->is_data_ready(true) == false)
		return true;

	sensorhub_event_t sensorhub_event;
	m_sensor_hal->get_sensor_data(sensorhub_event.data);

	{
		AUTOLOCK(m_command_lock);

		if (is_reset_noti(sensorhub_event.data.hub_data, sensorhub_event.data.hub_data_size)) {
			ERR("Sensorhub MCU is resetted!");
		}

		if (is_pedo_noti(sensorhub_event.data.hub_data, sensorhub_event.data.hub_data_size) && !is_display_on()) {
			INFO("Received Pedo data in LCD Off!");
			inform_current_time();
		}
	}

	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(CONTEXT_EVENT_REPORT)) {
		sensorhub_event.data.timestamp = get_timestamp();
		sensorhub_event.sensor_id = get_id();
		sensorhub_event.event_type = CONTEXT_EVENT_REPORT;
		push(sensorhub_event);
	}

	return true;
}

bool context_sensor::on_start(void)
{
	if (!m_sensor_hal->enable()) {
		ERR("m_sensor_hal start fail\n");
		return false;
	}

	initialize();

	return start_poll();
}

bool context_sensor::on_stop(void)
{
	finalize();

	if (!m_sensor_hal->disable()) {
		ERR("m_sensor_hal stop fail\n");
		return false;
	}

	return stop_poll();
}

bool context_sensor::get_properties(sensor_properties_t &properties)
{
	return m_sensor_hal->get_properties(properties);
}

int context_sensor::send_sensorhub_data(const char* data, int data_len)
{
	int ret;

	AUTOLOCK(m_command_lock);

	ret = send_sensorhub_data_internal(data, data_len);
	if ((ret < 0) && (ret != -EBUSY))
		return ret;

	if (ret == -EBUSY)
		WARN("Command is sent during sensorhub firmware update");

	return 0;
}

void context_sensor::initialize(void)
{
	m_display_state = is_display_on();
	m_charger_state = is_charger_connected();

	DBG("Initial inform sensorhub of display: %s, charger: %s", m_display_state? "on" : "off",
		m_charger_state? "connected" : "disconnected");

	inform_display_state(m_display_state);
	inform_charger_state(m_charger_state);

	if (vconf_notify_key_changed(VCONFKEY_SYSMAN_CHARGER_STATUS, charger_state_cb, this) != 0)
		ERR("Fail to set notify callback for %s", VCONFKEY_SYSMAN_CHARGER_STATUS);
}

void context_sensor::finalize(void)
{
	if (vconf_ignore_key_changed(VCONFKEY_SYSMAN_CHARGER_STATUS, charger_state_cb) < 0)
		ERR("Can't unset callback for %s", VCONFKEY_SYSMAN_CHARGER_STATUS);
}

int context_sensor::send_sensorhub_data_internal(const char* data, int data_len)
{
	int state;

	state = m_sensor_hal->send_sensorhub_data(data, data_len);

	if (state < 0) {
		ERR("Failed to send_sensorhub_data");
		return state;
	}

	return 0;
}

bool context_sensor::is_reset_noti(const char* data, int data_len)
{
	static const char reset_noti[] = {2, 1, (char)-43};

	if (data_len != sizeof(reset_noti))
		return false;

	return equal(reset_noti, reset_noti + sizeof(reset_noti), data);
}

bool context_sensor::is_pedo_noti(const char* data, int data_len)
{
	static const char pedo_noti[] = {1, 1, 3};
	static const char pedo_noti_ext[] = {1, 3, 3, 1};

	return (equal(pedo_noti, pedo_noti + sizeof(pedo_noti), data) || equal(pedo_noti_ext, pedo_noti_ext + sizeof(pedo_noti_ext), data));
}

void context_sensor::display_signal_handler(GDBusConnection *conn, const gchar *name, const gchar *path, const gchar *interface,
		const gchar *sig, GVariant *param, gpointer user_data)
{
	context_sensor* cp = static_cast<context_sensor*>(user_data);

	if (!strcmp(sig, "LCDOn")) {
		INFO("Display state: LCD ON");
		cp->inform_display_state(true);
	} else if (!strcmp(sig, "LCDOff")) {
		INFO("Display state: LCD Off");
		cp->inform_display_state(false);
		cp->inform_current_time();
	}
}

bool context_sensor::start_listen_display_state()
{
	GError *error = NULL;

	g_type_init();
	m_dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (!m_dbus_conn) {
		ERR("Error creating dbus connection: %s\n", error->message);
		g_error_free (error);
		return false;
	}

	INFO("G-DBUS connected");

	m_lcd_on_noti_id = g_dbus_connection_signal_subscribe(m_dbus_conn,
		"org.tizen.system.deviced", /* sender */
		"org.tizen.system.deviced.display", /* Interface */
		"LCDOn", /* Member */
		"/Org/Tizen/System/DeviceD/Display", /* Object path */
		NULL, /* arg0 */
		G_DBUS_SIGNAL_FLAGS_NONE,
		display_signal_handler,
		this, NULL);

	INFO("noti_id for LCDOn [%d]", m_lcd_on_noti_id);

	m_lcd_off_noti_id = g_dbus_connection_signal_subscribe(m_dbus_conn,
		"org.tizen.system.deviced", /* sender */
		"org.tizen.system.deviced.display", /* Interface */
		"LCDOff", /* Member */
		"/Org/Tizen/System/DeviceD/Display", /* Object path */
		NULL, /* arg0 */
		G_DBUS_SIGNAL_FLAGS_NONE,
		display_signal_handler,
		this, NULL);

	INFO("noti_id for LCDOff [%d]", m_lcd_off_noti_id);

	return true;
}

bool context_sensor::stop_listen_display_state()
{
	g_dbus_connection_signal_unsubscribe (m_dbus_conn, m_lcd_on_noti_id);
	g_dbus_connection_signal_unsubscribe (m_dbus_conn, m_lcd_off_noti_id);

	return true;
}

void context_sensor::charger_state_cb(keynode_t *node, void *user_data)
{
	bool charger_state;
	context_sensor* cp = static_cast<context_sensor*>(user_data);

	charger_state = cp->is_charger_connected();

	if (cp->m_charger_state != charger_state) {
		cp->m_charger_state = charger_state;
		cp->inform_charger_state(charger_state);
	}
}

bool context_sensor::is_display_on(void)
{
	int lcd_state;

	if (vconf_get_int(VCONFKEY_PM_STATE, &lcd_state) != 0) {
		ERR("Can't get the value of VCONFKEY_PM_STATE");
		return true;
	}

	if (lcd_state == VCONFKEY_PM_STATE_LCDOFF)
		return false;

	return true;
}


bool context_sensor::inform_display_state(bool display_on)
{
	static const char display_on_noti_cmd[] = {(char)-76, 13, (char)-47, 0};
	static const char display_off_noti_cmd[] = {(char)-76, 13, (char)-46, 0};

	const char* cmd;
	int cmd_len;

	if (display_on) {
		cmd = display_on_noti_cmd;
		cmd_len = sizeof(display_on_noti_cmd);
		DBG("Inform sensorhub of display on");
	} else {
		cmd = display_off_noti_cmd;
		cmd_len = sizeof(display_off_noti_cmd);
		DBG("Inform sensorhub of display off");
	}

	return (send_sensorhub_data_internal(cmd, cmd_len) >= 0);
}

bool context_sensor::is_charger_connected(void)
{
	int charger_state;

	if (vconf_get_int(VCONFKEY_SYSMAN_CHARGER_STATUS, &charger_state) != 0) {
		ERR("Can't get the value of VCONFKEY_SYSMAN_CHARGER_STATUS");
		return false;
	}

	return charger_state;
}

bool context_sensor::inform_charger_state(bool connected)
{
	static const char charger_connected_noti_cmd[] = {(char)-76, 13, (char)-42, 0};
	static const char charger_disconnected_noti_cmd[] = {(char)-76, 13,(char)-41, 0};

	const char* cmd;
	int cmd_len;

	if (connected) {
		cmd = charger_connected_noti_cmd;
		cmd_len = sizeof(charger_connected_noti_cmd);
		DBG("Inform sensorhub of charger connected");
	} else {
		cmd = charger_disconnected_noti_cmd;
		cmd_len = sizeof(charger_disconnected_noti_cmd);
		DBG("Inform sensorhub of charger disconnected");
	}

	return (send_sensorhub_data_internal(cmd, cmd_len) >= 0);
}


bool context_sensor::inform_current_time(void)
{
	char current_time_noti_cmd[] = {(char)-63, 14, 0, 0, 0};

	struct tm gm_time;
	time_t t;

	time(&t);
	gmtime_r(&t, &gm_time);

	current_time_noti_cmd[2] = gm_time.tm_hour;
	current_time_noti_cmd[3] = gm_time.tm_min;
	current_time_noti_cmd[4] = gm_time.tm_sec;

	return (send_sensorhub_data_internal(current_time_noti_cmd, sizeof(current_time_noti_cmd)) >= 0);
}

extern "C" sensor_module* create(void)
{
	context_sensor *sensor;

	try {
		sensor = new(std::nothrow) context_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
