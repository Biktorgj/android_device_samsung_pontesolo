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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <common.h>
#include <sf_common.h>
#include <orientation_sensor.h>
#include <sensor_plugin_loader.h>
#include <orientation_filter.h>
#include <cvirtual_sensor_config.h>

#define SENSOR_NAME "ORIENTATION_SENSOR"
#define SENSOR_TYPE_ORIENTATION		"ORIENTATION"

#define ACCELEROMETER_ENABLED 0x01
#define GYROSCOPE_ENABLED 0x02
#define GEOMAGNETIC_ENABLED 0x04
#define ORIENTATION_ENABLED 7

#define INITIAL_VALUE -1

#define MS_TO_US 1000

#define PI 3.141593
#define AZIMUTH_OFFSET_DEGREES 360
#define AZIMUTH_OFFSET_RADIANS (2 * PI)

#define ELEMENT_NAME											"NAME"
#define ELEMENT_VENDOR											"VENDOR"
#define ELEMENT_RAW_DATA_UNIT									"RAW_DATA_UNIT"
#define ELEMENT_DEFAULT_SAMPLING_TIME							"DEFAULT_SAMPLING_TIME"
#define ELEMENT_ACCEL_STATIC_BIAS								"ACCEL_STATIC_BIAS"
#define ELEMENT_GYRO_STATIC_BIAS								"GYRO_STATIC_BIAS"
#define ELEMENT_GEOMAGNETIC_STATIC_BIAS							"GEOMAGNETIC_STATIC_BIAS"
#define ELEMENT_ACCEL_ROTATION_DIRECTION_COMPENSATION			"ACCEL_ROTATION_DIRECTION_COMPENSATION"
#define ELEMENT_GYRO_ROTATION_DIRECTION_COMPENSATION			"GYRO_ROTATION_DIRECTION_COMPENSATION"
#define ELEMENT_GEOMAGNETIC_ROTATION_DIRECTION_COMPENSATION		"GEOMAGNETIC_ROTATION_DIRECTION_COMPENSATION"
#define ELEMENT_ACCEL_SCALE										"ACCEL_SCALE"
#define ELEMENT_GYRO_SCALE										"GYRO_SCALE"
#define ELEMENT_GEOMAGNETIC_SCALE								"GEOMAGNETIC_SCALE"
#define ELEMENT_MAGNETIC_ALIGNMENT_FACTOR						"MAGNETIC_ALIGNMENT_FACTOR"
#define ELEMENT_PITCH_ROTATION_COMPENSATION						"PITCH_ROTATION_COMPENSATION"
#define ELEMENT_ROLL_ROTATION_COMPENSATION						"ROLL_ROTATION_COMPENSATION"
#define ELEMENT_AZIMUTH_ROTATION_COMPENSATION					"AZIMUTH_ROTATION_COMPENSATION"

void pre_process_data(sensor_data<float> &data_out, const float *data_in, float *bias, int *sign, float scale)
{
	data_out.m_data.m_vec[0] = sign[0] * (data_in[0] - bias[0]) / scale;
	data_out.m_data.m_vec[1] = sign[1] * (data_in[1] - bias[1]) / scale;
	data_out.m_data.m_vec[2] = sign[2] * (data_in[2] - bias[2]) / scale;
}

