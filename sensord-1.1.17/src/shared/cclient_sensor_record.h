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

#ifndef CCLIENT_SENSOR_RECORD_H_
#define CCLIENT_SENSOR_RECORD_H_

#include <sensor_internal.h>
#include <sf_common.h>
#include <csensor_usage.h>
#include <csocket.h>
#include <unordered_map>

using std::unordered_map;

typedef unordered_map<sensor_id_t, csensor_usage> sensor_usage_map;

class cclient_sensor_record {
public:
	cclient_sensor_record();
	~cclient_sensor_record();

	void set_client_id(int client_id);

	void set_client_info(pid_t pid);
	const char* get_client_info(void);

	void set_permission(int permission);
	int get_permission(void);

	bool register_event(sensor_id_t sensor_id, unsigned int event_type);
	bool unregister_event(sensor_id_t sensor_id, unsigned int event_type);

	bool set_interval(sensor_id_t sensor_id, unsigned int interval);
	unsigned int get_interval(sensor_id_t sensor_id);
	bool set_option(sensor_id_t sensor_id, int option);

	bool set_start(sensor_id_t sensor_id, bool start);
	bool is_started(sensor_id_t sensor_id);

	bool is_listening_event(sensor_id_t sensor_id, unsigned int event_type);
	bool has_sensor_usage(void);
	bool has_sensor_usage(sensor_id_t sensor_id);

	bool get_registered_events(sensor_id_t sensor_id, event_type_vector &event_vec);

	bool add_sensor_usage(sensor_id_t sensor_id);
	bool remove_sensor_usage(sensor_id_t sensor_id);

	void set_event_socket(const csocket &socket);
	void get_event_socket(csocket &socket);
	bool close_event_socket(void);

private:
	int m_client_id;
	pid_t m_pid;
	int m_permission;
	string m_client_info;
	csocket m_event_socket;
	sensor_usage_map m_sensor_usages;
};

#endif /* CCLIENT_SENSOR_RECORD_H_ */
