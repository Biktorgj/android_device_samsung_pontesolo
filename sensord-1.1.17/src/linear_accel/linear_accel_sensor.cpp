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
#include <linear_accel_sensor.h>
#include <sensor_plugin_loader.h>
#include <cvirtual_sensor_config.h>

#define SENSOR_NAME "LINEAR_ACCEL_SENSOR"
#define SENSOR_TYPE_LINEAR_ACCEL	"LINEAR_ACCEL"

#define ELEMENT_NAME											"NAME"
#define ELEMENT_VENDOR											"VENDOR"
#define ELEMENT_RAW_DATA_UNIT									"RAW_DATA_UNIT"
#define ELEMENT_DEFAULT_SAMPLING_TIME							"DEFAULT_SAMPLING_TIME"
#define ELEMENT_ACCEL_STATIC_BIAS								"ACCEL_STATIC_BIAS"
#define ELEMENT_ACCEL_ROTATION_DIRECTION_COMPENSATION			"ACCEL_ROTATION_DIRECTION_COMPENSATION"
#define ELEMENT_ACCEL_SCALE										"ACCEL_SCALE"
#define ELEMENT_LINEAR_ACCEL_SIGN_COMPENSATION					"LINEAR_ACCEL_SIGN_COMPENSATION"

#define INITIAL_VALUE -1
#define GRAVITY 9.80665

#define MS_TO_US 1000

#define ACCELEROMETER_ENABLED 0x01
#define GRAVITY_ENABLED 0x02
#define LINEAR_ACCEL_ENABLED 3

