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

#ifndef CSENSOR_EVENT_LISTENER_H_
#define CSENSOR_EVENT_LISTENER_H_

#include <glib.h>
#include <sys/types.h>
#include <csensor_handle_info.h>
#include <unistd.h>
#include <csocket.h>
#include <string.h>
#include <sf_common.h>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cmutex.h>
#include <poller.h>

using std::unordered_map;
using std::vector;
using std::string;
using std::queue;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::condition_variable;

typedef vector<unsigned int> handle_vector;
typedef vector<sensor_id_t> sensor_id_vector;
typedef unordered_map<int,csensor_handle_info> sensor_handle_info_map;
typedef unordered_map<sensor_id_t, command_channel*> sensor_command_channel_map;

typedef struct {
	unsigned long long event_id;
	int handle;
	sensor_t sensor;
	unsigned int event_type;
	int cb_type;
	void *cb;
	void *sensor_data;
	void *user_data;
	sensor_accuracy_changed_cb_t accuracy_cb;
	unsigned long long timestamp;
	int accuracy;
	void *accuracy_user_data;
} client_callback_info;

typedef struct sensor_rep
{
	bool active;
	int option;
	unsigned int interval;
	event_type_vector event_types;
} sensor_rep;

typedef void (*hup_observer_t)(void);

class csensor_event_listener {
public:
	static csensor_event_listener& get_instance(void);
	int create_handle(sensor_id_t sensor_id);
	bool delete_handle(int handle);
	bool start_handle(int handle);
	bool stop_handle(int handle);
	bool register_event(int handle, unsigned int event_type, unsigned int interval, int cb_type, void *cb, void* user_data);
	bool unregister_event(int handle, unsigned int event_type);
	bool change_event_interval(int handle, unsigned int event_type, unsigned int interval);

	bool register_accuracy_cb(int handle, sensor_accuracy_changed_cb_t cb, void* user_data);
	bool unregister_accuracy_cb(int handle);

	bool set_sensor_params(int handle, int sensor_state, int sensor_option);
	bool get_sensor_params(int handle, int &sensor_state, int &sensor_option);
	bool set_sensor_state(int handle, int sensor_state);
	bool set_sensor_option(int handle, int sensor_option);
	bool set_event_interval(int handle, unsigned int event_type, unsigned int interval);
	bool get_event_info(int handle, unsigned int event_type, unsigned int &interval, int &cb_type, void* &cb, void* &user_data);
	void operate_sensor(sensor_id_t sensor, int power_save_state);
	void get_listening_sensors(sensor_id_vector &sensors);

	unsigned int get_active_min_interval(sensor_id_t sensor_id);
	unsigned int get_active_option(sensor_id_t sensor_id);
	void get_active_event_types(sensor_id_t sensor_id, event_type_vector &active_event_types);

	bool get_sensor_id(int handle, sensor_id_t &sensor_id);
	bool get_sensor_state(int handle, int &state);

	void get_sensor_rep(sensor_id_t sensor_id, sensor_rep& rep);

	bool has_client_id(void);
	int get_client_id(void);
	void set_client_id(int client_id);

	bool is_active(void);
	bool is_sensor_registered(sensor_id_t sensor_id);
	bool is_sensor_active(sensor_id_t sensor_id);

	bool add_command_channel(sensor_id_t sensor_id, command_channel *cmd_channel);
	bool get_command_channel(sensor_id_t sensor_id, command_channel **cmd_channel);
	bool close_command_channel(void);
	bool close_command_channel(sensor_id_t sensor_id);

	void get_all_handles(handle_vector &handles);

	bool start_event_listener(void);
	void stop_event_listener(void);
	void clear(void);

	void set_hup_observer(hup_observer_t observer);
private:
	enum thread_state {
		THREAD_STATE_START,
		THREAD_STATE_STOP,
		THREAD_STATE_TERMINATE,
	};
	typedef lock_guard<mutex> lock;
	typedef unique_lock<mutex> ulock;

	sensor_handle_info_map m_sensor_handle_infos;
	sensor_command_channel_map m_command_channels;

	int m_client_id;

	csocket m_event_socket;
	poller *m_poller;

	cmutex m_handle_info_lock;

	thread_state m_thread_state;
	mutex m_thread_mutex;
	condition_variable m_thread_cond;

	hup_observer_t m_hup_observer;

	csensor_event_listener();
	~csensor_event_listener();

	csensor_event_listener(const csensor_event_listener&) {};
	csensor_event_listener& operator=(const csensor_event_listener&);

	bool create_event_channel(void);
	void close_event_channel(void);

	bool sensor_event_poll(void* buffer, int buffer_len, int &event);

	void listen_events(void);
	client_callback_info* handle_calibration_cb(csensor_handle_info &handle_info, unsigned event_type, unsigned long long time, int accuracy);
	void handle_events(void* event);

	client_callback_info* get_callback_info(sensor_id_t sensor_id, const creg_event_info *event_info, void *sensor_data);

	unsigned long long renew_event_id(void);

	void post_callback_to_main_loop(client_callback_info *cb_info);

	bool is_event_active(int handle, unsigned int event_type, unsigned long long event_id);
	bool is_valid_callback(client_callback_info *cb_info);
	static gboolean callback_dispatcher(gpointer data);

	void set_thread_state(thread_state state);
};
#endif /* CSENSOR_EVENT_LISTENER_H_ */
