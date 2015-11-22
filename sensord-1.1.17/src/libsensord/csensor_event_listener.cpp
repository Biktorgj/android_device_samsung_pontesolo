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

#include <csensor_event_listener.h>
#include <client_common.h>
#include <sf_common.h>
#include <sensor_info_manager.h>

#include <thread>
#include <chrono>

using std::thread;
using std::pair;

csensor_event_listener::csensor_event_listener()
: m_client_id(CLIENT_ID_INVALID)
, m_thread_state(THREAD_STATE_TERMINATE)
, m_poller(NULL)
, m_hup_observer(NULL)
{
}

csensor_event_listener::~csensor_event_listener()
{
	stop_event_listener();
}


csensor_event_listener& csensor_event_listener::get_instance(void)
{
	static csensor_event_listener inst;
	return inst;
}


int csensor_event_listener::create_handle(sensor_id_t sensor)
{
	csensor_handle_info handle_info;
	int handle = 0;

	AUTOLOCK(m_handle_info_lock);

	while (m_sensor_handle_infos.count(handle) > 0)
		handle++;

	if (handle == MAX_HANDLE) {
		ERR("Handles of client %s are full", get_client_name());
		return MAX_HANDLE_REACHED;
	}

	handle_info.m_sensor_id = sensor;
	handle_info.m_sensor_state = SENSOR_STATE_STOPPED;
	handle_info.m_sensor_option = SENSOR_OPTION_DEFAULT;
	handle_info.m_handle = handle;
	handle_info.m_accuracy = -1;
	handle_info.m_accuracy_cb = NULL;
	handle_info.m_accuracy_user_data = NULL;

	m_sensor_handle_infos.insert(pair<int,csensor_handle_info> (handle, handle_info));

	return handle;
}

bool csensor_event_listener::delete_handle(int handle)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	m_sensor_handle_infos.erase(it_handle);
	return true;
}


bool csensor_event_listener::is_active()
{
	AUTOLOCK(m_handle_info_lock);

	return !m_sensor_handle_infos.empty();
}

bool csensor_event_listener::start_handle(int handle)
{
	return set_sensor_state(handle, SENSOR_STATE_STARTED);
}

bool csensor_event_listener::stop_handle(int handle)
{
	return set_sensor_state(handle, SENSOR_STATE_STOPPED);
}

bool csensor_event_listener::register_event(int handle, unsigned int event_type,
		unsigned int interval, int cb_type, void *cb, void* user_data)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	if (!it_handle->second.add_reg_event_info(event_type, interval, cb_type, cb, user_data))
		return false;

	return true;
}

bool csensor_event_listener::unregister_event(int handle, unsigned int event_type)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	if (!it_handle->second.delete_reg_event_info(event_type))
		return false;

	return true;
}

bool csensor_event_listener::change_event_interval(int handle, unsigned int event_type,
		unsigned int interval)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	if (!it_handle->second.change_reg_event_interval(event_type, interval))
		return false;

	return true;
}

bool csensor_event_listener::register_accuracy_cb(int handle, sensor_accuracy_changed_cb_t cb, void* user_data)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	it_handle->second.m_accuracy = -1;
	it_handle->second.m_accuracy_cb = cb;
	it_handle->second.m_accuracy_user_data = user_data;

	return true;
}

bool csensor_event_listener::unregister_accuracy_cb(int handle)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	it_handle->second.m_accuracy = -1;
	it_handle->second.m_accuracy_cb = NULL;
	it_handle->second.m_accuracy_user_data = NULL;

	return true;
}

bool csensor_event_listener::set_sensor_params(int handle, int sensor_state, int sensor_option)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	it_handle->second.m_sensor_state = sensor_state;
	it_handle->second.m_sensor_option = sensor_option;

	return true;
}

bool csensor_event_listener::get_sensor_params(int handle, int &sensor_state, int &sensor_option)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	sensor_state = it_handle->second.m_sensor_state;
	sensor_option = it_handle->second.m_sensor_option;

	return true;
}

