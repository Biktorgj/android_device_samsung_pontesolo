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

#ifndef _COMMAND_CHANNEL_H_
#define _COMMAND_CHANNEL_H_
#include <sf_common.h>
#include <sensor_internal.h>
#include <cpacket.h>
#include <csocket.h>
#include <vector>

using std::vector;

class command_channel
{
public:

	command_channel();
	~command_channel();

	bool create_channel(void);
	void set_client_id(int client_id);

	bool cmd_get_id(int &client_id);
	bool cmd_get_sensor_list(void);
	bool cmd_hello(sensor_id_t sensor);
	bool cmd_byebye(void);
	bool cmd_start(void);
	bool cmd_stop(void);
	bool cmd_set_option(int option);
	bool cmd_register_event(unsigned int event_type);
	bool cmd_register_events(event_type_vector &event_vec);
	bool cmd_unregister_event(unsigned int event_type);
	bool cmd_unregister_events(event_type_vector &event_vec);
	bool cmd_set_interval(unsigned int interval);
	bool cmd_unset_interval(void);
	bool cmd_set_command(unsigned int cmd, long value);
	bool cmd_get_data(unsigned int type, sensor_data_t* values);
	bool cmd_send_sensorhub_data(const char* buffer, int data_len);
private:
	csocket m_command_socket;
	int m_client_id;
	sensor_id_t m_sensor_id;
	bool command_handler(cpacket *packet, void **return_payload);
};

#endif /* COMMAND_CHANNEL_H_ */
