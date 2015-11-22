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

#include <virtual_sensor.h>
#include <auto_rotation_sensor.h>
#include <sensor_plugin_loader.h>
#include <auto_rotation_alg.h>
#include <auto_rotation_alg_emul.h>

using std::bind1st;
using std::mem_fun;

#define SENSOR_NAME "AUTO_ROTATION_SENSOR"
#define AUTO_ROTATION_LIB "/usr/lib/sensord/libauto-rotation.so"

auto_rotation_sensor::auto_rotation_sensor()
: m_accel_sensor(NULL)
, m_rotation(0)
, m_rotation_time(1) // rotation state is valid from initial state, so set rotation time to non-zero value
, m_alg(NULL)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(AUTO_ROTATION_EVENT_CHANGE_STATE);
}

auto_rotation_sensor::~auto_rotation_sensor()
{
	delete m_alg;

	INFO("auto_rotation_sensor is destroyed!\n");
}

bool auto_rotation_sensor::check_lib(void)
{
	if (access(AUTO_ROTATION_LIB, F_OK) < 0)
		return false;

	return true;
}

auto_rotation_alg *auto_rotation_sensor::get_alg()
{
	return new auto_rotation_alg_emul();
}

bool auto_rotation_sensor::init()
{
	m_accel_sensor = sensor_plugin_loader::get_instance().get_sensor(ACCELEROMETER_SENSOR);

	if (!m_accel_sensor) {
		ERR("cannot load accel sensor_hal[%s]", sensor_base::get_name());
		return false;
	}

	m_alg = get_alg();

	if (!m_alg) {
		ERR("Not supported AUTO ROTATION sensor");
		return false;
	}

	if (!m_alg->open())
		return false;

	set_privilege(SENSOR_PRIVILEGE_INTERNAL);

	INFO("%s is created!\n", sensor_base::get_name());

	return true;
}

sensor_type_t auto_rotation_sensor::get_type(void)
{
	return AUTO_ROTATION_SENSOR;
}

bool auto_rotation_sensor::on_start(void)
{
	m_rotation = AUTO_ROTATION_DEGREE_UNKNOWN;

	m_alg->start();

	m_accel_sensor->add_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->start();

	return activate();
}

bool auto_rotation_sensor::on_stop(void)
{
	m_accel_sensor->delete_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->stop();

	return deactivate();
}

void auto_rotation_sensor::synthesize(const sensor_event_t& event, vector<sensor_event_t> &outs)
{
	AUTOLOCK(m_mutex);

	if (event.event_type == ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME) {
		int rotation;
		float acc[3];
		acc[0] = event.data.values[0];
		acc[1] = event.data.values[1];
		acc[2] = event.data.values[2];

		if (!m_alg->get_rotation(acc, event.data.timestamp, m_rotation, rotation))
			return;

		AUTOLOCK(m_value_mutex);
		sensor_event_t rotation_event;

		INFO("Rotation: %d, ACC[0]: %f, ACC[1]: %f, ACC[2]: %f", rotation, event.data.values[0], event.data.values[1], event.data.values[2]);
		rotation_event.sensor_id = get_id();
		rotation_event.data.accuracy = SENSOR_ACCURACY_GOOD;
		rotation_event.event_type = AUTO_ROTATION_EVENT_CHANGE_STATE;
		rotation_event.data.timestamp = event.data.timestamp;
		rotation_event.data.values[0] = rotation;
		rotation_event.data.value_count = 1;
		outs.push_back(rotation_event);
		m_rotation = rotation;
		m_rotation_time = event.data.timestamp;

		return;
	}
	return;
}

int auto_rotation_sensor::get_sensor_data(unsigned int data_id, sensor_data_t &data)
{
	if (data_id != AUTO_ROTATION_BASE_DATA_SET)
		return -1;

	AUTOLOCK(m_value_mutex);

	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_rotation_time;
	data.values[0] = m_rotation;
	data.value_count = 1;

	return 0;
}

bool auto_rotation_sensor::get_properties(sensor_properties_t &properties)
{
	properties.name = "Auto Rotation Sensor";
	properties.vendor = "Samsung Electronics";
	properties.min_range = AUTO_ROTATION_DEGREE_UNKNOWN;
	properties.max_range = AUTO_ROTATION_DEGREE_270;
	properties.min_interval = 1;
	properties.resolution = 1;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;

	return true;
}

extern "C" sensor_module* create(void)
{
	auto_rotation_sensor *sensor;

	try {
		sensor = new(std::nothrow) auto_rotation_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
