/*
 * context_sensor_hal
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

#ifndef _CONTEXT_SENSOR_HAL_H_
#define _CONTEXT_SENSOR_HAL_H_

#include <sensor_hal.h>
#include <string>

using std::string;

class context_sensor_hal : public sensor_hal
{
public:
	context_sensor_hal();
	virtual ~context_sensor_hal();
	string get_model_id(void);
	sensor_type_t get_type(void);
	bool enable(void);
	bool disable(void);
	bool is_data_ready(bool wait);
	virtual int get_sensor_data(sensorhub_data_t &data);
	bool get_properties(sensor_properties_t &properties);
	int send_sensorhub_data(const char* data, int data_len);
private:
	sensorhub_data_t m_pending_data;
	sensorhub_data_t m_data;

	int m_node_handle;
	int m_context_fd;

	bool m_enabled;

	cmutex m_mutex;
	cmutex m_data_mutex;

	bool update_value(bool wait);

	int open_input_node(const char* input_node);

	int print_context_data(const char* name, const char *data, int length);
	int send_context_data(const char *data, int data_len);
	int read_context_data(void);
	int read_large_context_data(void);
};
#endif /*_CONTEXT_SENSOR_HAL_CLASS_H_*/
