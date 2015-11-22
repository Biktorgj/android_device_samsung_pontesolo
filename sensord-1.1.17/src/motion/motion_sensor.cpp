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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <dlfcn.h>

#include <common.h>
#include <sf_common.h>

#include <virtual_sensor.h>
#include <motion_sensor.h>
#include <sensor_plugin_loader.h>
#include <algorithm>

using std::unordered_map;
using std::string;

#define DEGREE_TO_RADIAN	(0.0174532925)
#define MS_TO_US	1000
#define PROXI_FAR	100
#define SENSOR_NAME "MOTION_SENSOR"
#define MOTION_ENGINE_LIB "/usr/lib/sensord/libmotion-engine.so"
#define SMART_RELAY_INIT	3
#define SMART_RELAY_FINI	4

motion_sensor::motion_sensor()
: m_accel_sensor(NULL)
, m_gyro_sensor(NULL)
, m_proxi_sensor(NULL)
, m_enabled(false)
, m_accel_sensor_started(false)
, m_gyro_sensor_started(false)
, m_proxi_sensor_started(false)
, m_tilt_scale_factor(0.0f)
, m_panning_scale_factor(0.0f)
, m_panning_browse_scale_factor(0.0f)
, m_smart_relay_running(false)
, m_handle(NULL)
{
	m_name = string(SENSOR_NAME);
}

void motion_sensor::get_motion_setting(void)
{
	int vconf_val = -1;

	if (vconf_get_bool(VCONFKEY_SETAPPL_MOTION_ACTIVATION, &vconf_val) == 0) {
		m_motion_activation_state = (vconf_val == 1) ? true : false;
		INFO("m_motion_activation_state value = [%d]", vconf_val);
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_USE_PICK_UP_CALL, &vconf_val) == 0) {
		set_motion_enable_state(MOTION_ENGINE_EVENT_DIRECT_CALL, (vconf_val == 1));
		INFO("m_motion_direct_call_state value = [%d]", vconf_val);
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_USE_DOUBLE_TAP, &vconf_val) == 0) {
		set_motion_enable_state(MOTION_ENGINE_EVENT_DOUBLETAP, (vconf_val == 1));
		INFO("m_motion_doubletap_state value = [%d]", vconf_val);
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_USE_PANNING, &vconf_val) == 0) {
		set_motion_enable_state(MOTION_ENGINE_EVENT_PANNING, (vconf_val == 1));
		INFO("m_motion_panning_state value = [%d]", vconf_val);
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_USE_TILT, &vconf_val) == 0) {
		set_motion_enable_state(MOTION_ENGINE_EVENT_TILT, (vconf_val == 1));
		INFO("m_motion_tilt_state value = [%d]", vconf_val);
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_USE_PANNING_BROWSER, &vconf_val) == 0) {
		set_motion_enable_state(MOTION_ENGINE_EVENT_PANNING_BROWSE, (vconf_val == 1));
		INFO("m_motion_panning_browser_state value = [%d]", vconf_val);
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_USE_SHAKE, &vconf_val) == 0) {
		set_motion_enable_state(MOTION_ENGINE_EVENT_SHAKE, (vconf_val == 1));
		INFO("m_motion_shake_state value = [%d]", vconf_val);
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_USE_TURN_OVER, &vconf_val) == 0) {
		set_motion_enable_state(MOTION_ENGINE_EVENT_TOP_TO_BOTTOM, (vconf_val == 1));
		INFO("m_motion_top_to_bottom_state value = [%d]", vconf_val);
	}

	if (vconf_get_int(VCONFKEY_SETAPPL_TILT_SENSITIVITY, &vconf_val) == 0) {
		m_tilt_scale_factor = get_scale_factor(vconf_val);
		INFO("tilt sensitivity = [%d], m_tilt_scale_factor = [%f]", vconf_val, m_tilt_scale_factor);
	}

	if (vconf_get_int(VCONFKEY_SETAPPL_PANNING_SENSITIVITY, &vconf_val) == 0) {
		m_panning_scale_factor = get_scale_factor(vconf_val);
		INFO("panning sensitivity = [%d], m_panning_scale_factor = [%f]", vconf_val, m_panning_scale_factor);
	}

	if (vconf_get_int(VCONFKEY_SETAPPL_PANNING_BROWSER_SENSITIVITY, &vconf_val) == 0) {
		m_panning_browse_scale_factor = get_scale_factor(vconf_val);
		INFO("panning browse sensitivity = [%d], m_panning_browse_scale_factor = [%f]", vconf_val, m_panning_browse_scale_factor);
	}

}