bool csensor_event_listener::set_sensor_state(int handle, int sensor_state)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	it_handle->second.m_sensor_state = sensor_state;

	return true;
}

bool csensor_event_listener::set_sensor_option(int handle, int sensor_option)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	it_handle->second.m_sensor_option = sensor_option;

	return true;
}

bool csensor_event_listener::set_event_interval(int handle, unsigned int event_type, unsigned int interval)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	if (!it_handle->second.change_reg_event_interval(event_type, interval))
		return false;

	return true;
}


bool csensor_event_listener::get_event_info(int handle, unsigned int event_type, unsigned int &interval, int &cb_type, void* &cb, void* &user_data)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	const creg_event_info *event_info;

	event_info = it_handle->second.get_reg_event_info(event_type);

	if (!event_info)
		return NULL;


	interval = event_info->m_interval;
	cb_type = event_info->m_cb_type;
	cb = event_info->m_cb;
	user_data = event_info->m_user_data;

	return true;
}


void csensor_event_listener::get_listening_sensors(sensor_id_vector &sensors)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		sensors.push_back(it_handle->second.m_sensor_id);
		++it_handle;
	}

	sort(sensors.begin(), sensors.end());
	unique(sensors.begin(),sensors.end());
}


void csensor_event_listener::get_sensor_rep(sensor_id_t sensor, sensor_rep& rep)
{
	AUTOLOCK(m_handle_info_lock);

	rep.active = is_sensor_active(sensor);
	rep.option = get_active_option(sensor);
	rep.interval = get_active_min_interval(sensor);
	get_active_event_types(sensor, rep.event_types);

}

void csensor_event_listener::operate_sensor(sensor_id_t sensor, int power_save_state)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if (it_handle->second.m_sensor_id == sensor) {
			if ((it_handle->second.m_sensor_state == SENSOR_STATE_STARTED) &&
				power_save_state &&
				!(it_handle->second.m_sensor_option & power_save_state)) {

				it_handle->second.m_sensor_state = SENSOR_STATE_PAUSED;
				INFO("%s's %s[%d] is paused", get_client_name(), get_sensor_name(sensor), it_handle->first);

			} else if ((it_handle->second.m_sensor_state == SENSOR_STATE_PAUSED) &&
				(!power_save_state || (it_handle->second.m_sensor_option & power_save_state))) {

				it_handle->second.m_sensor_state = SENSOR_STATE_STARTED;
				INFO("%s's %s[%d] is resumed", get_client_name(), get_sensor_name(sensor), it_handle->first);
			}
		}

		++it_handle;
	}
}

bool csensor_event_listener::add_command_channel(sensor_id_t sensor, command_channel *cmd_channel)
{
	auto it_channel = m_command_channels.find(sensor);

	if (it_channel != m_command_channels.end()) {
		ERR("%s alreay has command_channel for %s", get_client_name(), get_sensor_name(sensor));
		return false;
	}

	m_command_channels.insert(pair<sensor_id_t, command_channel *> (sensor, cmd_channel));

	return true;

}
bool csensor_event_listener::get_command_channel(sensor_id_t sensor, command_channel **cmd_channel)
{
	auto it_channel = m_command_channels.find(sensor);

	if (it_channel == m_command_channels.end()) {
		ERR("%s doesn't have command_channel for %s", get_client_name(), get_sensor_name(sensor));
		return false;
	}

	*cmd_channel = it_channel->second;

	return true;
}


bool csensor_event_listener::close_command_channel(void)
{
	auto it_channel = m_command_channels.begin();

	if (it_channel != m_command_channels.end()) {
		delete it_channel->second;
		++it_channel;
	}

	m_command_channels.clear();

	return true;
}

