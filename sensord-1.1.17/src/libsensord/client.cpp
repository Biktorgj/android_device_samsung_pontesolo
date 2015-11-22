/*
 * libsensord
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
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

#include <sf_common.h>
#include <sensor_internal_deprecated.h>
#include <sensor_internal.h>
#include <csensor_event_listener.h>
#include <client_common.h>
#include <vconf.h>
#include <cmutex.h>
#include <common.h>
#include <sensor_info.h>
#include <sensor_info_manager.h>

#ifndef API
#define API __attribute__((visibility("default")))
#endif

#define MIN_INTERVAL 20

static const int OP_SUCCESS = 0;
static const int OP_ERROR =  -1;

static csensor_event_listener &event_listener = csensor_event_listener::get_instance();
static cmutex lock;

static int g_power_save_state = 0;

static int get_power_save_state(void);
static void power_save_state_cb(keynode_t *node, void *data);
static void clean_up(void);
static void good_bye(void);
static bool change_sensor_rep(sensor_id_t sensor_id, sensor_rep &prev_rep, sensor_rep &cur_rep);
static void restore_session(void);
static bool register_event(int handle, unsigned int event_type, unsigned int interval, int max_batch_latency, int cb_type, void* cb, void *user_data);

void init_client(void)
{
	event_listener.set_hup_observer(restore_session);
	atexit(good_bye);
}

static void good_bye(void)
{
	_D("Good bye! %s\n", get_client_name());
	clean_up();
}

static int g_power_save_state_cb_cnt = 0;

static void set_power_save_state_cb(void)
{
	if (g_power_save_state_cb_cnt < 0)
		_E("g_power_save_state_cb_cnt(%d) is wrong", g_power_save_state_cb_cnt);

	++g_power_save_state_cb_cnt;

	if (g_power_save_state_cb_cnt == 1) {
		_D("Power save callback is registered");
		g_power_save_state = get_power_save_state();
		_D("power_save_state = [%d]", g_power_save_state);
		vconf_notify_key_changed(VCONFKEY_PM_STATE, power_save_state_cb, NULL);
		vconf_notify_key_changed(VCONFKEY_SETAPPL_PSMODE, power_save_state_cb, NULL);
	}
}

static void unset_power_save_state_cb(void)
{
	--g_power_save_state_cb_cnt;

	if (g_power_save_state_cb_cnt < 0)
		_E("g_power_save_state_cb_cnt(%d) is wrong", g_power_save_state_cb_cnt);

	if (g_power_save_state_cb_cnt == 0) {
		_D("Power save callback is unregistered");
		vconf_ignore_key_changed(VCONFKEY_PM_STATE, power_save_state_cb);
		vconf_ignore_key_changed(VCONFKEY_SETAPPL_PSMODE, power_save_state_cb);
	}
}

static void clean_up(void)
{
	handle_vector handles;

	event_listener.get_all_handles(handles);

	auto it_handle = handles.begin();

	while (it_handle != handles.end()) {
		sf_disconnect(*it_handle);
		++it_handle;
	}
}


static int get_power_save_state (void)
{
	int state = 0;
	int pm_state, ps_state;

	vconf_get_int(VCONFKEY_PM_STATE, &pm_state);
	vconf_get_int(VCONFKEY_SETAPPL_PSMODE, &ps_state);

	if (pm_state == VCONFKEY_PM_STATE_LCDOFF)
		state |= SENSOR_OPTION_ON_IN_SCREEN_OFF;

	if (ps_state == SETTING_PSMODE_SURVIVAL)
		state |= SENSOR_OPTION_ON_IN_POWERSAVE_MODE;

	return state;
}

static void power_save_state_cb(keynode_t *node, void *data)
{
	int cur_power_save_state;
	sensor_id_vector sensors;
	sensor_rep prev_rep, cur_rep;

	AUTOLOCK(lock);

	cur_power_save_state = get_power_save_state();

	if (cur_power_save_state == g_power_save_state) {
		_T("g_power_save_state NOT changed : [%d]", cur_power_save_state);
		return;
	}

	g_power_save_state = cur_power_save_state;
	_D("power_save_state: %d noti to %s", g_power_save_state, get_client_name());

	event_listener.get_listening_sensors(sensors);

	auto it_sensor = sensors.begin();

	while (it_sensor != sensors.end()) {
		event_listener.get_sensor_rep(*it_sensor, prev_rep);
		event_listener.operate_sensor(*it_sensor, cur_power_save_state);
		event_listener.get_sensor_rep(*it_sensor, cur_rep);
		change_sensor_rep(*it_sensor, prev_rep, cur_rep);

		++it_sensor;
	}
}


static void restore_session(void)
{
	AUTOLOCK(lock);

	_I("Trying to restore session for %s", get_client_name());

	command_channel *cmd_channel;
	int client_id;

	event_listener.close_command_channel();
	event_listener.set_client_id(CLIENT_ID_INVALID);

	sensor_id_vector sensors;

	event_listener.get_listening_sensors(sensors);

	bool first_connection = true;

	auto it_sensor = sensors.begin();

	while (it_sensor != sensors.end()) {
		cmd_channel = new(std::nothrow) command_channel();
		retm_if (!cmd_channel, "Failed to allocate memory");

		if (!cmd_channel->create_channel()) {
			_E("%s failed to create command channel for %s", get_client_name(), get_sensor_name(*it_sensor));
			delete cmd_channel;
			goto FAILED;
		}

		event_listener.add_command_channel(*it_sensor, cmd_channel);

		if (first_connection) {
			first_connection = false;
			if (!cmd_channel->cmd_get_id(client_id)) {
				_E("Failed to get client id");
				goto FAILED;
			}

			event_listener.set_client_id(client_id);
			event_listener.start_event_listener();
		}

		cmd_channel->set_client_id(client_id);

		if (!cmd_channel->cmd_hello(*it_sensor)) {
			_E("Sending cmd_hello(%s, %d) failed for %s", get_sensor_name(*it_sensor), client_id, get_client_name());
			goto FAILED;
		}

		sensor_rep prev_rep, cur_rep;
		prev_rep.active = false;
		prev_rep.option = SENSOR_OPTION_DEFAULT;
		prev_rep.interval = 0;

		event_listener.get_sensor_rep(*it_sensor, cur_rep);
		if (!change_sensor_rep(*it_sensor, prev_rep, cur_rep)) {
			_E("Failed to change rep(%s) for %s", get_sensor_name(*it_sensor), get_client_name());
			goto FAILED;
		}

		++it_sensor;
	}

	_I("Succeeded to restore session for %s", get_client_name());

	return;

FAILED:
	event_listener.clear();
	_E("Failed to restore session for %s", get_client_name());
}

static bool get_events_diff(event_type_vector &a_vec, event_type_vector &b_vec, event_type_vector &add_vec, event_type_vector &del_vec)
{
	sort(a_vec.begin(), a_vec.end());
	sort(b_vec.begin(), b_vec.end());

	set_difference(a_vec.begin(), a_vec.end(), b_vec.begin(), b_vec.end(), back_inserter(del_vec));
	set_difference(b_vec.begin(), b_vec.end(), a_vec.begin(), a_vec.end(), back_inserter(add_vec));

	return !(add_vec.empty() && del_vec.empty());
}


static bool change_sensor_rep(sensor_id_t sensor_id, sensor_rep &prev_rep, sensor_rep &cur_rep)
{
	int client_id;
	command_channel *cmd_channel;
	event_type_vector add_event_types, del_event_types;

	if (!event_listener.get_command_channel(sensor_id, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor_id));
		return false;
	}

	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), false, "Invalid client id : %d, %s, %s", client_id, get_sensor_name(sensor_id), get_client_name());

	get_events_diff(prev_rep.event_types, cur_rep.event_types, add_event_types, del_event_types);

	if (cur_rep.active) {
		if (prev_rep.option != cur_rep.option) {
			if (!cmd_channel->cmd_set_option(cur_rep.option)) {
				ERR("Sending cmd_set_option(%d, %s, %d) failed for %s", client_id, get_sensor_name(sensor_id), cur_rep.option, get_client_name());
				return false;
			}
		}

		if (prev_rep.interval != cur_rep.interval) {
			unsigned int min_interval;

			if (cur_rep.interval == 0)
				min_interval= POLL_MAX_HZ_MS;
			else
				min_interval = cur_rep.interval;

			if (cur_rep.event_types.empty())
				min_interval = POLL_10HZ_MS;

			if (!cmd_channel->cmd_set_interval(min_interval)) {
				ERR("Sending cmd_set_interval(%d, %s, %d) failed for %s", client_id, get_sensor_name(sensor_id), min_interval, get_client_name());
				return false;
			}
		}

		if (!add_event_types.empty()) {
			if (!cmd_channel->cmd_register_events(add_event_types)) {
				ERR("Sending cmd_register_events(%d, add_event_types) failed for %s", client_id, get_client_name());
				return false;
			}
		}

	}

	if (prev_rep.active && !del_event_types.empty()) {
		if (!cmd_channel->cmd_unregister_events(del_event_types)) {
			ERR("Sending cmd_unregister_events(%d, del_event_types) failed for %s", client_id, get_client_name());
			return false;
		}
	}

	if (prev_rep.active != cur_rep.active) {
		if (cur_rep.active) {
			if (!cmd_channel->cmd_start()) {
				ERR("Sending cmd_start(%d, %s) failed for %s", client_id, get_sensor_name(sensor_id), get_client_name());
				return false;
			}
		} else {
			if (!cmd_channel->cmd_unset_interval()) {
				ERR("Sending cmd_unset_interval(%d, %s) failed for %s", client_id, get_sensor_name(sensor_id), get_client_name());
				return false;
			}

			if (!cmd_channel->cmd_stop()) {
				ERR("Sending cmd_stop(%d, %s) failed for %s", client_id, get_sensor_name(sensor_id), get_client_name());
				return false;
			}
		}
	}

	return true;
}

API int sf_connect(sensor_type_t sensor_type)
{
	sensor_t sensor;

	sensor = sensord_get_sensor(sensor_type);

	return sensord_connect(sensor);
}

API int sf_disconnect(int handle)
{
	return sensord_disconnect(handle) ? OP_SUCCESS : OP_ERROR;
}

API int sf_start(int handle, int option)
{
	return sensord_start(handle, option) ? OP_SUCCESS : OP_ERROR;
}

API int sf_stop(int handle)
{
	return sensord_stop(handle) ? OP_SUCCESS : OP_ERROR;
}

API int sf_register_event(int handle, unsigned int event_type, event_condition_t *event_condition, sensor_callback_func_t cb, void *user_data)
{
	unsigned int interval = BASE_GATHERING_INTERVAL;

	if (event_condition != NULL) {
		if ((event_condition->cond_op == CONDITION_EQUAL) && (event_condition->cond_value1 > 0))
			interval = event_condition->cond_value1;
	}

	return register_event(handle, event_type, interval, 0, SENSOR_LEGACY_CB, (void*) cb, user_data) ? OP_SUCCESS : OP_ERROR;
}

API int sf_unregister_event(int handle, unsigned int event_type)
{
	return sensord_unregister_event(handle, event_type) ? OP_SUCCESS : OP_ERROR;
}

API int sf_change_event_condition(int handle, unsigned int event_type, event_condition_t *event_condition)
{
	unsigned int interval = BASE_GATHERING_INTERVAL;

	if (event_condition != NULL) {
		if ((event_condition->cond_op == CONDITION_EQUAL) && (event_condition->cond_value1 > 0))
			interval = event_condition->cond_value1;
	}

	return sensord_change_event_interval(handle, event_type, interval) ? OP_SUCCESS : OP_ERROR;
}

API int sf_change_sensor_option(int handle, int option)
{
	return sensord_set_option(handle, option) ? OP_SUCCESS : OP_ERROR;
}


API int sf_send_sensorhub_data(int handle, const char* data, int data_len)
{
	return sensord_send_sensorhub_data(handle, data, data_len) ? OP_SUCCESS : OP_ERROR;
}

API int sf_get_data(int handle, unsigned int data_id, sensor_data_t* sensor_data)
{
	return sensord_get_data(handle, data_id, sensor_data) ? OP_SUCCESS : OP_ERROR;
}

static bool get_sensor_list(void)
{
	static cmutex l;
	static bool init = false;

	AUTOLOCK(l);

	if (!init) {
		command_channel cmd_channel;

		if (!cmd_channel.create_channel()) {
			ERR("%s failed to create command channel", get_client_name());
			return false;
		}

		if (!cmd_channel.cmd_get_sensor_list())
			return false;

		init = true;
	}

	return true;
}

API bool sensord_get_sensor_list(sensor_type_t type, sensor_t **list, int *sensor_count)
{
	retvm_if (!get_sensor_list(), false, "Fail to get sensor list from server");

	vector<sensor_info *> sensor_infos = sensor_info_manager::get_instance().get_infos(type);

	if (!sensor_infos.empty()) {
		*list = (sensor_t *) malloc(sizeof(sensor_info *) * sensor_infos.size());
		retvm_if(!*list, false, "Failed to allocate memory");
	}

	for (int i = 0; i < sensor_infos.size(); ++i)
		*(*list + i) = sensor_info_to_sensor(sensor_infos[i]);

	*sensor_count = sensor_infos.size();

	return true;
}

API sensor_t sensord_get_sensor(sensor_type_t type)
{
	retvm_if (!get_sensor_list(), false, "Fail to get sensor list from server");

	const sensor_info *info;

	info = sensor_info_manager::get_instance().get_info(type);

	return sensor_info_to_sensor(info);
}

API bool sensord_get_type(sensor_t sensor, sensor_type_t *type)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info) || !type,
		NULL, "Invalid param: sensor (%p), type(%p)", sensor, type);

	*type = info->get_type();

	return true;
}

API const char* sensord_get_name(sensor_t sensor)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info),
		NULL, "Invalid param: sensor (%p)", sensor);

	return info->get_name();
}

API const char* sensord_get_vendor(sensor_t sensor)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info),
		NULL, "Invalid param: sensor (%p)", sensor);

	return info->get_vendor();
}

API bool sensord_get_privilege(sensor_t sensor, sensor_privilege_t *privilege)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info) || !privilege,
		false, "Invalid param: sensor (%p), privilege(%p)", sensor, privilege);

	*privilege = info->get_privilege();

	return true;
}

API bool sensord_get_min_range(sensor_t sensor, float *min_range)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info) || !min_range,
		false, "Invalid param: sensor (%p), min_range(%p)", sensor, min_range);

	*min_range = info->get_min_range();

	return true;
}

API bool sensord_get_max_range(sensor_t sensor, float *max_range)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info) || !max_range,
		false, "Invalid param: sensor (%p), max_range(%p)", sensor, max_range);

	*max_range = info->get_max_range();

	return true;
}

API bool sensord_get_resolution(sensor_t sensor, float *resolution)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info) || !resolution,
		false, "Invalid param: sensor (%p), resolution(%p)", sensor, resolution);

	*resolution = info->get_resolution();

	return true;
}

API bool sensord_get_min_interval(sensor_t sensor, int *min_interval)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info) || !min_interval,
		false, "Invalid param: sensor (%p), min_interval(%p)", sensor, min_interval);

	*min_interval = info->get_min_interval();

	return true;
}

API bool sensord_get_fifo_count(sensor_t sensor, int *fifo_count)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info) || !fifo_count,
		false, "Invalid param: sensor (%p), fifo_count(%p)", sensor, fifo_count);

	*fifo_count = info->get_fifo_count();

	return true;
}

API bool sensord_get_max_batch_count(sensor_t sensor, int *max_batch_count)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info) || !max_batch_count,
		false, "Invalid param: sensor (%p), max_batch_count(%p)", sensor, max_batch_count);

	*max_batch_count = info->get_max_batch_count();

	return true;
}

API bool sensord_get_supported_event_types(sensor_t sensor, unsigned int **event_types, int *count)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info) || !event_types || !count,
		false, "Invalid param: sensor (%p), event_types(%p), count(%)", sensor, event_types, count);

	vector<unsigned int> event_vec;

	info->get_supported_events(event_vec);
	*event_types = (unsigned int *) malloc(sizeof(unsigned int) * event_vec.size());
	retvm_if(!*event_types, false, "Failed to allocate memory");

	copy(event_vec.begin(), event_vec.end(), *event_types);
	*count = event_vec.size();

	return true;
}

API bool sensord_is_supported_event_type(sensor_t sensor, unsigned int event_type, bool *supported)
{
	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info) || !event_type || !supported,
		false, "Invalid param: sensor (%p), event_type(%p), supported(%)", sensor, event_type, supported);

	*supported = info->is_supported_event(event_type);

	return true;
}

API int sensord_connect(sensor_t sensor)
{
	command_channel *cmd_channel = NULL;
	int handle;
	int client_id;
	bool sensor_registered;
	bool first_connection = false;

	sensor_info* info = sensor_to_sensor_info(sensor);

	retvm_if (!sensor_info_manager::get_instance().is_valid(info),
		OP_ERROR, "Invalid param: sensor (%p)", sensor);

	sensor_id_t sensor_id =  info->get_id();

	AUTOLOCK(lock);

	sensor_registered = event_listener.is_sensor_registered(sensor_id);

	handle = event_listener.create_handle(sensor_id);
	if (handle == MAX_HANDLE) {
		ERR("Maximum number of handles reached, sensor: %s in client %s", get_sensor_name(sensor_id), get_client_name());
		return OP_ERROR;
	}

	if (!sensor_registered) {
		cmd_channel = new(std::nothrow) command_channel();
		retvm_if (!cmd_channel, OP_ERROR, "Failed to allocate memory");

		if (!cmd_channel->create_channel()) {
			ERR("%s failed to create command channel for %s", get_client_name(), get_sensor_name(sensor_id));
			event_listener.delete_handle(handle);
			delete cmd_channel;
			return OP_ERROR;
		}

		event_listener.add_command_channel(sensor_id, cmd_channel);
	}

	if (!event_listener.get_command_channel(sensor_id, &cmd_channel)) {
		ERR("%s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor_id));
		event_listener.delete_handle(handle);
		return OP_ERROR;
	}

	if (!event_listener.has_client_id()) {
		first_connection = true;
		if(!cmd_channel->cmd_get_id(client_id)) {
			ERR("Sending cmd_get_id() failed for %s", get_sensor_name(sensor_id));
			event_listener.close_command_channel(sensor_id);
			event_listener.delete_handle(handle);
			return OP_ERROR;
		}

		event_listener.set_client_id(client_id);
		INFO("%s gets client_id [%d]", get_client_name(), client_id);
		event_listener.start_event_listener();
		INFO("%s starts listening events with client_id [%d]", get_client_name(), client_id);
	}

	client_id = event_listener.get_client_id();
	cmd_channel->set_client_id(client_id);

	INFO("%s[%d] connects with %s[%d]", get_client_name(), client_id, get_sensor_name(sensor_id), handle);

	event_listener.set_sensor_params(handle, SENSOR_STATE_STOPPED, SENSOR_OPTION_DEFAULT);

	if (!sensor_registered) {
		if(!cmd_channel->cmd_hello(sensor_id)) {
			ERR("Sending cmd_hello(%s, %d) failed for %s", get_sensor_name(sensor_id), client_id, get_client_name());
			event_listener.close_command_channel(sensor_id);
			event_listener.delete_handle(handle);
			if (first_connection) {
				event_listener.set_client_id(CLIENT_ID_INVALID);
				event_listener.stop_event_listener();
			}
			return OP_ERROR;
		}
	}

	set_power_save_state_cb();
	return handle;


}


API bool sensord_disconnect(int handle)
{
	command_channel *cmd_channel;
	sensor_id_t sensor_id;
	int client_id;
	int sensor_state;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_state(handle, sensor_state)||
		!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}

	if (!event_listener.get_command_channel(sensor_id, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor_id));
		return false;
	}

	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), false, "Invalid client id : %d, handle: %d, %s, %s", client_id, handle, get_sensor_name(sensor_id), get_client_name());

	INFO("%s disconnects with %s[%d]", get_client_name(), get_sensor_name(sensor_id), handle);

	if (sensor_state != SENSOR_STATE_STOPPED) {
		WARN("%s[%d] for %s is not stopped before disconnecting.",
			get_sensor_name(sensor_id), handle, get_client_name());
		sensord_stop(handle);
	}

	if (!event_listener.delete_handle(handle))
		return false;

	if (!event_listener.is_active())
		event_listener.set_client_id(CLIENT_ID_INVALID);

	if (!event_listener.is_sensor_registered(sensor_id)) {
		if(!cmd_channel->cmd_byebye()) {
			ERR("Sending cmd_byebye(%d, %s) failed for %s", client_id, get_sensor_name(sensor_id), get_client_name());
			return false;
		}
		event_listener.close_command_channel(sensor_id);
	}

	if (!event_listener.is_active()) {
		INFO("Stop listening events for client %s with client id [%d]", get_client_name(), event_listener.get_client_id());
		event_listener.stop_event_listener();
	}

	unset_power_save_state_cb();

	return true;
}


static bool register_event(int handle, unsigned int event_type, unsigned int interval, int max_batch_latency, int cb_type, void* cb, void *user_data)
{
	sensor_id_t sensor_id;
	sensor_rep prev_rep, cur_rep;
	bool ret;

	retvm_if (!cb, false, "callback is NULL");

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}

	if (interval < MIN_INTERVAL)
		interval = MIN_INTERVAL;

	INFO("%s registers event %s[0x%x] for sensor %s[%d] with interval: %d, cb: 0x%x, user_data: 0x%x", get_client_name(), get_event_name(event_type),
			event_type, get_sensor_name(sensor_id), handle, interval, cb, user_data);

	event_listener.get_sensor_rep(sensor_id, prev_rep);
	event_listener.register_event(handle, event_type, interval, cb_type, cb, user_data);
	event_listener.get_sensor_rep(sensor_id, cur_rep);
	ret = change_sensor_rep(sensor_id, prev_rep, cur_rep);

	if (!ret)
		event_listener.unregister_event(handle, event_type);

	return ret;
}

API bool sensord_register_event(int handle, unsigned int event_type, unsigned int interval, unsigned int max_batch_latency, sensor_cb_t cb, void *user_data)
{
	return register_event(handle, event_type, interval, max_batch_latency, SENSOR_EVENT_CB, (void *)cb, user_data);
}

API bool sensord_register_hub_event(int handle, unsigned int event_type, unsigned int interval, unsigned int max_batch_latency, sensorhub_cb_t cb, void *user_data)
{
	return register_event(handle, event_type, interval, max_batch_latency, SENSORHUB_EVENT_CB, (void *)cb, user_data);
}


API bool sensord_unregister_event(int handle, unsigned int event_type)
{
	sensor_id_t sensor_id;
	sensor_rep prev_rep, cur_rep;
	bool ret;
	unsigned int prev_interval;
	int prev_cb_type;
	void *prev_cb;
	void *prev_user_data;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}

	INFO("%s unregisters event %s[0x%x] for sensor %s[%d]", get_client_name(), get_event_name(event_type),
		event_type, get_sensor_name(sensor_id), handle);

	event_listener.get_sensor_rep(sensor_id, prev_rep);
	event_listener.get_event_info(handle, event_type, prev_interval, prev_cb_type, prev_cb, prev_user_data);

	if (!event_listener.unregister_event(handle, event_type)) {
		ERR("%s try to unregister non registered event %s[0x%x] for sensor %s[%d]",
			get_client_name(),get_event_name(event_type), event_type, get_sensor_name(sensor_id), handle);
		return false;
	}

	event_listener.get_sensor_rep(sensor_id, cur_rep);
	ret =  change_sensor_rep(sensor_id, prev_rep, cur_rep);

	if (!ret)
		event_listener.register_event(handle, event_type, prev_interval, prev_cb_type, prev_cb, prev_user_data);

	return ret;

}


API bool sensord_register_accuracy_cb(int handle, sensor_accuracy_changed_cb_t cb, void *user_data)
{
	sensor_id_t sensor_id;

	retvm_if (!cb, false, "callback is NULL");

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}


	INFO("%s registers accuracy_changed_cb for sensor %s[%d] with cb: 0x%x, user_data: 0x%x",
		get_client_name(), get_sensor_name(sensor_id), handle, cb, user_data);

	event_listener.register_accuracy_cb(handle, cb , user_data);

	return true;

}

API bool sensord_unregister_accuracy_cb(int handle)
{
	sensor_id_t sensor_id;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}


	INFO("%s unregisters accuracy_changed_cb for sensor %s[%d]",
		get_client_name(), get_sensor_name(sensor_id), handle);

	event_listener.unregister_accuracy_cb(handle);

	return true;
}


API bool sensord_start(int handle, int option)
{
	sensor_id_t sensor_id;
	sensor_rep prev_rep, cur_rep;
	bool ret;
	int prev_state, prev_option;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}

	retvm_if ((option < 0) || (option >= SENSOR_OPTION_END), false, "Invalid option value : %d, handle: %d, %s, %s",
		option, handle, get_sensor_name(sensor_id), get_client_name());

	INFO("%s starts %s[%d], with option: %d, power save state: %d", get_client_name(), get_sensor_name(sensor_id),
		handle, option, g_power_save_state);

	if (g_power_save_state && !(g_power_save_state & option)) {
		event_listener.set_sensor_params(handle, SENSOR_STATE_PAUSED, option);
		return true;
	}

	event_listener.get_sensor_rep(sensor_id, prev_rep);
	event_listener.get_sensor_params(handle, prev_state, prev_option);
	event_listener.set_sensor_params(handle, SENSOR_STATE_STARTED, option);
	event_listener.get_sensor_rep(sensor_id, cur_rep);

	ret = change_sensor_rep(sensor_id, prev_rep, cur_rep);

	if (!ret)
		event_listener.set_sensor_params(handle, prev_state, prev_option);

	return ret;
}

API bool sensord_stop(int handle)
{
	sensor_id_t sensor_id;
	int sensor_state;
	bool ret;
	int prev_state, prev_option;

	sensor_rep prev_rep, cur_rep;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_state(handle, sensor_state)||
		!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}

	retvm_if ((sensor_state == SENSOR_STATE_STOPPED), true, "%s already stopped with %s[%d]",
		get_client_name(), get_sensor_name(sensor_id), handle);


	INFO("%s stops sensor %s[%d]", get_client_name(), get_sensor_name(sensor_id), handle);

	event_listener.get_sensor_rep(sensor_id, prev_rep);
	event_listener.get_sensor_params(handle, prev_state, prev_option);
	event_listener.set_sensor_state(handle, SENSOR_STATE_STOPPED);
	event_listener.get_sensor_rep(sensor_id, cur_rep);

	ret = change_sensor_rep(sensor_id, prev_rep, cur_rep);

	if (!ret)
		event_listener.set_sensor_params(handle, prev_state, prev_option);

	return ret;
}


API bool sensord_change_event_interval(int handle, unsigned int event_type, unsigned int interval)
{
	sensor_id_t sensor_id;
	sensor_rep prev_rep, cur_rep;
	bool ret;
	unsigned int prev_interval;
	int prev_cb_type;
	void *prev_cb;
	void *prev_user_data;
	unsigned int _interval;

	_interval = interval;
	if (_interval < MIN_INTERVAL)
		_interval = MIN_INTERVAL;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}

	INFO("%s changes interval of event %s[0x%x] for %s[%d] to interval %d", get_client_name(), get_event_name(event_type),
			event_type, get_sensor_name(sensor_id), handle, _interval);

	event_listener.get_sensor_rep(sensor_id, prev_rep);

	event_listener.get_event_info(handle, event_type, prev_interval, prev_cb_type, prev_cb, prev_user_data);

	if (!event_listener.set_event_interval(handle, event_type, _interval))
		return false;

	event_listener.get_sensor_rep(sensor_id, cur_rep);

	ret = change_sensor_rep(sensor_id, prev_rep, cur_rep);

	if (!ret)
		event_listener.set_event_interval(handle, event_type, prev_interval);

	return ret;

}

API bool sensord_change_event_max_batch_latency(int handle, unsigned int max_batch_latency)
{
	return false;
}


API bool sensord_set_option(int handle, int option)
{
	sensor_id_t sensor_id;
	sensor_rep prev_rep, cur_rep;
	int sensor_state;
	bool ret;
	int prev_state, prev_option;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_state(handle, sensor_state)||
		!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}

	retvm_if ((option < 0) || (option >= SENSOR_OPTION_END), false, "Invalid option value : %d, handle: %d, %s, %s",
		option, handle, get_sensor_name(sensor_id), get_client_name());


	event_listener.get_sensor_rep(sensor_id, prev_rep);
	event_listener.get_sensor_params(handle, prev_state, prev_option);

	if (g_power_save_state) {
		if ((option & g_power_save_state) && (sensor_state == SENSOR_STATE_PAUSED))
			event_listener.set_sensor_state(handle, SENSOR_STATE_STARTED);
		else if (!(option & g_power_save_state) && (sensor_state == SENSOR_STATE_STARTED))
			event_listener.set_sensor_state(handle, SENSOR_STATE_PAUSED);
	}
	event_listener.set_sensor_option(handle, option);

	event_listener.get_sensor_rep(sensor_id, cur_rep);
	ret =  change_sensor_rep(sensor_id, prev_rep, cur_rep);

	if (!ret)
		event_listener.set_sensor_option(handle, prev_option);

	return ret;

}

bool sensord_send_sensorhub_data(int handle, const char *data, int data_len)
{
	sensor_id_t sensor_id;
	command_channel *cmd_channel;
	int client_id;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}

	retvm_if (sensor_id != CONTEXT_SENSOR, false, "%s use this API wrongly, only for CONTEXT_SENSOR not for %s",
		get_client_name(), get_sensor_name(sensor_id));

	if (!event_listener.get_command_channel(sensor_id, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor_id));
		return false;
	}

	retvm_if((data_len < 0) || (data == NULL), false, "Invalid data_len: %d, data: 0x%x, handle: %d, %s, %s",
		data_len, data, handle, get_sensor_name(sensor_id), get_client_name());

	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), false, "Invalid client id : %d, handle: %d, %s, %s", client_id, handle, get_sensor_name(sensor_id), get_client_name());

	retvm_if (!event_listener.is_sensor_active(sensor_id), false, "%s with client_id:%d is not active state for %s with handle: %d",
		get_sensor_name(sensor_id), client_id, get_client_name(), handle);

	if (!cmd_channel->cmd_send_sensorhub_data(data, data_len)) {
		ERR("Sending cmd_send_sensorhub_data(%d, %d, 0x%x) failed for %s",
			client_id, data_len, data, get_client_name);
		return false;
	}

	return true;

}

API bool sensord_get_data(int handle, unsigned int data_id, sensor_data_t* sensor_data)
{
	sensor_id_t sensor_id;
	command_channel *cmd_channel;
	int sensor_state;
	int client_id;

	retvm_if ((!sensor_data), false, "sensor_data is NULL");

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_state(handle, sensor_state)||
		!event_listener.get_sensor_id(handle, sensor_id)) {
		ERR("client %s failed to get handle information", get_client_name());
		return false;
	}

	if (!event_listener.get_command_channel(sensor_id, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor_id));
		return false;
	}


	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), false, "Invalid client id : %d, handle: %d, %s, %s", client_id, handle, get_sensor_name(sensor_id), get_client_name());

	if (sensor_state != SENSOR_STATE_STARTED) {
		ERR("Sensor %s is not started for client %s with handle: %d, sensor_state: %d", get_sensor_name(sensor_id), get_client_name(), handle, sensor_state);
		return false;
	}

	if(!cmd_channel->cmd_get_data(data_id, sensor_data)) {
		ERR("cmd_get_data(%d, %d, 0x%x) failed for %s", client_id, data_id, sensor_data, get_client_name());
		return false;
	}

	return true;

}
