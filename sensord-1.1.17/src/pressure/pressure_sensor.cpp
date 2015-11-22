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
#include <pressure_sensor.h>
#include <sensor_plugin_loader.h>
#include <algorithm>
#include <csensor_config.h>

using std::bind1st;
using std::mem_fun;

#define SENSOR_NAME "PRESSURE_SENSOR"
#define SENSOR_TYPE_PRESSURE		"PRESSURE"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_TEMPERATURE_RESOLUTION	"TEMPERATURE_RESOLUTION"
#define ELEMENT_TEMPERATURE_OFFSET		"TEMPERATURE_OFFSET"
#define ATTR_VALUE				"value"

#define SEA_LEVEL_RESOLUTION 0.01

pressure_sensor::pressure_sensor()
: m_sensor_hal(NULL)
, m_resolution(0.0f)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(PRESSURE_EVENT_RAW_DATA_REPORT_ON_TIME);

	physical_sensor::set_poller(pressure_sensor::working, this);
}

pressure_sensor::~pressure_sensor()
{
	INFO("pressure_sensor is destroyed!");
}

bool pressure_sensor::init()
{
	m_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(PRESSURE_SENSOR);

	if (!m_sensor_hal) {
		ERR("cannot load sensor_hal[%s]", sensor_base::get_name());
		return false;
	}

	sensor_properties_t properties;

	if (!m_sensor_hal->get_properties(properties)) {
		ERR("sensor->get_properties() is failed!\n");
		return false;
	}

	m_resolution = properties.resolution;

	string model_id = m_sensor_hal->get_model_id();

	csensor_config &config = csensor_config::get_instance();

	double temperature_resolution;

	if (!config.get(SENSOR_TYPE_PRESSURE, model_id, ELEMENT_TEMPERATURE_RESOLUTION, temperature_resolution)) {
		ERR("[TEMPERATURE_RESOLUTION] is empty\n");
		throw ENXIO;
	}

	m_temperature_resolution = (float)temperature_resolution;
	INFO("m_temperature_resolution = %f\n", m_temperature_resolution);

	double temperature_offset;

	if (!config.get(SENSOR_TYPE_PRESSURE, model_id, ELEMENT_TEMPERATURE_OFFSET, temperature_offset)) {
		ERR("[TEMPERATURE_OFFSET] is empty\n");
		throw ENXIO;
	}

	m_temperature_offset = (float)temperature_offset;
	INFO("m_temperature_offset = %f\n", m_temperature_offset);

	INFO("%s is created!", sensor_base::get_name());

	return true;
}

sensor_type_t pressure_sensor::get_type(void)
{
	return PRESSURE_SENSOR;
}

bool pressure_sensor::working(void *inst)
{
	pressure_sensor *sensor = (pressure_sensor*)inst;
	return sensor->process_event();
}

bool pressure_sensor::process_event(void)
{
	sensor_event_t event;

	if (!m_sensor_hal->is_data_ready(true))
		return true;

	m_sensor_hal->get_sensor_data(event.data);

	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(PRESSURE_EVENT_RAW_DATA_REPORT_ON_TIME)) {
		event.sensor_id = get_id();
		event.event_type = PRESSURE_EVENT_RAW_DATA_REPORT_ON_TIME;
		raw_to_base(event.data);
		push(event);
	}

	return true;
}

bool pressure_sensor::on_start(void)
{
	if (!m_sensor_hal->enable()) {
		ERR("m_sensor_hal start fail\n");
		return false;
	}

	return start_poll();
}

bool pressure_sensor::on_stop(void)
{
	if (!m_sensor_hal->disable()) {
		ERR("m_sensor_hal stop fail\n");
		return false;
	}

	return stop_poll();
}

bool pressure_sensor::get_properties(sensor_properties_t &properties)
{
	return m_sensor_hal->get_properties(properties);
}

int pressure_sensor::get_sensor_data(unsigned int type, sensor_data_t &data)
{
	int ret;

	ret = m_sensor_hal->get_sensor_data(data);

	if (ret < 0)
		return -1;

	if (type == PRESSURE_BASE_DATA_SET) {
		raw_to_base(data);
		return 0;
	}

	return -1;
}

bool pressure_sensor::set_interval(unsigned long interval)
{
	AUTOLOCK(m_mutex);

	INFO("Polling interval is set to %dms", interval);

	return m_sensor_hal->set_interval(interval);
}

float pressure_sensor::pressure_to_altitude(float pressure)
{
	return 44330.0f * (1.0f - pow(pressure/m_sea_level_pressure, 1.0f/5.255f));
}

void pressure_sensor::raw_to_base(sensor_data_t &data)
{
	data.values[0] = data.values[0] * m_resolution;
	m_sea_level_pressure = data.values[1] * SEA_LEVEL_RESOLUTION;
	data.values[1] = pressure_to_altitude(data.values[0]);
	data.values[2] = data.values[2] * m_temperature_resolution + m_temperature_offset;
	data.value_count = 3;
}

extern "C" sensor_module* create(void)
{
	pressure_sensor *sensor;

	try {
		sensor = new(std::nothrow) pressure_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