bool csensor_event_listener::close_command_channel(sensor_id_t sensor_id)
{
	auto it_channel = m_command_channels.find(sensor_id);

	if (it_channel == m_command_channels.end()) {
		ERR("%s doesn't have command_channel for %s", get_client_name(), get_sensor_name(sensor_id));
		return false;
	}

	delete it_channel->second;

	m_command_channels.erase(it_channel);

	return true;
}


bool csensor_event_listener::has_client_id(void)
{
	return (m_client_id != CLIENT_ID_INVALID);
}

int csensor_event_listener::get_client_id(void)
{
	return m_client_id;
}

void csensor_event_listener::set_client_id(int client_id)
{
	m_client_id = client_id;
}

unsigned int csensor_event_listener::get_active_min_interval(sensor_id_t sensor)
{
	unsigned int min_interval = POLL_MAX_HZ_MS;
	bool active_sensor_found = false;
	unsigned int interval;

	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if ((it_handle->second.m_sensor_id == sensor) &&
			(it_handle->second.m_sensor_state == SENSOR_STATE_STARTED)) {
				active_sensor_found = true;
				interval = it_handle->second.get_min_interval();
				min_interval = (interval < min_interval) ? interval : min_interval;
		}

		++it_handle;
	}

	if (!active_sensor_found)
		DBG("Active sensor[0x%x] is not found for client %s", sensor, get_client_name());

	return (active_sensor_found) ? min_interval : 0;

}

unsigned int csensor_event_listener::get_active_option(sensor_id_t sensor)
{
	int active_option = SENSOR_OPTION_DEFAULT;
	bool active_sensor_found = false;
	int option;

	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if ((it_handle->second.m_sensor_id == sensor) &&
			(it_handle->second.m_sensor_state == SENSOR_STATE_STARTED)) {
				active_sensor_found = true;
				option = it_handle->second.m_sensor_option;
				active_option = (option > active_option) ? option : active_option;
		}

		++it_handle;
	}

	if (!active_sensor_found)
		DBG("Active sensor[0x%x] is not found for client %s", sensor, get_client_name());

	return active_option;
}

bool csensor_event_listener::get_sensor_id(int handle, sensor_id_t &sensor)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	sensor = it_handle->second.m_sensor_id;

	return true;
}

bool csensor_event_listener::get_sensor_state(int handle, int &sensor_state)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	sensor_state = it_handle->second.m_sensor_state;

	return true;
}

void csensor_event_listener::get_active_event_types(sensor_id_t sensor, event_type_vector &active_event_types)
{
	event_type_vector event_types;

	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if ((it_handle->second.m_sensor_id == sensor) &&
			(it_handle->second.m_sensor_state == SENSOR_STATE_STARTED))
				it_handle->second.get_reg_event_types(event_types);

		++it_handle;
	}

	if (event_types.empty())
		return;

	sort(event_types.begin(), event_types.end());

	unique_copy(event_types.begin(), event_types.end(), back_inserter(active_event_types));

}


void csensor_event_listener::get_all_handles(handle_vector &handles)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		handles.push_back(it_handle->first);
		++it_handle;
	}
}

bool csensor_event_listener::is_sensor_registered(sensor_id_t sensor)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if (it_handle->second.m_sensor_id == sensor)
			return true;

		++it_handle;
	}

	return false;
}


bool csensor_event_listener::is_sensor_active(sensor_id_t sensor)
{
	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if ((it_handle->second.m_sensor_id == sensor) &&
			(it_handle->second.m_sensor_state == SENSOR_STATE_STARTED))
			return true;

		++it_handle;
	}

	return false;
}

