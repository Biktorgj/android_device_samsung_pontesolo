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

#include <gyro_sensor.h>
#include <sensor_plugin_loader.h>

#define MS_TO_US 1000
#define DPS_TO_MDPS 1000
#define RAW_DATA_TO_DPS_UNIT(X) ((float)(X)/((float)DPS_TO_MDPS))

#define SENSOR_NAME "GYROSCOPE_SENSOR"

gyro_sensor::gyro_sensor()
: m_sensor_hal(NULL)
, m_resolution(0.0f)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME);
	register_supported_event(GYROSCOPE_EVENT_UNPROCESSED_DATA_REPORT_ON_TIME);

	physical_sensor::set_poller(gyro_sensor::working, this);
}

gyro_sensor::~gyro_sensor()
{
	INFO("gyro_sensor is destroyed!\n");
}

bool gyro_sensor::init()
{
	m_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(GYROSCOPE_SENSOR);

	if (!m_sensor_hal) {
		ERR("cannot load sensor_hal[%s]", sensor_base::get_name());
		return false;
	}

	sensor_properties_t properties;

	if (m_sensor_hal->get_properties(properties) == false) {
		ERR("sensor->get_properties() is failed!\n");
		return false;
	}

	m_resolution = properties.resolution;

	INFO("%s is created!\n", sensor_base::get_name());

	return true;
}

sensor_type_t gyro_sensor::get_type(void)
{
	return GYROSCOPE_SENSOR;
}

bool gyro_sensor::working(void *inst)
{
	gyro_sensor *sensor = (gyro_sensor*)inst;
	return sensor->process_event();;
}

bool gyro_sensor::process_event(void)
{
	sensor_event_t event;

	if (m_sensor_hal->is_data_ready(true) == false)
		return true;

	m_sensor_hal->get_sensor_data(event.data);

	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(GYROSCOPE_EVENT_UNPROCESSED_DATA_REPORT_ON_TIME)) {
		event.sensor_id = get_id();
		event.event_type = GYROSCOPE_EVENT_UNPROCESSED_DATA_REPORT_ON_TIME;
		push(event);
	}

	if (get_client_cnt(GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME)) {
		event.sensor_id = get_id();
		event.event_type = GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME;
		raw_to_base(event.data);
		push(event);
	}

	return true;
}

bool gyro_sensor::on_start(void)
{
	if (!m_sensor_hal->enable()) {
		ERR("m_sensor_hal start fail\n");
		return false;
	}

	return start_poll();
}

bool gyro_sensor::on_stop(void)
{
	if (!m_sensor_hal->disable()) {
		ERR("m_sensor_hal stop fail\n");
		return false;
	}

	return stop_poll();
}

bool gyro_sensor::get_properties(sensor_properties_t &properties)
{
	return m_sensor_hal->get_properties(properties);
}

int gyro_sensor::get_sensor_data(unsigned int type, sensor_data_t &data)
{
	int state;

	if (type != GYRO_BASE_DATA_SET)
		return -1;

	state = m_sensor_hal->get_sensor_data(data);

	if (state < 0) {
		ERR("m_sensor_hal get struct_data fail\n");
		return -1;
	}

	raw_to_base(data);

	return 0;
}

bool gyro_sensor::set_interval(unsigned long interval)
{
	AUTOLOCK(m_mutex);

	INFO("Polling interval is set to %dms", interval);

	return m_sensor_hal->set_interval(interval);
}

void gyro_sensor::raw_to_base(sensor_data_t &data)
{
	data.value_count = 3;
	data.values[0] = data.values[0] * m_resolution;
	data.values[1] = data.values[1] * m_resolution;
	data.values[2] = data.values[2] * m_resolution;
}

extern "C" sensor_module* create(void)
{
	gyro_sensor *sensor;

	try {
		sensor = new(std::nothrow) gyro_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
