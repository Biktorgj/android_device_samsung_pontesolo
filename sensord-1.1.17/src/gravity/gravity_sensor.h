/*
 * sensord
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

#ifndef _GRAVITY_SENSOR_H_
#define _GRAVITY_SENSOR_H_

#include <sensor_internal.h>
#include <virtual_sensor.h>

class gravity_sensor : public virtual_sensor {
public:
	gravity_sensor();
	virtual ~gravity_sensor();

	bool init();
	sensor_type_t get_type(void);

	void synthesize(const sensor_event_t &event, vector<sensor_event_t> &outs);

	bool add_interval(int client_id, unsigned int interval, bool is_processor);
	bool delete_interval(int client_id, bool is_processor);

	int get_sensor_data(unsigned int data_id, sensor_data_t &data);
	bool get_properties(sensor_properties_t &properties);
private:
	sensor_base *m_orientation_sensor;
	cmutex m_value_mutex;

	float m_x;
	float m_y;
	float m_z;
	int m_accuracy;
	unsigned long long m_time;
	unsigned int m_interval;

	string m_vendor;
	string m_raw_data_unit;
	string m_orientation_data_unit;
	int m_default_sampling_time;
	int m_gravity_sign_compensation[3];

	bool on_start(void);
	bool on_stop(void);
};

#endif /* _GRAVITY_SENSOR_H_ */