void motion_sensor::motion_enable_cb(keynode_t *node, void *data)
{
	bool state;
	motion_sensor* sensor = static_cast<motion_sensor*>(data);

	state = (vconf_keynode_get_bool(node) == 1) ? true : false;

	INFO("vconf_notify_key_changed node name = [%s] value = [%d]", vconf_keynode_get_name(node), (int)state);

	cmutex &mutex = sensor->m_mutex;
	AUTOLOCK(mutex);

	if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_USE_PICK_UP_CALL)) {
		sensor->set_motion_enable_state(MOTION_ENGINE_EVENT_DIRECT_CALL, state);
	} else if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_USE_DOUBLE_TAP)) {
		sensor->set_motion_enable_state(MOTION_ENGINE_EVENT_DOUBLETAP, state);
	} else if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_USE_TILT)) {
		sensor->set_motion_enable_state(MOTION_ENGINE_EVENT_TILT, state);
	} else if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_USE_PANNING)) {
		sensor->set_motion_enable_state(MOTION_ENGINE_EVENT_PANNING, state);
	} else if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_USE_PANNING_BROWSER)) {
		sensor->set_motion_enable_state(MOTION_ENGINE_EVENT_PANNING_BROWSE, state);
	} else if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_USE_SHAKE)) {
		sensor->set_motion_enable_state(MOTION_ENGINE_EVENT_SHAKE, state);
	} else if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_USE_TURN_OVER)) {
		sensor->set_motion_enable_state(MOTION_ENGINE_EVENT_TOP_TO_BOTTOM, state);
	} else if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_MOTION_ACTIVATION)) {
		sensor->m_motion_activation_state = state;
	} else {
		return;
	}

	sensor->operate_sensor();
}

void motion_sensor::sensitivity_change_cb(keynode_t *node, void *data)
{
	int sensitivity;
	motion_sensor* sensor = static_cast<motion_sensor*>(data);

	sensitivity = vconf_keynode_get_int(node);

	INFO("vconf_notify_key_changed node name = [%s] value = [%d]", vconf_keynode_get_name(node), sensitivity);

	if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_TILT_SENSITIVITY)) {
		sensor->m_tilt_scale_factor = get_scale_factor(sensitivity);
		INFO("tilt sensitivity = [%d], m_tilt_scale_factor = [%f]",
			sensitivity, sensor->m_tilt_scale_factor);
	} else if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_PANNING_SENSITIVITY)) {
		sensor->m_panning_scale_factor = get_scale_factor(sensitivity);
		INFO("panning sensitivity = [%d], m_panning_scale_factor = [%f]",
			sensitivity,sensor-> m_panning_scale_factor);
	} else if (!strcmp(vconf_keynode_get_name(node), VCONFKEY_SETAPPL_PANNING_BROWSER_SENSITIVITY)) {
		sensor->m_panning_browse_scale_factor = get_scale_factor(sensitivity);
		INFO("panning_browse sensitivity = [%d], m_panning_browse_scale_factor = [%f]",
			sensitivity, sensor->m_panning_browse_scale_factor);
	}
}

motion_sensor::~motion_sensor()
{
	if (m_handle)
		dlclose(m_handle);

	INFO("motion_sensor is destroyed!\n");
}