client_callback_info* csensor_event_listener::handle_calibration_cb(csensor_handle_info &handle_info, unsigned event_type, unsigned long long time, int accuracy)
{
	unsigned int cal_event_type = get_calibration_event_type(event_type);
	creg_event_info *event_info = NULL;
	creg_event_info *cal_event_info = NULL;
	client_callback_info* cal_callback_info = NULL;

	if (!cal_event_type)
		return NULL;

	cal_event_info = handle_info.get_reg_event_info(cal_event_type);
	if ((accuracy == SENSOR_ACCURACY_BAD) && !handle_info.m_bad_accuracy &&	cal_event_info) {
		sensor_event_data_t cal_event_data;
		sensor_data_t cal_data;
		void *cal_sensor_data;

		cal_event_info->m_previous_event_time = time;

		event_info = handle_info.get_reg_event_info(event_type);
		if (!event_info)
			return NULL;

		if (event_info->m_cb_type == SENSOR_LEGACY_CB) {
			cal_event_data.event_data = (void *)&(accuracy);
			cal_event_data.event_data_size = sizeof(accuracy);
			cal_sensor_data = &cal_event_data;
		} else {
			cal_data.accuracy = accuracy;
			cal_data.timestamp = time;
			cal_data.values[0] = accuracy;
			cal_data.value_count = 1;
			cal_sensor_data = &cal_data;
		}

		cal_callback_info = get_callback_info(handle_info.m_sensor_id, cal_event_info, cal_sensor_data);

		handle_info.m_bad_accuracy = true;

		print_event_occurrence_log(handle_info, cal_event_info);
	}

	if ((accuracy != SENSOR_ACCURACY_BAD) && handle_info.m_bad_accuracy)
		handle_info.m_bad_accuracy = false;

	return cal_callback_info;
}


void csensor_event_listener::handle_events(void* event)
{
	const unsigned int MS_TO_US = 1000;
	const float MIN_DELIVERY_DIFF_FACTOR = 0.75f;

	unsigned long long cur_time;
	creg_event_info *event_info = NULL;
	sensor_event_data_t event_data;
	sensor_id_t sensor_id;
	void *sensor_data;

	sensor_panning_data_t panning_data;
	int single_state_event_data = 0;

	int accuracy = SENSOR_ACCURACY_GOOD;

	unsigned int event_type = *((unsigned int *)(event));
	bool is_hub_event = is_sensorhub_event(event_type);

	client_callback_info* callback_info = NULL;
	vector<client_callback_info *> client_callback_infos;

	if (is_hub_event) {
		sensorhub_event_t *sensor_hub_event = (sensorhub_event_t *)event;
		sensor_id = sensor_hub_event->sensor_id;
		sensor_data = &(sensor_hub_event->data);
		cur_time = sensor_hub_event->data.timestamp;

		event_data.event_data = &(sensor_hub_event->data);
		event_data.event_data_size = sizeof(sensor_hub_event->data);
	} else {
		sensor_event_t *sensor_event = (sensor_event_t *)event;
		sensor_id = sensor_event->sensor_id;
		sensor_data = &(sensor_event->data);
		cur_time = sensor_event->data.timestamp;
		accuracy = sensor_event->data.accuracy;

		if (is_single_state_event(event_type)) {
			single_state_event_data = (int) sensor_event->data.values[0];
			event_data.event_data = (void *)&(single_state_event_data);
			event_data.event_data_size = sizeof(single_state_event_data);
		} else if (is_panning_event(event_type)) {
			panning_data.x = (int)sensor_event->data.values[0];
			panning_data.y = (int)sensor_event->data.values[1];
			event_data.event_data = (void *)&panning_data;
			event_data.event_data_size = sizeof(panning_data);
		} else {
			event_data.event_data = &(sensor_event->data);
			event_data.event_data_size = sizeof(sensor_event->data);
		}
	}

	{	/* scope for the lock */
		AUTOLOCK(m_handle_info_lock);

		for (auto it_handle = m_sensor_handle_infos.begin(); it_handle != m_sensor_handle_infos.end(); ++it_handle) {

			csensor_handle_info &sensor_handle_info = it_handle->second;

			event_info = sensor_handle_info.get_reg_event_info(event_type);
			if ((sensor_handle_info.m_sensor_id != sensor_id) ||
				(sensor_handle_info.m_sensor_state != SENSOR_STATE_STARTED) ||
				!event_info)
				continue;

			if (event_info->m_fired)
				continue;

			event_info->m_previous_event_time = cur_time;

			client_callback_info* cal_callback_info = handle_calibration_cb(sensor_handle_info, event_type, cur_time, accuracy);

			if (cal_callback_info)
				client_callback_infos.push_back(cal_callback_info);

			if (event_info->m_cb_type == SENSOR_LEGACY_CB)
				callback_info = get_callback_info(sensor_id, event_info, &event_data);
			else
				callback_info = get_callback_info(sensor_id, event_info, sensor_data);

			if (!callback_info) {
				ERR("Failed to get callback_info");
				continue;
			}

			if (sensor_handle_info.m_accuracy != accuracy) {
				sensor_handle_info.m_accuracy = accuracy;

				callback_info->accuracy_cb = sensor_handle_info.m_accuracy_cb;
				callback_info->timestamp = cur_time;
				callback_info->accuracy = accuracy;
				callback_info->accuracy_user_data = sensor_handle_info.m_accuracy_user_data;
			}

			client_callback_infos.push_back(callback_info);

			if (is_one_shot_event(event_type))
				event_info->m_fired = true;

			print_event_occurrence_log(sensor_handle_info, event_info);
		}
	}

	auto it_calback_info = client_callback_infos.begin();

	while (it_calback_info != client_callback_infos.end()) {
		post_callback_to_main_loop(*it_calback_info);
		++it_calback_info;
	}

}


