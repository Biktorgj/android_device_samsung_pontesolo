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

#include <uncal_gyro_sensor.h>
#include <sensor_plugin_loader.h>

#define MS_TO_US 1000
#define DPS_TO_MDPS 1000
#define RAW_DATA_TO_DPS_UNIT(X) ((float)(X)/((float)DPS_TO_MDPS))

#define SENSOR_NAME "UNCAL_GYROSCOPE_SENSOR"

uncal_gyro_sensor::uncal_gyro_sensor()
: m_sensor_hal(NULL)
, m_resolution(0.0f)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(UNCAL_GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME);

	physical_sensor::set_poller(uncal_gyro_sensor::working, this);
}

uncal_gyro_sensor::~uncal_gyro_sensor()
{
	INFO("uncal_gyro_sensor is destroyed!\n");
}

bool uncal_gyro_sensor::init()
{
	m_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(UNCAL_GYROSCOPE_SENSOR);

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

sensor_type_t uncal_gyro_sensor::get_type(void)
{
	return UNCAL_GYROSCOPE_SENSOR;
}

bool uncal_gyro_sensor::working(void *inst)
{
	uncal_gyro_sensor *sensor = (uncal_gyro_sensor*)inst;
	return sensor->process_event();;
}

bool uncal_gyro_sensor::process_event(void)
{
	sensor_event_t event;

	if (m_sensor_hal->is_data_ready(true) == false)
		return true;

	m_sensor_hal->get_sensor_data(event.data);

	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(UNCAL_GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME)) {
		event.sensor_id = get_id();
		event.event_type = UNCAL_GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME;
		raw_to_base(event.data);
		push(event);
	}

	return true;
}

bool uncal_gyro_sensor::on_start(void)
{
	if (!m_sensor_hal->enable()) {
		ERR("m_sensor_hal start fail\n");
		return false;
	}

	return start_poll();
}

bool uncal_gyro_sensor::on_stop(void)
{
	if (!m_sensor_hal->disable()) {
		ERR("m_sensor_hal stop fail\n");
		return false;
	}

	return stop_poll();
}

bool uncal_gyro_sensor::get_properties(sensor_properties_t &properties)
{
	return m_sensor_hal->get_properties(properties);
}

int uncal_gyro_sensor::get_sensor_data(unsigned int type, sensor_data_t &data)
{
	int state;

	if (type != UNCAL_GYRO_BASE_DATA_SET)
		return -1;

	state = m_sensor_hal->get_sensor_data(data);

	if (state < 0) {
		ERR("m_sensor_hal get struct_data fail\n");
		return -1;
	}

	raw_to_base(data);

	return 0;
}

bool uncal_gyro_sensor::set_interval(unsigned long interval)
{
	AUTOLOCK(m_mutex);

	INFO("Polling interval is set to %dms", interval);

	return m_sensor_hal->set_interval(interval);
}

void uncal_gyro_sensor::raw_to_base(sensor_data_t &data)
{
	data.value_count = 6;
	data.values[0] = data.values[0] * m_resolution;
	data.values[1] = data.values[1] * m_resolution;
	data.values[2] = data.values[2] * m_resolution;
	data.values[3] = data.values[3] * m_resolution;
	data.values[4] = data.values[4] * m_resolution;
	data.values[5] = data.values[5] * m_resolution;
}

extern "C" sensor_module* create(void)
{
	uncal_gyro_sensor *sensor;

	try {
		sensor = new(std::nothrow) uncal_gyro_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}