bool motion_sensor::register_sensor_event(sensor_type_t sensor_type, bool is_register)
{
	unsigned int event_type;
	sensor_base *sensor;

	if (sensor_type == ACCELEROMETER_SENSOR) {
		sensor = m_accel_sensor;
		event_type = ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME;
	} else if (sensor_type == GYROSCOPE_SENSOR) {
		sensor = m_gyro_sensor;
		event_type = GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME;
	} else if (sensor_type == PROXIMITY_SENSOR) {
		sensor = m_proxi_sensor;
		event_type = PROXIMITY_EVENT_CHANGE_STATE;
	} else {
		ERR("Not supported sensor_type: 0x%x", sensor_type);
		return false;
	}

	if (is_register) {
		sensor->add_client(event_type);
		sensor->add_interval((int)this, SAMPLING_TIME, true);
		sensor->start();
	} else {
		sensor->delete_client(event_type);
		sensor->delete_interval((int)this, true);
		sensor->stop();
	}

	return true;
}

bool motion_sensor::operate_sensor(void)
{
	bool accel_turn_on = is_sensor_used(ACC_USED);
	bool gyro_turn_on = is_sensor_used(GYRO_USED);
	bool proxi_turn_on = is_sensor_used(PROXI_USED);

	if (accel_turn_on != m_accel_sensor_started) {
		register_sensor_event(ACCELEROMETER_SENSOR, accel_turn_on);
		m_accel_sensor_started = accel_turn_on;
		INFO("accel sensor %s", accel_turn_on ? "started" : "stopped");
	}

	if (gyro_turn_on != m_gyro_sensor_started) {
		register_sensor_event(GYROSCOPE_SENSOR, gyro_turn_on);
		m_gyro_sensor_started = gyro_turn_on;
		INFO("gyro sensor %s", gyro_turn_on ? "started" : "stopped");
	}

	if (proxi_turn_on != m_proxi_sensor_started) {
		register_sensor_event(PROXIMITY_SENSOR, proxi_turn_on);
		m_proxi_sensor_started = proxi_turn_on;
		INFO("proxi sensor %s", proxi_turn_on ? "started" : "stopped");
	}

	return true;
}

bool motion_sensor::add_client(unsigned int event_type)
{
	AUTOLOCK(m_mutex);

	if (!sensor_base::add_client(event_type))
		return false;

	switch (event_type) {
	case MOTION_ENGINE_EVENT_NO_MOVE:
		ME_init();
		break;
	case MOTION_ENGINE_EVENT_SMART_RELAY:
		{	/* the scope for the lock */
			AUTOLOCK(m_smart_relay_mutex);
			if (!m_smart_relay_running) {
				m_smart_relay_running = true;
				ME_reset_angle(SMART_RELAY_INIT);
				INFO("Smart relay is initialized");
			}
		}
		break;
	default:
		break;

	}

	if (m_enabled)
		operate_sensor();

	return true;
}

bool motion_sensor::delete_client(unsigned int event_type)
{
	AUTOLOCK(m_mutex);

	if (!sensor_base::delete_client(event_type))
		return false;

	switch (event_type) {
	case MOTION_ENGINE_EVENT_SMART_RELAY:
		if (get_client_cnt(MOTION_ENGINE_EVENT_SMART_RELAY) == 0) {
			AUTOLOCK(m_smart_relay_mutex);
			if (m_smart_relay_running) {
				m_smart_relay_running = false;
				ME_reset_angle(SMART_RELAY_FINI);
				INFO("Smart relay is finalized");
			}
		}
		break;
	default:
		break;
	}

	if (m_enabled)
		operate_sensor();

	return true;
}

