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

#include <common.h>
#include <sf_common.h>

#include <accel_sensor.h>
#include <sensor_plugin_loader.h>
#include <algorithm>

using std::bind1st;
using std::mem_fun;

#define GRAVITY 9.80665
#define G_TO_MG 1000

#define RAW_DATA_TO_G_UNIT(X) (((float)(X))/((float)G_TO_MG))
#define RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(X) (GRAVITY * (RAW_DATA_TO_G_UNIT(X)))

#define SENSOR_NAME "ACCELEROMETER_SENSOR"

accel_sensor::accel_sensor()
: m_sensor_hal(NULL)
, m_interval(POLL_1HZ_MS)
{
	m_name = string(SENSOR_NAME);

	vector<unsigned int> supported_events = {
		ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME,
		ACCELEROMETER_EVENT_UNPROCESSED_DATA_REPORT_ON_TIME,
	};

	for_each(supported_events.begin(), supported_events.end(),
		bind1st(mem_fun(&sensor_base::register_supported_event), this));

	physical_sensor::set_poller(accel_sensor::working, this);
}

accel_sensor::~accel_sensor()
{
	INFO("accel_sensor is destroyed!\n");
}

bool accel_sensor::init()
{
	m_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(ACCELEROMETER_SENSOR);

	if (!m_sensor_hal) {
		ERR("cannot load sensor_hal[%s]", sensor_base::get_name());
		return false;
	}

	sensor_properties_t properties;

	if (m_sensor_hal->get_properties(properties) == false) {
		ERR("sensor->get_properties() is failed!\n");
		return false;
	}

	m_raw_data_unit = properties.resolution / GRAVITY * G_TO_MG;

	INFO("m_raw_data_unit accel : [%f]\n", m_raw_data_unit);

	INFO("%s is created!\n", sensor_base::get_name());
	return true;
}

sensor_type_t accel_sensor::get_type(void)
{
	return ACCELEROMETER_SENSOR;
}

bool accel_sensor::working(void *inst)
{
	accel_sensor *sensor = (accel_sensor*)inst;
	return sensor->process_event();
}

bool accel_sensor::process_event(void)
{
	sensor_event_t base_event;

	if (!m_sensor_hal->is_data_ready(true))
		return true;

	m_sensor_hal->get_sensor_data(base_event.data);

	AUTOLOCK(m_mutex);
	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(ACCELEROMETER_EVENT_UNPROCESSED_DATA_REPORT_ON_TIME)) {
		base_event.sensor_id = get_id();
		base_event.event_type = ACCELEROMETER_EVENT_UNPROCESSED_DATA_REPORT_ON_TIME;
		push(base_event);
	}

	if (get_client_cnt(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME)) {
		base_event.sensor_id = get_id();
		base_event.event_type = ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME;
		raw_to_base(base_event.data);
		push(base_event);
	}

	return true;
}

bool accel_sensor::on_start(void)
{
	if (!m_sensor_hal->enable()) {
		ERR("m_sensor_hal start fail\n");
		return false;
	}

	return start_poll();
}

bool accel_sensor::on_stop(void)
{
	if (!m_sensor_hal->disable()) {
		ERR("m_sensor_hal stop fail\n");
		return false;
	}

	return stop_poll();
}

bool accel_sensor::get_properties(sensor_properties_t &properties)
{
	return m_sensor_hal->get_properties(properties);
}

int accel_sensor::get_sensor_data(unsigned int type, sensor_data_t &data)
{
	if (m_sensor_hal->get_sensor_data(data) < 0) {
		ERR("Failed to get sensor data");
		return -1;
	}

	if (type == ACCELEROMETER_BASE_DATA_SET) {
		raw_to_base(data);
	} else {
		ERR("Does not support type: 0x%x", type);
		return -1;
	}

	return 0;
}

bool accel_sensor::set_interval(unsigned long interval)
{
	AUTOLOCK(m_mutex);

	m_interval = interval;

	INFO("Polling interval is set to %dms", interval);

	return m_sensor_hal->set_interval(interval);
}

void accel_sensor::raw_to_base(sensor_data_t &data)
{
	data.value_count = 3;
	data.values[0] = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(data.values[0] * m_raw_data_unit);
	data.values[1] = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(data.values[1] * m_raw_data_unit);
	data.values[2] = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(data.values[2] * m_raw_data_unit);
}

extern "C" sensor_module* create(void)
{
	accel_sensor *sensor;

	try {
		sensor = new(std::nothrow) accel_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
