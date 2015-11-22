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

#ifndef _SENSOR_EVENT_DISPATCHER_H_
#define _SENSOR_EVENT_DISPATCHER_H_

#include <sf_common.h>
#include <csensor_event_queue.h>
#include <cclient_info_manager.h>
#include <sensor_fusion.h>
#include <csocket.h>
#include <virtual_sensor.h>
#include <vconf.h>

typedef unordered_map<unsigned int, sensor_event_t> event_type_last_event_map;
typedef list<virtual_sensor *> virtual_sensors;

class csensor_event_dispatcher
{
private:
	bool m_lcd_on;
	csocket m_accept_socket;
	cmutex m_mutex;
	cmutex m_last_events_mutex;
	event_type_last_event_map m_last_events;
	virtual_sensors m_active_virtual_sensors;
	cmutex m_active_virtual_sensors_mutex;
	sensor_fusion *m_sensor_fusion;

	csensor_event_dispatcher();
	~csensor_event_dispatcher();
	csensor_event_dispatcher(csensor_event_dispatcher const&) {};
	csensor_event_dispatcher& operator=(csensor_event_dispatcher const&);

	void accept_connections(void);
	void accept_event_channel(csocket client_socket);

	void dispatch_event(void);
	void send_sensor_events(void* events, int event_cnt, bool is_hub_event);
	static cclient_info_manager& get_client_info_manager(void);
	static csensor_event_queue& get_event_queue(void);

	bool is_record_event(unsigned int event_type);
	void put_last_event(unsigned int event_type, const sensor_event_t &event);
	bool get_last_event(unsigned int event_type, sensor_event_t &event);

	bool has_active_virtual_sensor(virtual_sensor *sensor);
	virtual_sensors get_active_virtual_sensors(void);

	void sort_sensor_events(sensor_event_t *events, unsigned int cnt);
public:
	static csensor_event_dispatcher& get_instance();
	bool run(void);
	void request_last_event(int client_id, sensor_id_t sensor_id);

	bool add_active_virtual_sensor(virtual_sensor *sensor);
	bool delete_active_virtual_sensor(virtual_sensor *sensor);
};

#endif
