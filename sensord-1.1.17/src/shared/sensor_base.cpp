/*
 * libsensord-share
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

#include <sensor_base.h>

#include <algorithm>

#define UNKNOWN_NAME "UNKNOWN_SENSOR"

sensor_base::sensor_base()
: m_privilege(SENSOR_PRIVILEGE_PUBLIC)
, m_permission(SENSOR_PERMISSION_STANDARD)
, m_client(0)
, m_started(false)
{

}

sensor_base::~sensor_base()
{

}

bool sensor_base::init()
{
	return true;
}

bool sensor_base::is_virtual()
{
	return false;
}


bool sensor_base::is_fusion(void)
{
	return false;
}

void sensor_base::set_id(sensor_id_t id)
{
	m_id = id;
}

sensor_id_t sensor_base::get_id(void)
{
	return m_id;
}

sensor_privilege_t sensor_base::get_privilege(void)
{
	return m_privilege;
}

int sensor_base::get_permission(void)
{
	return m_permission;
}


void sensor_base::set_privilege(sensor_privilege_t privilege)
{
	m_privilege = privilege;
}

void sensor_base::set_permission(int permission)
{
	m_permission = permission;
}


sensor_type_t sensor_base::get_type()
{
	return UNKNOWN_SENSOR;
}

const char* sensor_base::get_name()
{
	if (m_name.empty())
		return UNKNOWN_NAME;

	return m_name.c_str();
}

bool sensor_base::on_start()
{
	return true;
}

bool sensor_base::on_stop()
{
	return true;
}

bool sensor_base::start()
{
	AUTOLOCK(m_mutex);
	AUTOLOCK(m_client_mutex);

	++m_client;

	if (m_client == 1) {
		if (!on_start()) {
			ERR("[%s] sensor failed to start", get_name());
			return false;
		}

		m_started = true;
	}

	INFO("[%s] sensor started, #client = %d", get_name(), m_client);

	return true;
}

bool sensor_base::stop(void)
{
	AUTOLOCK(m_mutex);
	AUTOLOCK(m_client_mutex);

	--m_client;

	if (m_client == 0) {
		if (!on_stop()) {
			ERR("[%s] sensor faild to stop", get_name());
			return false;
		}

		m_started = false;
	}

	INFO("[%s] sensor stopped, #client = %d", get_name(), m_client);

	return true;
}


bool sensor_base::is_started(void)
{
	AUTOLOCK(m_mutex);
	AUTOLOCK(m_client_mutex);

	return m_started;
}

bool sensor_base::add_client(unsigned int event_type)
{
	if (!is_supported(event_type)) {
		ERR("Invaild event type: 0x%x", event_type);
		return false;
	}

	AUTOLOCK(m_client_info_mutex);

	++(m_client_info[event_type]);
	return true;
}

bool sensor_base::delete_client(unsigned int event_type)
{
	if (!is_supported(event_type)) {
		ERR("Invaild event type: 0x%x", event_type);
		return false;
	}

	AUTOLOCK(m_client_info_mutex);

	auto iter = m_client_info.find(event_type);

	if (iter == m_client_info.end())
		return false;

	if (iter->second == 0)
		return false;

	--(iter->second);

	return true;
}

bool sensor_base::add_interval(int client_id, unsigned int interval, bool is_processor)
{
	unsigned int prev_min, cur_min;

	AUTOLOCK(m_interval_info_list_mutex);

	prev_min = m_interval_info_list.get_min();

	if (!m_interval_info_list.add_interval(client_id, interval, is_processor))
		return false;

	cur_min = m_interval_info_list.get_min();

	if (cur_min != prev_min) {
		INFO("Min interval for sensor[0x%x] is changed from %dms to %dms"
			" by%sclient[%d] adding interval",
			get_type(), prev_min, cur_min,
			is_processor ? " processor " : " ", client_id);
		set_interval(cur_min);
	}

	return true;
}

bool sensor_base::delete_interval(int client_id, bool is_processor)
{
	unsigned int prev_min, cur_min;
	AUTOLOCK(m_interval_info_list_mutex);

	prev_min = m_interval_info_list.get_min();

	if (!m_interval_info_list.delete_interval(client_id, is_processor))
		return false;

	cur_min = m_interval_info_list.get_min();

	if (!cur_min) {
		INFO("No interval for sensor[0x%x] by%sclient[%d] deleting interval, "
			 "so set to default %dms",
			 get_type(), is_processor ? " processor " : " ",
			 client_id, POLL_1HZ_MS);

		set_interval(POLL_1HZ_MS);
	} else if (cur_min != prev_min) {
		INFO("Min interval for sensor[0x%x] is changed from %dms to %dms"
			" by%sclient[%d] deleting interval",
			get_type(), prev_min, cur_min,
			is_processor ? " processor " : " ", client_id);

		set_interval(cur_min);
	}

	return true;
}

unsigned int sensor_base::get_interval(int client_id, bool is_processor)
{
	AUTOLOCK(m_interval_info_list_mutex);

	return m_interval_info_list.get_interval(client_id, is_processor);
}

void sensor_base::get_sensor_info(sensor_info &info)
{
	sensor_properties_t properties;
	get_properties(properties);

	info.set_type(get_type());
	info.set_id(get_id());
	info.set_privilege(m_privilege);
	info.set_name(properties.name.c_str());
	info.set_vendor(properties.vendor.c_str());
	info.set_min_range(properties.min_range);
	info.set_max_range(properties.max_range);
	info.set_resolution(properties.resolution);
	info.set_min_interval(properties.min_interval);
	info.set_fifo_count(properties.fifo_count);
	info.set_max_batch_count(properties.max_batch_count);
	info.set_supported_events(m_supported_event_info);

	return;
}

bool sensor_base::get_properties(sensor_properties_t &properties)
{
	return false;
}

bool sensor_base::is_supported(unsigned int event_type)
{
	auto iter = find(m_supported_event_info.begin(), m_supported_event_info.end(), event_type);

	if (iter == m_supported_event_info.end())
		return false;

	return true;
}

long sensor_base::set_command(unsigned int cmd, long value)
{
	return -1;
}

int sensor_base::send_sensorhub_data(const char* data, int data_len)
{
	return -1;
}

int sensor_base::get_sensor_data(unsigned int type, sensor_data_t &data)
{
	return -1;
}

void sensor_base::register_supported_event(unsigned int event_type)
{
	m_supported_event_info.push_back(event_type);
}

unsigned int sensor_base::get_client_cnt(unsigned int event_type)
{
	AUTOLOCK(m_client_info_mutex);

	auto iter = m_client_info.find(event_type);

	if (iter == m_client_info.end())
		return 0;

	return iter->second;
}

bool sensor_base::set_interval(unsigned long val)
{
	return true;
}

unsigned long long sensor_base::get_timestamp(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return ((unsigned long long)(t.tv_sec)*1000000000LL + t.tv_nsec) / 1000;
}

unsigned long long sensor_base::get_timestamp(timeval *t)
{
	if (!t) {
		ERR("t is NULL");
		return 0;
	}

	return ((unsigned long long)(t->tv_sec)*1000000LL +t->tv_usec);
}
