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

#ifndef _ORIENTATION_SENSOR_H_
#define _ORIENTATION_SENSOR_H_

#include <sensor_internal.h>
#include <virtual_sensor.h>
#include <orientation_filter.h>

class orientation_sensor : public virtual_sensor {
public:
	orientation_sensor();
	virtual ~orientation_sensor();

	bool init(void);

	void synthesize(const sensor_event_t &event, vector<sensor_event_t> &outs);

	bool add_interval(int client_id, unsigned int interval, bool is_processor);
	bool delete_interval(int client_id, bool is_processor);
	bool get_properties(sensor_properties_t &properties);
	sensor_type_t get_type(void);

	int get_sensor_data(const unsigned int data_id, sensor_data_t &data);

private:
	sensor_base *m_accel_sensor;
	sensor_base *m_gyro_sensor;
	sensor_base *m_magnetic_sensor;

	sensor_data<float> m_accel;
	sensor_data<float> m_gyro;
	sensor_data<float> m_magnetic;

	cmutex m_value_mutex;

	orientation_filter<float> m_orientation;

	unsigned int m_enable_orientation;

	float m_roll;
	float m_pitch;
	float m_azimuth;
	unsigned long long m_time;
	unsigned int m_interval;

	string m_vendor;
	string m_raw_data_unit;
	int m_default_sampling_time;
	float m_accel_static_bias[3];
	float m_gyro_static_bias[3];
	float m_geomagnetic_static_bias[3];
	int m_accel_rotation_direction_compensation[3];
	int m_gyro_rotation_direction_compensation[3];
	int m_geomagnetic_rotation_direction_compensation[3];
	float m_accel_scale;
	float m_gyro_scale;
	float m_geomagnetic_scale;
	int m_magnetic_alignment_factor;
	int m_azimuth_rotation_compensation;
	int m_pitch_rotation_compensation;
	int m_roll_rotation_compensation;

	bool on_start(void);
	bool on_stop(void);
};

#endif
