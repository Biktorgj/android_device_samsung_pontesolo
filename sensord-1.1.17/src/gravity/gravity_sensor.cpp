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
#include <gravity_sensor.h>
#include <sensor_plugin_loader.h>
#include <cvirtual_sensor_config.h>

#define INITIAL_VALUE -1
#define GRAVITY 9.80665

#define DEG2RAD (M_PI/180)
#define DEVIATION 0.1

#define SENSOR_NAME "GRAVITY_SENSOR"
#define SENSOR_TYPE_GRAVITY		"GRAVITY"
#define SENSOR_TYPE_ORIENTATION		"ORIENTATION"

#define MS_TO_US 1000

#define ELEMENT_NAME											"NAME"
#define ELEMENT_VENDOR											"VENDOR"
#define ELEMENT_RAW_DATA_UNIT									"RAW_DATA_UNIT"
#define ELEMENT_DEFAULT_SAMPLING_TIME							"DEFAULT_SAMPLING_TIME"
#define ELEMENT_GRAVITY_SIGN_COMPENSATION						"GRAVITY_SIGN_COMPENSATION"
#define ELEMENT_ORIENTATION_DATA_UNIT							"RAW_DATA_UNIT"

gravity_sensor::gravity_sensor()
: m_orientation_sensor(NULL)
, m_x(INITIAL_VALUE)
, m_y(INITIAL_VALUE)
, m_z(INITIAL_VALUE)
, m_time(0)
{
	cvirtual_sensor_config &config = cvirtual_sensor_config::get_instance();

	m_name = std::string(SENSOR_NAME);
	register_supported_event(GRAVITY_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_time = get_timestamp();

	if (!config.get(SENSOR_TYPE_GRAVITY, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_ORIENTATION, ELEMENT_ORIENTATION_DATA_UNIT, m_orientation_data_unit)) {
		ERR("[ORIENTATION_DATA_UNIT] is empty\n");
		throw ENXIO;
	}

	INFO("m_orientation_data_unit = %s", m_orientation_data_unit.c_str());

	if (!config.get(SENSOR_TYPE_GRAVITY, ELEMENT_RAW_DATA_UNIT, m_raw_data_unit)) {
		ERR("[RAW_DATA_UNIT] is empty\n");
		throw ENXIO;
	}

	INFO("m_raw_data_unit = %s", m_raw_data_unit.c_str());

	if (!config.get(SENSOR_TYPE_GRAVITY, ELEMENT_DEFAULT_SAMPLING_TIME, &m_default_sampling_time)) {
		ERR("[DEFAULT_SAMPLING_TIME] is empty\n");
		throw ENXIO;
	}

	INFO("m_default_sampling_time = %d", m_default_sampling_time);

	if (!config.get(SENSOR_TYPE_GRAVITY, ELEMENT_GRAVITY_SIGN_COMPENSATION, m_gravity_sign_compensation, 3)) {
		ERR("[GRAVITY_SIGN_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_gravity_sign_compensation = (%d, %d, %d)", m_gravity_sign_compensation[0], m_gravity_sign_compensation[1], m_gravity_sign_compensation[2]);

	m_interval = m_default_sampling_time * MS_TO_US;
}

gravity_sensor::~gravity_sensor()
{
	INFO("gravity_sensor is destroyed!\n");
}

bool gravity_sensor::init()
{
	m_orientation_sensor = sensor_plugin_loader::get_instance().get_sensor(ORIENTATION_SENSOR);

	if (!m_orientation_sensor) {
		ERR("Failed to load orientation sensor: 0x%x", m_orientation_sensor);
		return false;
	}

	INFO("%s is created!", sensor_base::get_name());
	return true;
}

sensor_type_t gravity_sensor::get_type(void)
{
	return GRAVITY_SENSOR;
}

bool gravity_sensor::on_start(void)
{
	AUTOLOCK(m_mutex);

	m_orientation_sensor->add_client(ORIENTATION_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_orientation_sensor->start();

	activate();
	return true;
}

bool gravity_sensor::on_stop(void)
{
	AUTOLOCK(m_mutex);

	m_orientation_sensor->delete_client(ORIENTATION_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_orientation_sensor->stop();

	deactivate();
	return true;
}

bool gravity_sensor::add_interval(int client_id, unsigned int interval, bool is_processor)
{
	AUTOLOCK(m_mutex);
	m_orientation_sensor->add_interval(client_id, interval, true);

	return sensor_base::add_interval(client_id, interval, true);
}

bool gravity_sensor::delete_interval(int client_id, bool is_processor)
{
	AUTOLOCK(m_mutex);
	m_orientation_sensor->delete_interval(client_id, true);

	return sensor_base::delete_interval(client_id, true);
}

void gravity_sensor::synthesize(const sensor_event_t &event, vector<sensor_event_t> &outs)
{
	sensor_event_t gravity_event;
	float pitch, roll, azimuth;

	azimuth = event.data.values[0];
	pitch = event.data.values[1];
	roll = event.data.values[2];

	const float MIN_DELIVERY_DIFF_FACTOR = 0.75f;
	unsigned long long diff_time;

	if(m_orientation_data_unit == "DEGREES") {
		azimuth *= DEG2RAD;
		pitch *= DEG2RAD;
		roll *= DEG2RAD;
	}

	if (event.event_type == ORIENTATION_EVENT_RAW_DATA_REPORT_ON_TIME) {
		diff_time = event.data.timestamp - m_time;

		if (m_time && (diff_time < m_interval * MIN_DELIVERY_DIFF_FACTOR))
			return;

		gravity_event.sensor_id = get_id();
		gravity_event.event_type = GRAVITY_EVENT_RAW_DATA_REPORT_ON_TIME;
		if ((roll >= (M_PI/2)-DEVIATION && roll <= (M_PI/2)+DEVIATION) ||
				(roll >= -(M_PI/2)-DEVIATION && roll <= -(M_PI/2)+DEVIATION)) {
			gravity_event.data.values[0] = m_gravity_sign_compensation[0] * GRAVITY * sin(roll) * cos(azimuth);
			gravity_event.data.values[1] = m_gravity_sign_compensation[1] * GRAVITY * sin(azimuth);
			gravity_event.data.values[2] = m_gravity_sign_compensation[2] * GRAVITY * cos(roll);
		} else if ((pitch >= (M_PI/2)-DEVIATION && pitch <= (M_PI/2)+DEVIATION) ||
				(pitch >= -(M_PI/2)-DEVIATION && pitch <= -(M_PI/2)+DEVIATION)) {
			gravity_event.data.values[0] = m_gravity_sign_compensation[0] * GRAVITY * sin(azimuth);
			gravity_event.data.values[1] = m_gravity_sign_compensation[1] * GRAVITY * sin(pitch) * cos(azimuth);
			gravity_event.data.values[2] = m_gravity_sign_compensation[2] * GRAVITY * cos(pitch);
		} else {
			gravity_event.data.values[0] = m_gravity_sign_compensation[0] * GRAVITY * sin(roll);
			gravity_event.data.values[1] = m_gravity_sign_compensation[1] * GRAVITY * sin(pitch);
			gravity_event.data.values[2] = m_gravity_sign_compensation[2] * GRAVITY * cos(roll) * cos(pitch);
		}
		gravity_event.data.value_count = 3;
		gravity_event.data.timestamp = get_timestamp();
		gravity_event.data.accuracy = SENSOR_ACCURACY_GOOD;

		push(gravity_event);

		{
			AUTOLOCK(m_value_mutex);

			m_time = gravity_event.data.timestamp;
			m_x = gravity_event.data.values[0];
			m_y = gravity_event.data.values[1];
			m_z = gravity_event.data.values[2];
		}
	}
}

int gravity_sensor::get_sensor_data(const unsigned int event_type, sensor_data_t &data)
{
	if (event_type != GRAVITY_EVENT_RAW_DATA_REPORT_ON_TIME)
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

bool gravity_sensor::get_properties(sensor_properties_t &properties)
{
	properties.min_range = -GRAVITY;
	properties.max_range = GRAVITY;
	properties.resolution = 0.000001;
	properties.vendor = m_vendor;
	properties.name = SENSOR_NAME;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	properties.min_interval = 1;

	return true;
}

extern "C" sensor_module* create(void)
{
	gravity_sensor *sensor;

	try {
		sensor = new(std::nothrow) gravity_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
