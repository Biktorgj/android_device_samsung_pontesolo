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

#include <sensor_info.h>
#include <common.h>
#include <algorithm>

sensor_type_t sensor_info::get_type(void)
{
	return m_type;
}

sensor_id_t sensor_info::get_id(void)
{
	return m_id;
}

sensor_privilege_t sensor_info::get_privilege(void)
{
	return m_privilege;
}

const char* sensor_info::get_name(void)
{
	return m_name.c_str();
}

const char* sensor_info::get_vendor(void)
{
	return m_vendor.c_str();
}

float sensor_info::get_min_range(void)
{
	return m_min_range;
}

float sensor_info::get_max_range(void)
{
	return m_max_range;
}

float sensor_info::get_resolution(void)
{
	return m_resolution;
}

int sensor_info::get_min_interval(void)
{
	return m_min_interval;
}

int sensor_info::get_fifo_count(void)
{
	return m_fifo_count;
}

int sensor_info::get_max_batch_count(void)
{
	return m_max_batch_count;
}

void sensor_info::get_supported_events(vector<unsigned int> &events)
{
	events = m_supported_events;
}

bool sensor_info::is_supported_event(unsigned int event)
{
	auto iter = find(m_supported_events.begin(), m_supported_events.end(), event);

	if (iter == m_supported_events.end())
		return false;

	return true;
}

void sensor_info::set_type(sensor_type_t type)
{
	m_type = type;
}

void sensor_info::set_id(sensor_id_t id)
{
	m_id = id;
}

void sensor_info::set_privilege(sensor_privilege_t privilege)
{
	m_privilege = privilege;
}

void sensor_info::set_name(const char *name)
{
	m_name = name;
}

void sensor_info::set_vendor(const char *vendor)
{
	m_vendor = vendor;
}

void sensor_info::set_min_range(float min_range)
{
	m_min_range = min_range;
}

void sensor_info::set_max_range(float max_range)
{
	m_max_range = max_range;
}

void sensor_info::set_resolution(float resolution)
{
	m_resolution = resolution;
}

void sensor_info::set_min_interval(int min_interval)
{
	m_min_interval = min_interval;
}

void sensor_info::set_fifo_count(int fifo_count)
{
	m_fifo_count = fifo_count;
}

void sensor_info::set_max_batch_count(int max_batch_count)
{
	m_max_batch_count = max_batch_count;
}

void sensor_info::register_supported_event(unsigned int event)
{
	m_supported_events.push_back(event);
}

void sensor_info::set_supported_events(vector<unsigned int> &events)
{
	copy(events.begin(), events.end(), back_inserter(m_supported_events));
}

void sensor_info::get_raw_data(raw_data_t &data)
{
	put(data, (int)m_type);
	put(data, (int) m_id);
	put(data, (int) m_privilege);
	put(data, m_name);
	put(data, m_vendor);
	put(data, m_min_range);
	put(data, m_max_range);
	put(data, m_resolution);
	put(data, m_min_interval);
	put(data, m_fifo_count);
	put(data, m_max_batch_count);
	put(data, m_supported_events);
}

void sensor_info::set_raw_data(const char *data, int data_len)
{
	raw_data_t raw_data(&data[0], &data[data_len]);

	auto it_r_data = raw_data.begin();

	int type, id, privilege;

	it_r_data = get(it_r_data, type);
	m_type = (sensor_type_t) type;
	it_r_data = get(it_r_data, id);
	m_id = (sensor_id_t) id;
	it_r_data = get(it_r_data, privilege);
	m_privilege = (sensor_privilege_t) privilege;
	it_r_data = get(it_r_data, m_name);
	it_r_data = get(it_r_data, m_vendor);
	it_r_data = get(it_r_data, m_min_range);
	it_r_data = get(it_r_data, m_max_range);
	it_r_data = get(it_r_data, m_resolution);
	it_r_data = get(it_r_data, m_min_interval);
	it_r_data = get(it_r_data, m_fifo_count);
	it_r_data = get(it_r_data, m_max_batch_count);
	it_r_data = get(it_r_data, m_supported_events);
}

void sensor_info::show(void)
{
	INFO("Type = %d", m_type);
	INFO("ID = 0x%x", (int)m_id);
	INFO("Privilege = %d", (int)m_privilege);
	INFO("Name = %s", m_name.c_str());
	INFO("Vendor = %s", m_vendor.c_str());
	INFO("Min_range = %f", m_min_range);
	INFO("Max_range = %f", m_max_range);
	INFO("Resolution = %f", m_resolution);
	INFO("Min_interval = %d", m_min_interval);
	INFO("Fifo_count = %d", m_fifo_count);
	INFO("Max_batch_count = %d", m_max_batch_count);

	for (int i = 0; i < m_supported_events.size(); ++i)
		INFO("supported_events[%d] = 0x%x", i, m_supported_events[i]);
}


void sensor_info::clear(void)
{
	m_type = UNKNOWN_SENSOR;
	m_id = -1;
	m_privilege = SENSOR_PRIVILEGE_PUBLIC;
	m_name.clear();
	m_vendor.clear();
	m_min_range = 0.0f;
	m_max_range = 0.0f;
	m_resolution = 0.0f;
	m_min_interval = 0;
	m_fifo_count = 0;
	m_max_batch_count = 0;
	m_supported_events.clear();
}


void sensor_info::put(raw_data_t &data, int value)
{
	char buffer[sizeof(value)];

	(*(int *) buffer) = value;

	copy(&buffer[0], &buffer[sizeof(buffer)], back_inserter(data));
}

void sensor_info::put(raw_data_t &data, float value)
{
	char buffer[sizeof(value)];

	(*(float *) buffer) = value;

	copy(&buffer[0], &buffer[sizeof(buffer)], back_inserter(data));
}

void sensor_info::put(raw_data_t &data, string &value)
{
	put(data, (int) value.size());

	copy(value.begin(), value.end(), back_inserter(data));
}

void sensor_info::put(raw_data_t &data, vector<unsigned int> &value)
{
	put(data, (int) value.size());

	auto it = value.begin();

	while (it != value.end()) {
		put(data, (int) *it);
		++it;
	}
}

raw_data_iterator sensor_info::get(raw_data_iterator it, int &value)
{
	copy(it, it + sizeof(value), (char*) &value);

	return it + sizeof(value);
}

raw_data_iterator sensor_info::get(raw_data_iterator it, float &value)
{
	copy(it, it + sizeof(value), (char*) &value);

	return it + sizeof(value);
}

raw_data_iterator sensor_info::get(raw_data_iterator it, string &value)
{
	int len;

	it = get(it, len);

	copy(it, it + len, back_inserter(value));

	return it + len;
}

raw_data_iterator sensor_info::get(raw_data_iterator it, vector<unsigned int> &value)
{
	int len;

	it = get(it, len);

	int ele;
	for (int i = 0; i < len; ++i) {
		it = get(it, ele);
		value.push_back(ele);
	}

	return it;
}
