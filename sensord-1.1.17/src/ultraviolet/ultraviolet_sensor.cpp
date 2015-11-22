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

#include <ultraviolet_sensor.h>
#include <sensor_plugin_loader.h>
#include <algorithm>

using std::bind1st;
using std::mem_fun;

#define SENSOR_NAME "ULTRAVIOLET_SENSOR"

ultraviolet_sensor::ultraviolet_sensor()
: m_sensor_hal(NULL)
, m_resolution(0.0f)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(ULTRAVIOLET_EVENT_RAW_DATA_REPORT_ON_TIME);

	physical_sensor::set_poller(ultraviolet_sensor::working, this);
}

ultraviolet_sensor::~ultraviolet_sensor()
{
	INFO("ultraviolet_sensor is destroyed!");
}

bool ultraviolet_sensor::init()
{
	m_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(ULTRAVIOLET_SENSOR);

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

	INFO("%s is created!", sensor_base::get_name());

	return true;
}

sensor_type_t ultraviolet_sensor::get_type(void)
{
	return ULTRAVIOLET_SENSOR;
}

bool ultraviolet_sensor::working(void *inst)
{
	ultraviolet_sensor *sensor = (ultraviolet_sensor*)inst;
	return sensor->process_event();
}

bool ultraviolet_sensor::process_event(void)
{
	sensor_event_t event;

	if (!m_sensor_hal->is_data_ready(true))
		return true;

	m_sensor_hal->get_sensor_data(event.data);

	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(ULTRAVIOLET_EVENT_RAW_DATA_REPORT_ON_TIME)) {
		event.sensor_id = get_id();
		event.event_type = ULTRAVIOLET_EVENT_RAW_DATA_REPORT_ON_TIME;
		raw_to_base(event.data);
		push(event);
	}

	return true;
}

bool ultraviolet_sensor::on_start(void)
{
	if (!m_sensor_hal->enable()) {
		ERR("m_sensor_hal start fail\n");
		return false;
	}

	return start_poll();
}

bool ultraviolet_sensor::on_stop(void)
{
	if (!m_sensor_hal->disable()) {
		ERR("m_sensor_hal stop fail\n");
		return false;
	}

	return stop_poll();
}

bool ultraviolet_sensor::get_properties(sensor_properties_t &properties)
{
	return m_sensor_hal->get_properties(properties);
}

int ultraviolet_sensor::get_sensor_data(unsigned int type, sensor_data_t &data)
{
	int ret;

	ret = m_sensor_hal->get_sensor_data(data);

	if (ret < 0)
		return -1;

	if (type == ULTRAVIOLET_BASE_DATA_SET) {
		raw_to_base(data);
		return 0;
	}

	return -1;
}

bool ultraviolet_sensor::set_interval(unsigned long interval)
{
	AUTOLOCK(m_mutex);

	INFO("Polling interval is set to %dms", interval);

	return m_sensor_hal->set_interval(interval);
}

void ultraviolet_sensor::raw_to_base(sensor_data_t &data)
{
	/*
	double lsb = data.values[0];
	double comp_lsb;

	const double c = 0.89;
	const double e = 2.12;
	const double f = -132.8;

	if (lsb > 108.8f)
		comp_lsb = e * lsb + f;
	else
		comp_lsb = c * lsb;

	data.values[0] = comp_lsb * m_resolution;
	*/
	data.values[0] = data.values[0] * m_resolution;
	data.value_count = 1;
}

extern "C" sensor_module* create(void)
{
	ultraviolet_sensor *sensor;

	try {
		sensor = new(std::nothrow) ultraviolet_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
