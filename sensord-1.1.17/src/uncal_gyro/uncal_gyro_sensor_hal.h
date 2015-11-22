/*
 * uncal_gyro_sensor_hal
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

#ifndef UNCAL_GYRO_SENSOR_HAL_H_
#define UNCAL_GYRO_SENSOR_HAL_H_

#include <sensor_hal.h>
#include <string>

using std::string;

class uncal_gyro_sensor_hal : public sensor_hal
{
public:
	uncal_gyro_sensor_hal();
	virtual ~uncal_gyro_sensor_hal();
	string get_model_id(void);
	sensor_type_t get_type(void);
	bool enable(void);
	bool disable(void);
	bool set_interval(unsigned long val);
	bool is_data_ready(bool wait);
	virtual int get_sensor_data(sensor_data_t &data);
	virtual bool get_properties(sensor_properties_t &properties);

private:
	int m_x;
	int m_y;
	int m_z;
	int m_x_offset;
	int m_y_offset;
	int m_z_offset;

	string m_model_id;
	string m_vendor;
	string m_chip_name;

	int m_node_handle;
	unsigned long m_polling_interval;
	unsigned long long m_fired_time;

	float m_min_range;
	float m_max_range;
	int m_resolution;
	float m_raw_data_unit;

	string m_data_node;
	string m_enable_node;
	string m_interval_node;

	bool m_sensorhub_controlled;

	cmutex m_value_mutex;

	bool update_value(bool wait);

};
#endif /*_UNCAL_GYRO_SENSOR_HAL_CLASS_H_*/