bool motion_sensor::turn_off_sensor(void)
{
	if (m_accel_sensor_started) {
		INFO("accel sensor stopped");
		register_sensor_event(ACCELEROMETER_SENSOR, false);
		m_accel_sensor_started = false;
	}

	if (m_gyro_sensor_started) {
		INFO("gyro sensor stopped");
		register_sensor_event(GYROSCOPE_SENSOR, false);
		m_gyro_sensor_started = false;
	}

	if (m_proxi_sensor_started) {
		INFO("proxi sensor stopped");
		register_sensor_event(PROXIMITY_SENSOR, false);
		m_proxi_sensor_started = false;
	}

	return true;

}

float motion_sensor::get_scale_factor(int sensitivity)
{
	const int SENSITIVITY_MAX = 7;
	const int SENSITIVITY_DEF = 3;

	return (1.0f + (float)(sensitivity - SENSITIVITY_DEF) / (float)SENSITIVITY_MAX);
}

bool motion_sensor::load_lib(void)
{
	m_handle = dlopen(MOTION_ENGINE_LIB, RTLD_NOW);

	if (!m_handle) {
		ERR("dlopen(%s) error, cause: %s", MOTION_ENGINE_LIB, dlerror());
		return false;
	}

	ME_init = (ME_init_t) dlsym(m_handle, "ME_init");
	ME_fini = (ME_fini_t) dlsym(m_handle, "ME_fini");
	ME_get_gyro = (ME_get_gyro_t) dlsym(m_handle, "ME_get_gyro");
	ME_get_result = (ME_get_result_t) dlsym(m_handle, "ME_get_result");
	ME_get_panning_dx = (ME_get_panning_dx_t) dlsym(m_handle, "ME_get_panning_dx");
	ME_get_panning_dy = (ME_get_panning_dy_t) dlsym(m_handle, "ME_get_panning_dy");
	ME_get_panning_dz = (ME_get_panning_dz_t) dlsym(m_handle, "ME_get_panning_dz");
	ME_clear_result = (ME_clear_result_t) dlsym(m_handle, "ME_clear_result");
	ME_reset_angle = (ME_reset_angle_t) dlsym(m_handle, "ME_reset_angle");

	if (!ME_init || !ME_fini || !ME_get_gyro || !ME_get_result ||
		!ME_get_panning_dx || !ME_get_panning_dy || !ME_get_panning_dz ||
		!ME_clear_result || !ME_reset_angle) {
		ERR("Fail to load motion engine symbols");
		dlclose(m_handle);
		m_handle = NULL;
		return false;
	}

	return true;
}