orientation_sensor::orientation_sensor()
: m_accel_sensor(NULL)
, m_gyro_sensor(NULL)
, m_magnetic_sensor(NULL)
, m_roll(INITIAL_VALUE)
, m_pitch(INITIAL_VALUE)
, m_azimuth(INITIAL_VALUE)
, m_time(0)
{
	cvirtual_sensor_config &config = cvirtual_sensor_config::get_instance();

	m_name = string(SENSOR_NAME);
	register_supported_event(ORIENTATION_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_enable_orientation = 0;
	m_time = get_timestamp();

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_RAW_DATA_UNIT, m_raw_data_unit)) {
		ERR("[RAW_DATA_UNIT] is empty\n");
		throw ENXIO;
	}

	INFO("m_raw_data_unit = %s", m_raw_data_unit.c_str());

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_DEFAULT_SAMPLING_TIME, &m_default_sampling_time)) {
		ERR("[DEFAULT_SAMPLING_TIME] is empty\n");
		throw ENXIO;
	}

	INFO("m_default_sampling_time = %d", m_default_sampling_time);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_ACCEL_STATIC_BIAS, m_accel_static_bias, 3)) {
		ERR("[ACCEL_STATIC_BIAS] is empty\n");
		throw ENXIO;
	}

	INFO("m_accel_static_bias = (%f, %f, %f)", m_accel_static_bias[0], m_accel_static_bias[1], m_accel_static_bias[2]);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_GYRO_STATIC_BIAS, m_gyro_static_bias,3)) {
		ERR("[GYRO_STATIC_BIAS] is empty\n");
		throw ENXIO;
	}

	INFO("m_gyro_static_bias = (%f, %f, %f)", m_gyro_static_bias[0], m_gyro_static_bias[1], m_gyro_static_bias[2]);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_GEOMAGNETIC_STATIC_BIAS, m_geomagnetic_static_bias, 3)) {
		ERR("[GEOMAGNETIC_STATIC_BIAS] is empty\n");
		throw ENXIO;
	}

	INFO("m_geomagnetic_static_bias = (%f, %f, %f)", m_geomagnetic_static_bias[0], m_geomagnetic_static_bias[1], m_geomagnetic_static_bias[2]);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_ACCEL_ROTATION_DIRECTION_COMPENSATION, m_accel_rotation_direction_compensation, 3)) {
		ERR("[ACCEL_ROTATION_DIRECTION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_accel_rotation_direction_compensation = (%d, %d, %d)", m_accel_rotation_direction_compensation[0], m_accel_rotation_direction_compensation[1], m_accel_rotation_direction_compensation[2]);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_GYRO_ROTATION_DIRECTION_COMPENSATION, m_gyro_rotation_direction_compensation, 3)) {
		ERR("[GYRO_ROTATION_DIRECTION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_gyro_rotation_direction_compensation = (%d, %d, %d)", m_gyro_rotation_direction_compensation[0], m_gyro_rotation_direction_compensation[1], m_gyro_rotation_direction_compensation[2]);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_GEOMAGNETIC_ROTATION_DIRECTION_COMPENSATION, m_geomagnetic_rotation_direction_compensation, 3)) {
		ERR("[GEOMAGNETIC_ROTATION_DIRECTION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_geomagnetic_rotation_direction_compensation = (%d, %d, %d)", m_geomagnetic_rotation_direction_compensation[0], m_geomagnetic_rotation_direction_compensation[1], m_geomagnetic_rotation_direction_compensation[2]);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_ACCEL_SCALE, &m_accel_scale)) {
		ERR("[ACCEL_SCALE] is empty\n");
		throw ENXIO;
	}

	INFO("m_accel_scale = %f", m_accel_scale);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_GYRO_SCALE, &m_gyro_scale)) {
		ERR("[GYRO_SCALE] is empty\n");
		throw ENXIO;
	}

	INFO("m_gyro_scale = %f", m_gyro_scale);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_GEOMAGNETIC_SCALE, &m_geomagnetic_scale)) {
		ERR("[GEOMAGNETIC_SCALE] is empty\n");
		throw ENXIO;
	}

	INFO("m_geomagnetic_scale = %f", m_geomagnetic_scale);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_MAGNETIC_ALIGNMENT_FACTOR, &m_magnetic_alignment_factor)) {
		ERR("[MAGNETIC_ALIGNMENT_FACTOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_magnetic_alignment_factor = %d", m_magnetic_alignment_factor);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_AZIMUTH_ROTATION_COMPENSATION, &m_azimuth_rotation_compensation)) {
		ERR("[AZIMUTH_ROTATION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_azimuth_rotation_compensation = %d", m_azimuth_rotation_compensation);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_PITCH_ROTATION_COMPENSATION, &m_pitch_rotation_compensation)) {
		ERR("[PITCH_ROTATION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_pitch_rotation_compensation = %d", m_pitch_rotation_compensation);

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_ROLL_ROTATION_COMPENSATION, &m_roll_rotation_compensation)) {
		ERR("[ROLL_ROTATION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_roll_rotation_compensation = %d", m_roll_rotation_compensation);

	m_interval = m_default_sampling_time * MS_TO_US;

}