client_callback_info* csensor_event_listener::get_callback_info(sensor_id_t sensor_id, const creg_event_info *event_info, void* sensor_data)
{
	client_callback_info* callback_info;

	callback_info = new(std::nothrow)client_callback_info;
	retvm_if (!callback_info, NULL, "Failed to allocate memory");

	callback_info->sensor = sensor_info_to_sensor(sensor_info_manager::get_instance().get_info(sensor_id));
	callback_info->event_id = event_info->m_id;
	callback_info->handle = event_info->m_handle;
	callback_info->cb_type = event_info->m_cb_type;
	callback_info->cb = event_info->m_cb;
	callback_info->event_type = event_info->type;
	callback_info->user_data = event_info->m_user_data;
	callback_info->accuracy_cb = NULL;
	callback_info->timestamp = 0;
	callback_info->accuracy = -1;
	callback_info->accuracy_user_data = NULL;

	if (event_info->m_cb_type == SENSOR_EVENT_CB) {
		callback_info->sensor_data = new(std::nothrow) char[sizeof(sensor_data_t)];

		if (!callback_info->sensor_data) {
			ERR("Failed to allocate memory");
			delete callback_info;
			return NULL;
		}

		copy_sensor_data((sensor_data_t*) callback_info->sensor_data, (sensor_data_t*) sensor_data);
	} else if (event_info->m_cb_type == SENSORHUB_EVENT_CB) {
		callback_info->sensor_data = new(std::nothrow) char[sizeof(sensorhub_data_t)];

		if (!callback_info->sensor_data) {
			ERR("Failed to allocate memory");
			delete callback_info;
			return NULL;
		}

		copy_sensorhub_data((sensorhub_data_t*) callback_info->sensor_data, (sensorhub_data_t*) sensor_data);
	} else if(event_info->m_cb_type == SENSOR_LEGACY_CB) {
		sensor_event_data_t *dest_sensor_data;
		sensor_event_data_t *src_sensor_data = (sensor_event_data_t *)sensor_data;
		callback_info->sensor_data = new(std::nothrow) char[sizeof(sensor_event_data_t)];

		if (!callback_info->sensor_data) {
			ERR("Failed to allocate memory");
			delete callback_info;
			return NULL;
		}

		dest_sensor_data = (sensor_event_data_t *) callback_info->sensor_data;
		dest_sensor_data->event_data_size = src_sensor_data->event_data_size;
		dest_sensor_data->event_data = new(std::nothrow) char[src_sensor_data->event_data_size];

		if (!dest_sensor_data->event_data) {
			ERR("Failed to allocate memory");
			delete[] (char *)(callback_info->sensor_data);
			delete callback_info;
			return NULL;
		}

		if (is_sensorhub_event(event_info->type))
			copy_sensorhub_data((sensorhub_data_t*)dest_sensor_data->event_data, (sensorhub_data_t*)src_sensor_data->event_data);
		else
			memcpy(dest_sensor_data->event_data, src_sensor_data->event_data, src_sensor_data->event_data_size);
	}

	return callback_info;
}

