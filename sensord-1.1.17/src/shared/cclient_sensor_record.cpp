/*
 * libsensord-share
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

#include <cclient_sensor_record.h>
#include <common.h>

using std::pair;

cclient_sensor_record::cclient_sensor_record()
: m_client_id(0)
, m_pid(-1)
, m_permission(SENSOR_PERMISSION_NONE)
{

}

cclient_sensor_record::~cclient_sensor_record()
{
	m_sensor_usages.clear();
	close_event_socket();
}

bool cclient_sensor_record::register_event(sensor_id_t sensor_id, unsigned int event_type)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end()) {
		csensor_usage usage;
		usage.register_event(event_type);
		m_sensor_usages.insert(pair<sensor_id_t,csensor_usage>(sensor_id, usage));
		return true;
	}

	if (!it_usage->second.register_event(event_type)) {
		ERR("Event[0x%x] is already registered", event_type);
		return false;
	}

	return true;
}

bool cclient_sensor_record::unregister_event(sensor_id_t sensor_id, unsigned int event_type)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end()) {
		ERR("Sensor[0x%x] is not registered", sensor_id);
		return false;
	}

	if (!it_usage->second.unregister_event(event_type)) {
		ERR("Event[0x%x] is already registered", event_type);
		return false;
	}

	return true;
}

bool cclient_sensor_record::set_interval(sensor_id_t sensor_id, unsigned int interval)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end()) {
		csensor_usage usage;
		usage.m_interval = interval;
		m_sensor_usages.insert(pair<sensor_id_t, csensor_usage>(sensor_id, usage));
	} else {
		it_usage->second.m_interval = interval;
	}

	return true;
}

bool cclient_sensor_record::set_option(sensor_id_t sensor_id, int option)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end()) {
		csensor_usage usage;
		usage.m_option = option;
		m_sensor_usages.insert(pair<sensor_id_t, csensor_usage>(sensor_id, usage));
	} else {
		it_usage->second.m_option = option;
	}

	return true;
}


bool cclient_sensor_record::set_start(sensor_id_t sensor_id, bool start)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end()) {
		csensor_usage usage;
		usage.m_start = start;
		m_sensor_usages.insert(pair<sensor_id_t, csensor_usage>(sensor_id, usage));
	} else {
		it_usage->second.m_start = start;
	}

	return true;
}

bool cclient_sensor_record::is_started(sensor_id_t sensor_id)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end())
		return false;

	return it_usage->second.m_start;
}



unsigned int cclient_sensor_record::get_interval(sensor_id_t sensor_id)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end()) {
		ERR("Sensor[0x%x] is not found", sensor_id);
		return 0;
	}

	return it_usage->second.m_interval;
}

bool cclient_sensor_record::is_listening_event(sensor_id_t sensor_id, unsigned int event_type)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end())
		return false;

	if (it_usage->second.is_event_registered(event_type))
		return true;

	return false;
}

bool cclient_sensor_record::add_sensor_usage(sensor_id_t sensor_id)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage != m_sensor_usages.end()) {
		ERR("Sensor[0x%x] is already registered", sensor_id);
		return false;
	}

	csensor_usage usage;
	m_sensor_usages.insert(pair<sensor_id_t,csensor_usage> (sensor_id, usage));
	return true;
}

bool cclient_sensor_record::remove_sensor_usage(sensor_id_t sensor_id)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end()) {
		ERR("Sensor[0x%x] is not found", sensor_id);
		return false;
	}
	m_sensor_usages.erase(it_usage);
	return true;
}

bool cclient_sensor_record::has_sensor_usage(void)
{
	if (m_sensor_usages.empty())
		return false;

	return true;
}


bool cclient_sensor_record::has_sensor_usage(sensor_id_t sensor_id)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end()) {
		DBG("Sensor[0x%x] is not found", sensor_id);
		return false;
	}

	return true;
}


bool cclient_sensor_record::get_registered_events(sensor_id_t sensor_id, event_type_vector &event_vec)
{
	auto it_usage = m_sensor_usages.find(sensor_id);

	if (it_usage == m_sensor_usages.end()) {
		DBG("Sensor[0x%x] is not found", sensor_id);
		return false;
	}

	copy(it_usage->second.m_reg_events.begin(), it_usage->second.m_reg_events.end(), back_inserter(event_vec));

	return true;

}

void cclient_sensor_record::set_client_id(int client_id)
{
	m_client_id = client_id;
}

void cclient_sensor_record::set_client_info(pid_t pid)
{
	char client_info[NAME_MAX + 32];
	char proc_name[NAME_MAX];

	m_pid = pid;
	get_proc_name(pid, proc_name);

	snprintf(client_info, sizeof(client_info), "%s[pid=%d, id=%d]", proc_name, m_pid, m_client_id);
	m_client_info.assign(client_info);

}

const char* cclient_sensor_record::get_client_info(void)
{
	return m_client_info.c_str();
}

void cclient_sensor_record::set_permission(int permission)
{
	m_permission = permission;
}

int cclient_sensor_record::get_permission(void)
{
	return  m_permission;
}


void cclient_sensor_record::set_event_socket(const csocket &socket)
{
	m_event_socket = socket;
}

void cclient_sensor_record::get_event_socket(csocket &socket)
{
	socket = m_event_socket;
}


bool cclient_sensor_record::close_event_socket(void)
{
	return m_event_socket.close();
}


