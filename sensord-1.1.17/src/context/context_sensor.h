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

#ifndef _CONTEXT_SENSOR_H_
#define _CONTEXT_SENSOR_H_

#include <sensor_common.h>

#include <physical_sensor.h>
#include <sensor_hal.h>

#include <vconf.h>
#include <glib.h>
#include <gio/gio.h>

class context_sensor : public physical_sensor {
public:
	context_sensor();
	virtual ~context_sensor();

	virtual bool init();
	virtual sensor_type_t get_type(void);

	static bool working(void *inst);

	virtual bool get_properties(sensor_properties_t &properties);
	virtual int send_sensorhub_data(const char* data, int data_len);
private:
	sensor_hal *m_sensor_hal;

	cmutex m_command_lock;

	bool process_event(void);

	GDBusConnection *m_dbus_conn;
	unsigned int m_lcd_on_noti_id;
	unsigned int m_lcd_off_noti_id;

	bool m_is_listen_display_state;
	bool m_display_state;
	bool m_charger_state;

	void initialize(void);
	void finalize(void);

	virtual bool on_start(void);
	virtual bool on_stop(void);

	int send_sensorhub_data_internal(const char* data, int data_len);

	bool is_reset_noti(const char* data, int data_len);
	bool is_pedo_noti(const char* data, int data_len);

	bool is_display_on(void);
	bool inform_display_state(bool display_on);
	bool is_charger_connected(void);
	bool inform_charger_state(bool connected);
	bool inform_current_time(void);

	static void display_signal_handler(GDBusConnection *conn, const gchar *name, const gchar *path, const gchar *interface,
			const gchar *sig, GVariant *param, gpointer user_data);
	bool start_listen_display_state();
	bool stop_listen_display_state();

	static void display_state_cb(keynode_t *node, void *user_data);
	static void charger_state_cb(keynode_t *node, void *user_data);
};

#endif