void csensor_event_listener::post_callback_to_main_loop(client_callback_info* cb_info)
{
	g_idle_add_full(G_PRIORITY_DEFAULT, callback_dispatcher, cb_info, NULL);
}


bool csensor_event_listener::is_event_active(int handle, unsigned int event_type, unsigned long long event_id)
{
	creg_event_info *event_info;

	AUTOLOCK(m_handle_info_lock);

	auto it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end())
		return false;

	event_info = it_handle->second.get_reg_event_info(event_type);
	if (!event_info)
		return false;

	if (event_info->m_id != event_id)
		return false;

	return true;
}


bool csensor_event_listener::is_valid_callback(client_callback_info *cb_info)
{
	return is_event_active(cb_info->handle, cb_info->event_type, cb_info->event_id);
}

gboolean csensor_event_listener::callback_dispatcher(gpointer data)
{
	client_callback_info *cb_info = (client_callback_info*) data;

	if (csensor_event_listener::get_instance().is_valid_callback(cb_info)) {
		if (cb_info->accuracy_cb)
			cb_info->accuracy_cb(cb_info->sensor, cb_info->timestamp, cb_info->accuracy, cb_info->accuracy_user_data);

		if (cb_info->cb_type == SENSOR_EVENT_CB)
			((sensor_cb_t) cb_info->cb)(cb_info->sensor, cb_info->event_type, (sensor_data_t *) cb_info->sensor_data, cb_info->user_data);
		else if (cb_info->cb_type == SENSORHUB_EVENT_CB)
			((sensorhub_cb_t) cb_info->cb)(cb_info->sensor, cb_info->event_type, (sensorhub_data_t *) cb_info->sensor_data, cb_info->user_data);
		else if (cb_info->cb_type == SENSOR_LEGACY_CB)
			((sensor_legacy_cb_t) cb_info->cb)(cb_info->event_type, (sensor_event_data_t *) cb_info->sensor_data, cb_info->user_data);
	} else {
		WARN("Discard invalid callback cb(0x%x)(%s, 0x%x, 0x%x) with id: %llu",
		cb_info->cb, get_event_name(cb_info->event_type), cb_info->sensor_data,
		cb_info->user_data, cb_info->event_id);
	}

	if (cb_info->cb_type == SENSOR_LEGACY_CB) {
		sensor_event_data_t *data = (sensor_event_data_t *) cb_info->sensor_data;
		delete[] (char *)data->event_data;
	}

	delete[] (char*)(cb_info->sensor_data);
	delete cb_info;

/*
* 	To be called only once, it returns false
*/
	return false;
}



bool csensor_event_listener::sensor_event_poll(void* buffer, int buffer_len, int &event)
{
	ssize_t len;

	len = m_event_socket.recv(buffer, buffer_len);

	if (!len) {
		if(!m_poller->poll(event))
			return false;
		len = m_event_socket.recv(buffer, buffer_len);

		if (!len) {
			INFO("%s failed to read after poll!", get_client_name());
			return false;
		}
	}

	if (len < 0) {
		INFO("%s failed to recv event from event socket", get_client_name());
		return false;
	}

	return true;
}



