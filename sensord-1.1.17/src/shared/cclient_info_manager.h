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

#ifndef CCLIENT_INFO_MANAGER_H_
#define CCLIENT_INFO_MANAGER_H_

#include <cclient_sensor_record.h>
#include <unordered_map>
#include <common.h>
#include <cmutex.h>
using std::unordered_map;

typedef unordered_map<int,cclient_sensor_record> client_id_sensor_record_map;
typedef vector<int> client_id_vec;


class cclient_info_manager {
public:
	static cclient_info_manager& get_instance();
	int create_client_record(void);
	bool remove_client_record(int client_id);
	bool has_client_record(int client_id);

	void set_client_info(int client_id, pid_t pid);
	const char* get_client_info(int client_id);

	bool set_permission(int client_id, int permission);
	bool get_permission(int client_id, int &permission);

	bool create_sensor_record(int client_id, sensor_id_t sensor_id);
	bool remove_sensor_record(int client_id, sensor_id_t sensor_id);
	bool has_sensor_record(int client_id, sensor_id_t sensor_id);
	bool has_sensor_record(int client_id);

	bool register_event(int client_id, sensor_id_t sensor_id, unsigned int event_type);
	bool unregister_event(int client_id, sensor_id_t sensor_id, unsigned int event_type);

	bool set_interval(int client_id, sensor_id_t sensor_id, unsigned int interval);
	unsigned int get_interval(int client_id, sensor_id_t sensor_id);
	bool set_option(int client_id, sensor_id_t sensor_id, int option);

	bool set_start(int client_id, sensor_id_t sensor_id, bool start);
	bool is_started(int client_id, sensor_id_t sensor_id);

	bool get_registered_events(int client_id, sensor_id_t sensor_id, event_type_vector &event_vec);

	bool get_listener_ids(sensor_id_t sensor_id, unsigned int event_type, client_id_vec &id_vec);
	bool get_event_socket(int client_id, csocket &sock);
	bool set_event_socket(int client_id, const csocket &sock);
private:
	client_id_sensor_record_map m_clients;
	cmutex m_mutex;

	cclient_info_manager();
	~cclient_info_manager();
	cclient_info_manager(cclient_info_manager const&) {};
	cclient_info_manager& operator=(cclient_info_manager const&);
};

#endif /* CCLIENT_INFO_MANAGER_H_ */
