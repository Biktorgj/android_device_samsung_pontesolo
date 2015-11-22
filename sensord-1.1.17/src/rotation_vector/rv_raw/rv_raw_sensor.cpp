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

#include <rv_raw_sensor.h>
#include <sensor_plugin_loader.h>

#define SENSOR_NAME "RV_RAW_SENSOR"

rv_raw_sensor::rv_raw_sensor()
: m_sensor_hal(NULL)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(RV_RAW_EVENT_RAW_DATA_REPORT_ON_TIME);
	register_supported_event(RV_RAW_EVENT_CALIBRATION_NEEDED);

	physical_sensor::set_poller(rv_raw_sensor::working, this);
}

rv_raw_sensor::~rv_raw_sensor()
{
	INFO("rv_raw_sensor is destroyed!\n");
}

bool rv_raw_sensor::init()
{
	m_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(RV_RAW_SENSOR);

	if (!m_sensor_hal) {
		ERR("cannot load sensor_hal[%s]", sensor_base::get_name());
		return false;
	}

	sensor_properties_t properties;

	if (!m_sensor_hal->get_properties(properties)) {
		ERR("sensor->get_properties() is failed!\n");
		return false;
	}

	set_privilege(SENSOR_PRIVILEGE_INTERNAL);

	INFO("%s is created!\n", sensor_base::get_name());

	return true;
}

sensor_type_t rv_raw_sensor::get_type(void)
{
	return RV_RAW_SENSOR;
}

bool rv_raw_sensor::working(void *inst)
{
	rv_raw_sensor *sensor = (rv_raw_sensor*)inst;
	return sensor->process_event();;
}

bool rv_raw_sensor::process_event(void)
{
	sensor_event_t event;

	if (!m_sensor_hal->is_data_ready(true))
		return true;

	m_sensor_hal->get_sensor_data(event.data);

	AUTOLOCK(m_client_info_mutex);
	AUTOLOCK(m_mutex);

	if (get_client_cnt(RV_RAW_EVENT_RAW_DATA_REPORT_ON_TIME)) {
		event.sensor_id = get_id();
		event.event_type = RV_RAW_EVENT_RAW_DATA_REPORT_ON_TIME;
		push(event);
	}

	return true;
}

bool rv_raw_sensor::on_start(void)
{
	if (!m_sensor_hal->enable()) {
		ERR("m_sensor_hal start fail\n");
		return false;
	}

	return start_poll();
}

bool rv_raw_sensor::on_stop(void)
{
	if (!m_sensor_hal->disable()) {
		ERR("m_sensor_hal stop fail\n");
		return false;
	}

	return stop_poll();
}

bool rv_raw_sensor::get_properties(sensor_properties_t &properties)
{
	return m_sensor_hal->get_properties(properties);
}

int rv_raw_sensor::get_sensor_data(unsigned int type, sensor_data_t &data)
{
	int state;

	if (type != RV_RAW_BASE_DATA_SET)
		return -1;

	state = m_sensor_hal->get_sensor_data(data);

	if (state < 0) {
		ERR("m_sensor_hal get struct_data fail\n");
		return -1;
	}

	return 0;
}

bool rv_raw_sensor::set_interval(unsigned long interval)
{
	AUTOLOCK(m_mutex);

	INFO("Polling interval is set to %dms", interval);

	return m_sensor_hal->set_interval(interval);
}

extern "C" sensor_module* create(void)
{
	rv_raw_sensor *sensor;

	try {
		sensor = new(std::nothrow) rv_raw_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