void csensor_event_listener::listen_events(void)
{
	sensorhub_event_t buffer;
	int event;

	do {
		lock l(m_thread_mutex);
		if (m_thread_state == THREAD_STATE_START) {
			if (!sensor_event_poll(&buffer, sizeof(buffer), event)) {
				INFO("sensor_event_poll failed");
				break;
			}

			handle_events(&buffer);
		} else {
			break;
		}
	} while (true);

	if (m_poller != NULL) {
		delete m_poller;
		m_poller = NULL;
	}

	close_event_channel();

	{ /* the scope for the lock */
		lock l(m_thread_mutex);
		m_thread_state = THREAD_STATE_TERMINATE;
		m_thread_cond.notify_one();
	}

	INFO("Event listener thread is terminated.");

	if (has_client_id() && (event & EPOLLHUP)) {
		if (m_hup_observer)
			m_hup_observer();
	}

}

bool csensor_event_listener::create_event_channel(void)
{
	int client_id;
	event_channel_ready_t event_channel_ready;

	if (!m_event_socket.create(SOCK_SEQPACKET))
		return false;

	if (!m_event_socket.connect(EVENT_CHANNEL_PATH)) {
		ERR("Failed to connect event channel for client %s, event socket fd[%d]", get_client_name(), m_event_socket.get_socket_fd());
		return false;
	}

	m_event_socket.set_connection_mode();

	client_id = get_client_id();

	if (m_event_socket.send(&client_id, sizeof(client_id)) <= 0) {
		ERR("Failed to send client id for client %s on event socket[%d]", get_client_name(), m_event_socket.get_socket_fd());
		return false;
	}

	if (m_event_socket.recv(&event_channel_ready, sizeof(event_channel_ready)) <= 0) {
		ERR("%s failed to recv event_channel_ready packet on event socket[%d] with client id [%d]",
			get_client_name(), m_event_socket.get_socket_fd(), client_id);
		return false;
	}

	if ((event_channel_ready.magic != EVENT_CHANNEL_MAGIC) || (event_channel_ready.client_id != client_id)) {
		ERR("Event_channel_ready packet is wrong, magic = 0x%x, client id = %d",
			event_channel_ready.magic, event_channel_ready.client_id);
		return false;
	}

	INFO("Event channel is established for client %s on socket[%d] with client id : %d",
		get_client_name(), m_event_socket.get_socket_fd(), client_id);

	return true;
}


void csensor_event_listener::close_event_channel(void)
{
	m_event_socket.close();
}


void csensor_event_listener::stop_event_listener(void)
{
	const int THREAD_TERMINATING_TIMEOUT = 2;

	ulock u(m_thread_mutex);

	if (m_thread_state != THREAD_STATE_TERMINATE) {
		m_thread_state = THREAD_STATE_STOP;

		_D("%s is waiting listener thread[state: %d] to be terminated", get_client_name(), m_thread_state);
		if (m_thread_cond.wait_for(u, std::chrono::seconds(THREAD_TERMINATING_TIMEOUT))
			== std::cv_status::timeout)
			_E("Fail to stop listener thread after waiting %d seconds", THREAD_TERMINATING_TIMEOUT);
		else
			_D("Listener thread for %s is terminated", get_client_name());
	}
}

void csensor_event_listener::set_thread_state(thread_state state)
{
	lock l(m_thread_mutex);
	m_thread_state = state;
}

void csensor_event_listener::clear(void)
{
	close_event_channel();
	stop_event_listener();
	close_command_channel();
	m_sensor_handle_infos.clear();
	set_client_id(CLIENT_ID_INVALID);
}


void csensor_event_listener::set_hup_observer(hup_observer_t observer)
{
	m_hup_observer = observer;
}

bool csensor_event_listener::start_event_listener(void)
{
	if (!create_event_channel()) {
		ERR("Event channel is not established for %s", get_client_name());
		return false;
	}

	m_event_socket.set_transfer_mode();

	m_poller = new(std::nothrow) poller(m_event_socket.get_socket_fd());
	retvm_if (!m_poller, false, "Failed to allocate memory");

	set_thread_state(THREAD_STATE_START);

	thread listener(&csensor_event_listener::listen_events, this);
	listener.detach();

	return true;
}