orientation_sensor::~orientation_sensor()
{
	INFO("orientation_sensor is destroyed!\n");
}

bool orientation_sensor::init(void)
{
	m_accel_sensor = sensor_plugin_loader::get_instance().get_sensor(ACCELEROMETER_SENSOR);
	m_gyro_sensor = sensor_plugin_loader::get_instance().get_sensor(GYROSCOPE_SENSOR);
	m_magnetic_sensor = sensor_plugin_loader::get_instance().get_sensor(GEOMAGNETIC_SENSOR);

	if (!m_accel_sensor || !m_gyro_sensor || !m_magnetic_sensor) {
		ERR("Failed to load sensors,  accel: 0x%x, gyro: 0x%x, mag: 0x%x",
			m_accel_sensor, m_gyro_sensor, m_magnetic_sensor);
		return false;
	}

	INFO("%s is created!", sensor_base::get_name());
	return true;
}

sensor_type_t orientation_sensor::get_type(void)
{
	return ORIENTATION_SENSOR;
}

bool orientation_sensor::on_start(void)
{
	AUTOLOCK(m_mutex);
	m_accel_sensor->add_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->start();
	m_gyro_sensor->add_client(GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_gyro_sensor->start();
	m_magnetic_sensor->add_client(GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_magnetic_sensor->start();

	activate();
	return true;
}

bool orientation_sensor::on_stop(void)
{
	AUTOLOCK(m_mutex);
	m_accel_sensor->delete_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->stop();
	m_gyro_sensor->delete_client(GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_gyro_sensor->stop();
	m_magnetic_sensor->delete_client(GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_magnetic_sensor->stop();

	deactivate();
	return true;
}

bool orientation_sensor::add_interval(int client_id, unsigned int interval, bool is_processor)
{
	AUTOLOCK(m_mutex);
	m_accel_sensor->add_interval(client_id, interval, true);
	m_gyro_sensor->add_interval(client_id, interval, true);
	m_magnetic_sensor->add_interval(client_id, interval, true);

	return sensor_base::add_interval(client_id, interval, true);
}

bool orientation_sensor::delete_interval(int client_id, bool is_processor)
{
	AUTOLOCK(m_mutex);
	m_accel_sensor->delete_interval(client_id, true);
	m_gyro_sensor->delete_interval(client_id, true);
	m_magnetic_sensor->delete_interval(client_id, true);

	return sensor_base::delete_interval(client_id, true);
}

void orientation_sensor::synthesize(const sensor_event_t &event, vector<sensor_event_t> &outs)
{
	const float MIN_DELIVERY_DIFF_FACTOR = 0.75f;
	unsigned long long diff_time;

	sensor_event_t orientation_event;
	euler_angles<float> euler_orientation;
	float azimuth_offset;

	if (event.event_type == ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME) {
		diff_time = event.data.timestamp - m_time;

		if (m_time && (diff_time < m_interval * MIN_DELIVERY_DIFF_FACTOR))
			return;

		pre_process_data(m_accel, event.data.values, m_accel_static_bias, m_accel_rotation_direction_compensation, m_accel_scale);

		m_accel.m_time_stamp = event.data.timestamp;

		m_enable_orientation |= ACCELEROMETER_ENABLED;
	}
	else if (event.event_type == GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME) {
		diff_time = event.data.timestamp - m_time;

		if (m_time && (diff_time < m_interval * MIN_DELIVERY_DIFF_FACTOR))
			return;

		pre_process_data(m_gyro, event.data.values, m_gyro_static_bias, m_gyro_rotation_direction_compensation, m_gyro_scale);

		m_gyro.m_time_stamp = event.data.timestamp;

		m_enable_orientation |= GYROSCOPE_ENABLED;
	}
	else if (event.event_type == GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME) {
		diff_time = event.data.timestamp - m_time;

		if (m_time && (diff_time < m_interval * MIN_DELIVERY_DIFF_FACTOR))
			return;

		pre_process_data(m_magnetic, event.data.values, m_geomagnetic_static_bias, m_geomagnetic_rotation_direction_compensation, m_geomagnetic_scale);

		m_magnetic.m_time_stamp = event.data.timestamp;

		m_enable_orientation |= GEOMAGNETIC_ENABLED;
	}

	if (m_enable_orientation == ORIENTATION_ENABLED) {
		m_enable_orientation = 0;

		m_orientation.m_pitch_phase_compensation = m_pitch_rotation_compensation;
		m_orientation.m_roll_phase_compensation = m_roll_rotation_compensation;
		m_orientation.m_azimuth_phase_compensation = m_azimuth_rotation_compensation;
		m_orientation.m_magnetic_alignment_factor = m_magnetic_alignment_factor;

		{
			AUTOLOCK(m_fusion_mutex);
			euler_orientation = m_orientation.get_orientation(m_accel, m_gyro, m_magnetic);
		}

		if(m_raw_data_unit == "DEGREES") {
			euler_orientation = rad2deg(euler_orientation);
			azimuth_offset = AZIMUTH_OFFSET_DEGREES;
		}
		else {
			azimuth_offset = AZIMUTH_OFFSET_RADIANS;
		}

		orientation_event.sensor_id = get_id();
		orientation_event.event_type = ORIENTATION_EVENT_RAW_DATA_REPORT_ON_TIME;
		orientation_event.data.accuracy = SENSOR_ACCURACY_GOOD;
		orientation_event.data.timestamp = get_timestamp();
		orientation_event.data.value_count = 3;
		orientation_event.data.values[1] = euler_orientation.m_ang.m_vec[0];
		orientation_event.data.values[2] = euler_orientation.m_ang.m_vec[1];
		if (euler_orientation.m_ang.m_vec[2] >= 0)
			orientation_event.data.values[0] = euler_orientation.m_ang.m_vec[2];
		else
			orientation_event.data.values[0] = euler_orientation.m_ang.m_vec[2] + azimuth_offset;

		{
			AUTOLOCK(m_value_mutex);
			m_time = orientation_event.data.timestamp;
			m_azimuth = orientation_event.data.values[0];
			m_pitch = orientation_event.data.values[1];
			m_roll = orientation_event.data.values[2];
		}

		push(orientation_event);
	}

	return;
}

int orientation_sensor::get_sensor_data(const unsigned int event_type, sensor_data_t &data)
{
	if (event_type != ORIENTATION_EVENT_RAW_DATA_REPORT_ON_TIME)
		return -1;

	AUTOLOCK(m_value_mutex);
	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_time;
	data.values[0] = m_azimuth;
	data.values[1] = m_pitch;
	data.values[2] = m_roll;
	data.value_count = 3;

	return 0;
}

bool orientation_sensor::get_properties(sensor_properties_t &properties)
{
	if(m_raw_data_unit == "DEGREES") {
		properties.min_range = -180;
		properties.max_range = 360;
	}
	else {
		properties.min_range = -PI;
		properties.max_range = 2 * PI;
	}
	properties.resolution = 0.000001;
	properties.vendor = m_vendor;
	properties.name = SENSOR_NAME;
	properties.min_interval = 1;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;

	return true;
}

extern "C" sensor_module* create(void)
{
	orientation_sensor *sensor;

	try {
		sensor = new(std::nothrow) orientation_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