bool motion_sensor::init()
{
	m_accel_sensor = sensor_plugin_loader::get_instance().get_sensor(ACCELEROMETER_SENSOR);
	m_gyro_sensor = sensor_plugin_loader::get_instance().get_sensor(GYROSCOPE_SENSOR);
	m_proxi_sensor = sensor_plugin_loader::get_instance().get_sensor(PROXIMITY_SENSOR);

	if (!load_lib())
		return false;

	reset();

	if (m_accel_sensor) {
		m_motion_info[MOTION_ENGINE_EVENT_SNAP] = {false, true, ACC_USED};
		m_motion_info[MOTION_ENGINE_EVENT_SHAKE] = {false, false, ACC_USED};
		m_motion_info[MOTION_ENGINE_EVENT_DOUBLETAP] = {false, false, ACC_USED};
		m_motion_info[MOTION_ENGINE_EVENT_TOP_TO_BOTTOM] = {false, false, ACC_USED};
		m_motion_info[MOTION_ENGINE_EVENT_NO_MOVE] = {false, true, ACC_USED};
		m_motion_info[MOTION_ENGINE_EVENT_SHAKE_ALWAYS_ON] = {true, true, ACC_USED};
	}

	if (m_gyro_sensor) {
		m_motion_info[MOTION_ENGINE_EVENT_TILT] = {false, false, GYRO_USED};
		m_motion_info[MOTION_ENGINE_EVENT_PANNING_BROWSE] = {false, false, GYRO_USED};
		m_motion_info[MOTION_ENGINE_EVENT_PANNING] = {false, false, GYRO_USED};
	}

	if (m_accel_sensor && m_gyro_sensor)
		m_motion_info[MOTION_ENGINE_EVENT_TILT_TO_UNLOCK] = {true, true, ACC_USED | GYRO_USED};

	if (m_accel_sensor && m_gyro_sensor && m_proxi_sensor) {
		m_motion_info[MOTION_ENGINE_EVENT_DIRECT_CALL] = {false, false, ACC_USED | GYRO_USED | PROXI_USED};
		m_motion_info[MOTION_ENGINE_EVENT_SMART_RELAY] = {true, true, ACC_USED | GYRO_USED | PROXI_USED};
	}

	auto it_motion_event_info = m_motion_info.begin();

	while (it_motion_event_info != m_motion_info.end()) {
		register_supported_event(it_motion_event_info->first);
		++it_motion_event_info;
	}

	get_motion_setting();

	vconf_notify_key_changed(VCONFKEY_SETAPPL_USE_PICK_UP_CALL, motion_enable_cb, this);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_USE_PICK_UP, motion_enable_cb, this);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_USE_DOUBLE_TAP, motion_enable_cb, this);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_USE_PANNING, motion_enable_cb, this);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_USE_TILT, motion_enable_cb, this);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_USE_PANNING_BROWSER, motion_enable_cb, this);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_USE_SHAKE, motion_enable_cb, this);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_USE_TURN_OVER, motion_enable_cb, this);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_MOTION_ACTIVATION, motion_enable_cb, this);

	vconf_notify_key_changed(VCONFKEY_SETAPPL_TILT_SENSITIVITY, sensitivity_change_cb, this);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_PANNING_SENSITIVITY, sensitivity_change_cb, this);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_PANNING_BROWSER_SENSITIVITY, sensitivity_change_cb, this);

	set_privilege(SENSOR_PRIVILEGE_INTERNAL);

	INFO("%s is created!\n", sensor_base::get_name());
	return true;
}

sensor_type_t motion_sensor::get_type(void)
{
	return MOTION_SENSOR;
}

void motion_sensor::reset(void)
{
	m_g_x = m_g_y = m_g_z = 0;
	m_proxi = PROXI_FAR;

	m_prev_acc_time = 0;
	m_prev_gyro_time = 0;
}

bool motion_sensor::on_start(void)
{
	int ret = ME_init();

	if (ret != 1)
		ERR("Fail to initialize motion engine, ret = %d", ret);

	reset();
	operate_sensor();
	activate();

	m_enabled = true;
	return true;
}

bool motion_sensor::on_stop(void)
{
	ME_fini();
	turn_off_sensor();
	deactivate();

	m_enabled = false;
	return true;
}

void motion_sensor::put_motion_event(unsigned int event_type, int value, vector<sensor_event_t> &outs)
{
	sensor_event_t event;

	event.sensor_id = get_id();
	event.data.timestamp = get_timestamp();
	event.event_type = event_type;
	event.data.value_count = 1;
	event.data.values[0] = value;
	outs.push_back(event);

}

void motion_sensor::put_tilt_event(unsigned int event_type, float scale_factor, vector<sensor_event_t> &outs)
{
	sensor_event_t event;
	int data[3];

	data[0] = ME_get_panning_dx();
	data[1] = ME_get_panning_dy();
	data[2] = ME_get_panning_dz();

	DBG("Panning DX = %d, DY = %d, DZ = %d", data[0], data[1], data[2]);

	event.sensor_id = get_id();
	event.data.timestamp = get_timestamp();
	event.data.value_count = 3;
	event.data.values[0] = (data[0] * scale_factor);
	event.data.values[1] = (data[1] * scale_factor);
	event.data.values[2] = (data[2] * scale_factor);

	if (event.data.values[0] || event.data.values[1] || event.data.values[2]) {
		event.event_type = event_type;
		outs.push_back(event);
	}

}