linear_accel_sensor::linear_accel_sensor()
: m_accel_sensor(NULL)
, m_gravity_sensor(NULL)
, m_x(INITIAL_VALUE)
, m_y(INITIAL_VALUE)
, m_z(INITIAL_VALUE)
, m_time(0)
{
	cvirtual_sensor_config &config = cvirtual_sensor_config::get_instance();

	m_name = string(SENSOR_NAME);
	m_enable_linear_accel = 0;
	register_supported_event(LINEAR_ACCEL_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_time = get_timestamp();

	if (!config.get(SENSOR_TYPE_LINEAR_ACCEL, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_LINEAR_ACCEL, ELEMENT_RAW_DATA_UNIT, m_raw_data_unit)) {
		ERR("[RAW_DATA_UNIT] is empty\n");
		throw ENXIO;
	}

	INFO("m_raw_data_unit = %s", m_raw_data_unit.c_str());

	if (!config.get(SENSOR_TYPE_LINEAR_ACCEL, ELEMENT_DEFAULT_SAMPLING_TIME, &m_default_sampling_time)) {
		ERR("[DEFAULT_SAMPLING_TIME] is empty\n");
		throw ENXIO;
	}

	INFO("m_default_sampling_time = %d", m_default_sampling_time);

	if (!config.get(SENSOR_TYPE_LINEAR_ACCEL, ELEMENT_ACCEL_STATIC_BIAS, m_accel_static_bias, 3)) {
		ERR("[ACCEL_STATIC_BIAS] is empty\n");
		throw ENXIO;
	}

	if (!config.get(SENSOR_TYPE_LINEAR_ACCEL, ELEMENT_ACCEL_ROTATION_DIRECTION_COMPENSATION, m_accel_rotation_direction_compensation, 3)) {
		ERR("[ACCEL_ROTATION_DIRECTION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_accel_rotation_direction_compensation = (%d, %d, %d)", m_accel_rotation_direction_compensation[0], m_accel_rotation_direction_compensation[1], m_accel_rotation_direction_compensation[2]);


	if (!config.get(SENSOR_TYPE_LINEAR_ACCEL, ELEMENT_ACCEL_SCALE, &m_accel_scale)) {
		ERR("[ACCEL_SCALE] is empty\n");
		throw ENXIO;
	}

	INFO("m_accel_scale = %f", m_accel_scale);


	if (!config.get(SENSOR_TYPE_LINEAR_ACCEL, ELEMENT_LINEAR_ACCEL_SIGN_COMPENSATION, m_linear_accel_sign_compensation, 3)) {
		ERR("[LINEAR_ACCEL_SIGN_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_linear_accel_sign_compensation = (%d, %d, %d)", m_linear_accel_sign_compensation[0], m_linear_accel_sign_compensation[1], m_linear_accel_sign_compensation[2]);

	m_interval = m_default_sampling_time * MS_TO_US;

}

linear_accel_sensor::~linear_accel_sensor()
{
	INFO("linear_accel_sensor is destroyed!\n");
}

bool linear_accel_sensor::init()
{
	m_gravity_sensor = sensor_plugin_loader::get_instance().get_sensor(GRAVITY_SENSOR);
	m_accel_sensor = sensor_plugin_loader::get_instance().get_sensor(ACCELEROMETER_SENSOR);

	if (!m_accel_sensor || !m_gravity_sensor) {
		ERR("Failed to load sensors,  accel: 0x%x, gravity: 0x%x",
			m_accel_sensor, m_gravity_sensor);
		return false;
	}

	INFO("%s is created!", sensor_base::get_name());
	return true;
}

sensor_type_t linear_accel_sensor::get_type(void)
{
	return LINEAR_ACCEL_SENSOR;
}

bool linear_accel_sensor::on_start(void)
{
	AUTOLOCK(m_mutex);
	m_gravity_sensor->add_client(GRAVITY_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_gravity_sensor->start();

	m_accel_sensor->add_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->start();

	activate();
	return true;
}

bool linear_accel_sensor::on_stop(void)
{
	AUTOLOCK(m_mutex);
	m_gravity_sensor->delete_client(GRAVITY_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_gravity_sensor->stop();

	m_accel_sensor->delete_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->stop();

	deactivate();
	return true;
}

bool linear_accel_sensor::add_interval(int client_id, unsigned int interval, bool is_processor)
{
	AUTOLOCK(m_mutex);
	m_gravity_sensor->add_interval(client_id, interval, true);
	m_accel_sensor->add_interval(client_id, interval, true);

	return sensor_base::add_interval(client_id, interval, true);
}

bool linear_accel_sensor::delete_interval(int client_id, bool is_processor)
{
	AUTOLOCK(m_mutex);
	m_gravity_sensor->delete_interval(client_id, true);
	m_accel_sensor->delete_interval(client_id, true);

	return sensor_base::delete_interval(client_id, true);
}

void linear_accel_sensor::synthesize(const sensor_event_t &event, vector<sensor_event_t> &outs)
{
	sensor_event_t lin_accel_event;

	const float MIN_DELIVERY_DIFF_FACTOR = 0.75f;
	unsigned long long diff_time;

	if (event.event_type == ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME) {
		diff_time = event.data.timestamp - m_time;

		if (m_time && (diff_time < m_interval * MIN_DELIVERY_DIFF_FACTOR))
			return;

		m_accel.m_data.m_vec[0] = m_accel_rotation_direction_compensation[0] * (event.data.values[0] - m_accel_static_bias[0]) / m_accel_scale;
		m_accel.m_data.m_vec[1] = m_accel_rotation_direction_compensation[1] * (event.data.values[1] - m_accel_static_bias[1]) / m_accel_scale;
		m_accel.m_data.m_vec[2] = m_accel_rotation_direction_compensation[2] * (event.data.values[2] - m_accel_static_bias[2]) / m_accel_scale;

		m_accel.m_time_stamp = event.data.timestamp;

		m_enable_linear_accel |= ACCELEROMETER_ENABLED;
	}
	else if (event.event_type == GRAVITY_EVENT_RAW_DATA_REPORT_ON_TIME) {
		diff_time = event.data.timestamp - m_time;

		if (m_time && (diff_time < m_interval * MIN_DELIVERY_DIFF_FACTOR))
			return;

		m_gravity.m_data.m_vec[0] = event.data.values[0];
		m_gravity.m_data.m_vec[1] = event.data.values[1];
		m_gravity.m_data.m_vec[2] = event.data.values[2];

		m_gravity.m_time_stamp = event.data.timestamp;

		m_enable_linear_accel |= GRAVITY_ENABLED;
	}

	if (m_enable_linear_accel == LINEAR_ACCEL_ENABLED) {
		m_enable_linear_accel = 0;

		lin_accel_event.sensor_id = get_id();
		lin_accel_event.event_type = LINEAR_ACCEL_EVENT_RAW_DATA_REPORT_ON_TIME;
		lin_accel_event.data.value_count = 3;
		lin_accel_event.data.timestamp = get_timestamp();
		lin_accel_event.data.accuracy = SENSOR_ACCURACY_GOOD;
		lin_accel_event.data.values[0] = m_linear_accel_sign_compensation[0] * (m_accel.m_data.m_vec[0] - m_gravity.m_data.m_vec[0]);
		lin_accel_event.data.values[1] = m_linear_accel_sign_compensation[1] * (m_accel.m_data.m_vec[1] - m_gravity.m_data.m_vec[1]);
		lin_accel_event.data.values[2] = m_linear_accel_sign_compensation[2] * (m_accel.m_data.m_vec[2] - m_gravity.m_data.m_vec[2]);
		push(lin_accel_event);

		{
			AUTOLOCK(m_value_mutex);
			m_time = lin_accel_event.data.timestamp;
			m_x = lin_accel_event.data.values[0];
			m_y = lin_accel_event.data.values[1];
			m_z = lin_accel_event.data.values[2];
		}
	}

	return;
}

int linear_accel_sensor::get_sensor_data(const unsigned int event_type, sensor_data_t &data)
{
	if (event_type != LINEAR_ACCEL_EVENT_RAW_DATA_REPORT_ON_TIME)
		return -1;

	AUTOLOCK(m_value_mutex);
	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_time;
	data.values[0] = m_x;
	data.values[1] = m_y;
	data.values[2] = m_z;
	data.value_count = 3;
	return 0;
}

bool linear_accel_sensor::get_properties(sensor_properties_t &properties)
{
	m_accel_sensor->get_properties(properties);
	properties.name = "Linear Acceleration Sensor";
	properties.vendor = m_vendor;
	properties.resolution = 0.000001;

	return true;
}

extern "C" sensor_module* create(void)
{
	linear_accel_sensor *sensor;

	try {
		sensor = new(std::nothrow) linear_accel_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