void motion_sensor::get_motion(float a_x, float a_y, float a_z, float g_x, float g_y, float g_z, int proxi, vector<sensor_event_t> &outs)
{
	int new_snap_event = 0;
	int new_shake_event = 0;
	int new_doubletap_event = 0;
	int new_top_to_bottom_event = 0;
	int new_direct_call_event = 0;
	int new_smart_relay_event = 0;
	int new_tilt_to_unlock_event = 0;
	int new_no_move_event = 0;

	unsigned int sensor_usage = 0;

	MOTION_SCENARIO_RESULT *motion_result = NULL;

	sensor_usage = ((m_gyro_sensor_started ? MOTION_USE_GYRO : 0) | (m_accel_sensor_started ? MOTION_USE_ACC : 0));

	DBG("a_x = %f, a_y = %f, a_z = %f, g_x = %f, g_y = %f, g_z = %f, proxi = %d, sensor_usage = 0x%x",
		a_x, a_y, a_z, g_x, g_y, g_z, proxi, sensor_usage);

	motion_result = ME_get_result(a_x, a_y, a_z, g_x, g_y, g_z, proxi,
		0,0, sensor_usage, SAMPLING_TIME, 0);

	switch (motion_result->motionEvent) {
	case MOTION_EVENT_TWO_TAPPING:
		new_doubletap_event = MOTION_ENGINE_DOUBLTAP_DETECTION;
		ME_clear_result(MOTION_SCENARIO_ONLY_TWO_TAPPING);
		INFO("DOUBLE_TAP motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_FLIP_TOP_TO_BOTTOM:
		new_top_to_bottom_event = MOTION_ENGINE_TOP_TO_BOTTOM_DETECTION;
		ME_clear_result(MOTION_SCENARIO_ON_CALL_FLIP_TOP_TO_BOTTOM);
		INFO("TOP_TO_BOTTOM motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_SNAP_X_POSITIVE:
		new_snap_event = MOTION_ENGINE_POSITIVE_SNAP_X;
		ME_clear_result(MOTION_SCENARIO_ONLY_SNAP);
		DBG("SNAP motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_SNAP_X_NEGATIVE:
		new_snap_event = MOTION_ENGINE_NEGATIVE_SNAP_X;
		ME_clear_result(MOTION_SCENARIO_ONLY_SNAP);
		DBG("SNAP motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_SNAP_Y_POSITIVE:
		new_snap_event = MOTION_ENGINE_POSITIVE_SNAP_Y;
		ME_clear_result(MOTION_SCENARIO_ONLY_SNAP);
		DBG("SNAP motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_SNAP_Y_NEGATIVE:
		new_snap_event = MOTION_ENGINE_NEGATIVE_SNAP_Y;
		ME_clear_result(MOTION_SCENARIO_ONLY_SNAP);
		DBG("SNAP motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_SNAP_Z_POSITIVE:
		new_snap_event = MOTION_ENGINE_POSITIVE_SNAP_Z;
		ME_clear_result(MOTION_SCENARIO_ONLY_SNAP);
		DBG("SNAP motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_SNAP_Z_NEGATIVE:
		new_snap_event = MOTION_ENGINE_NEGATIVE_SNAP_Z;
		ME_clear_result(MOTION_SCENARIO_ONLY_SNAP);
		DBG("SNAP motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_SHAKE_START:
		new_shake_event = MOTION_ENGINE_SHAKE_DETECTION;
		ME_clear_result(MOTION_SCENARIO_ONLY_SHAKE);
		INFO("SHAKE motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_SHAKE:
		new_shake_event = MOTION_ENGINE_SHAKE_CONTINUING;
		ME_clear_result(MOTION_SCENARIO_ONLY_SHAKE);
		INFO("SHAKE motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_SHAKE_STOP:
		new_shake_event = MOTION_ENGINE_SHAKE_FINISH;
		ME_clear_result(MOTION_SCENARIO_ONLY_SHAKE);
		INFO("SHAKE motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_DIRECT_CALL:
		new_direct_call_event = MOTION_ENGINE_DIRECT_CALL_DETECTION;
		ME_clear_result(MOTION_SCENARIO_DIRECT_CALL);
		INFO("DIRECT_CALL motion detection result = [%d]\n" ,motion_result->motionEvent);
		break;
	case MOTION_EVENT_SMART_RELAY:
		new_smart_relay_event = MOTION_ENGINE_SMART_RELAY_DETECTION;
		INFO("SMART_RELAY motion detection result = [%d]\n" ,motion_result->motionEvent);
		{       /* the cope for the lock */
			AUTOLOCK(m_smart_relay_mutex);
			if (m_smart_relay_running) {
				ME_reset_angle(SMART_RELAY_FINI);
				ME_reset_angle(SMART_RELAY_INIT);
				INFO("Smart relay is reinitialized");
			}
		}
		break;
	case MOTION_EVENT_TILT_TO_UNLOCK:
		new_tilt_to_unlock_event = MOTION_ENGINE_TILT_TO_UNLOCK_DETECTION;
		ME_clear_result(MOTION_SCENARIO_TILT_TO_UNLOCK);
		INFO("TILT_TO_UNLOCK motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	case MOTION_EVENT_FLAT:
		new_no_move_event = MOTION_ENGINE_NO_MOVE_DETECTION;
		ME_clear_result(MOTION_SCENARIO_REACTIVE_ALERT);
		INFO("NO MOVE motion detection result = [%d]\n", motion_result->motionEvent);
		break;
	default:
		DBG("NO matched motion");
		break;
	}

	if (new_doubletap_event && is_active_event(MOTION_ENGINE_EVENT_DOUBLETAP))
		put_motion_event(MOTION_ENGINE_EVENT_DOUBLETAP, new_doubletap_event, outs);

	if (new_top_to_bottom_event && is_active_event(MOTION_ENGINE_EVENT_TOP_TO_BOTTOM))
		put_motion_event(MOTION_ENGINE_EVENT_TOP_TO_BOTTOM, new_top_to_bottom_event, outs);

	if (new_snap_event && is_active_event(MOTION_ENGINE_EVENT_SNAP))
		put_motion_event(MOTION_ENGINE_EVENT_SNAP, new_snap_event, outs);

	if (new_shake_event && is_active_event(MOTION_ENGINE_EVENT_SHAKE))
		put_motion_event(MOTION_ENGINE_EVENT_SHAKE, new_shake_event, outs);

	if (new_shake_event && is_active_event(MOTION_ENGINE_EVENT_SHAKE_ALWAYS_ON))
		put_motion_event(MOTION_ENGINE_EVENT_SHAKE_ALWAYS_ON, new_shake_event, outs);

	if (new_direct_call_event && is_active_event(MOTION_ENGINE_EVENT_DIRECT_CALL))
		put_motion_event(MOTION_ENGINE_EVENT_DIRECT_CALL, new_direct_call_event, outs);

	if (new_smart_relay_event && is_active_event(MOTION_ENGINE_EVENT_SMART_RELAY))
		put_motion_event(MOTION_ENGINE_EVENT_SMART_RELAY, new_smart_relay_event, outs);

	if (new_tilt_to_unlock_event && is_active_event(MOTION_ENGINE_EVENT_TILT_TO_UNLOCK))
		put_motion_event(MOTION_ENGINE_EVENT_TILT_TO_UNLOCK, new_tilt_to_unlock_event, outs);

	if (new_no_move_event && is_active_event(MOTION_ENGINE_EVENT_NO_MOVE))
		put_motion_event(MOTION_ENGINE_EVENT_NO_MOVE, new_no_move_event, outs);

	if (is_active_event(MOTION_ENGINE_EVENT_PANNING))
		put_tilt_event(MOTION_ENGINE_EVENT_PANNING, m_panning_scale_factor, outs);

	if (is_active_event(MOTION_ENGINE_EVENT_TILT))
		put_tilt_event(MOTION_ENGINE_EVENT_TILT, m_tilt_scale_factor, outs);

	if (is_active_event(MOTION_ENGINE_EVENT_PANNING_BROWSE))
		put_tilt_event(MOTION_ENGINE_EVENT_PANNING_BROWSE, m_panning_browse_scale_factor, outs);

	return;
}


bool motion_sensor::get_properties(sensor_properties_t &properties)
{
	properties.name = "Motion Sensor";
	properties.vendor = "Samsung Electronics";
	properties.min_range = 0;
	properties.max_range = 100;
	properties.min_interval = 1;
	properties.resolution = 1;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	return true;
}


bool motion_sensor::is_active_event(unsigned int type)
{
	auto it_motion_event_info = m_motion_info.find(type);

	if (it_motion_event_info == m_motion_info.end())
		return false;

	return (m_motion_activation_state || it_motion_event_info->second.always_on)
		&& it_motion_event_info->second.enabled;
}

bool motion_sensor::is_sensor_used(int sensor)
{
	auto it_motion_event_info = m_motion_info.begin();

	while (it_motion_event_info != m_motion_info.end()) {
		if (it_motion_event_info->second.used_sensor & sensor) {
			if ((m_motion_activation_state || it_motion_event_info->second.always_on)
				&& it_motion_event_info->second.enabled
				&& get_client_cnt(it_motion_event_info->first))
				return true;
		}

		++it_motion_event_info;
	}

	return false;
}

void motion_sensor::set_motion_enable_state(unsigned int type, bool enable)
{
	auto it_motion_event_info = m_motion_info.find(type);

	if (it_motion_event_info != m_motion_info.end())
		it_motion_event_info->second.enabled = enable;
}

void motion_sensor::synthesize(const sensor_event_t& event, vector<sensor_event_t> &outs)
{
	const float MIN_DELIVERY_DIFF_FACTOR = 0.75f;

	long long diff_time;

	if (event.event_type == ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME) {

		diff_time = event.data.timestamp - m_prev_acc_time;

		if (m_prev_acc_time && (diff_time < SAMPLING_TIME * MS_TO_US * MIN_DELIVERY_DIFF_FACTOR))
			return;

		m_prev_acc_time = event.data.timestamp;

		if (!m_gyro_sensor_started)
			m_g_x = m_g_y = m_g_z = 0;

		get_motion(event.data.values[0], event.data.values[1], event.data.values[2],
			m_g_x, m_g_y, m_g_z, m_proxi, outs);

		return;
	} else if (event.event_type == GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME) {

		diff_time = event.data.timestamp - m_prev_gyro_time;

		if (m_prev_gyro_time && (diff_time < SAMPLING_TIME * MS_TO_US * MIN_DELIVERY_DIFF_FACTOR))
			return;

		m_prev_gyro_time = event.data.timestamp;

		XYZ_TYPE *cal_gyro_xyz = NULL;

		cal_gyro_xyz = ME_get_gyro(event.data.values[0] * DEGREE_TO_RADIAN,
			event.data.values[1] * DEGREE_TO_RADIAN, event.data.values[2] * DEGREE_TO_RADIAN, SAMPLING_TIME);

		if (cal_gyro_xyz) {
			m_g_x = cal_gyro_xyz->x;
			m_g_y = cal_gyro_xyz->y;
			m_g_z = cal_gyro_xyz->z;

			if (!m_accel_sensor_started)
				get_motion(0.0, 0.0, 0.0, m_g_x, m_g_y, m_g_z, m_proxi, outs);
		}
		return;
	} else if (event.event_type == PROXIMITY_EVENT_CHANGE_STATE) {
		m_proxi = event.data.values[0] ? 0 : PROXI_FAR;
		return;
	}

	return;
}

extern "C" sensor_module* create(void)
{
	motion_sensor *sensor;

	try {
		sensor = new(std::nothrow) motion_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
