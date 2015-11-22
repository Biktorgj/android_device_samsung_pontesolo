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

#include <proxi_sensor.h>
#include <sensor_plugin_loader.h>


#define SENSOR_NAME "PROXI_SENSOR"

proxi_sensor::proxi_sensor()
: m_sensor_hal(NULL)
, m_state(PROXIMITY_STATE_FAR)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(PROXIMITY_EVENT_CHANGE_STATE);
	register_supported_event(PROXIMITY_EVENT_STATE_REPORT_ON_TIME);
	register_supported_event(PROXIMITY_EVENT_DISTANCE_DATA_REPORT_ON_TIME);

	physical_sensor::set_poller(proxi_sensor::working, this);
}

proxi_sensor::~proxi_sensor()
{
	INFO("proxi_sensor is destroyed!\n");
}

bool proxi_sensor::init()
{
	m_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(PROXIMITY_SENSOR);

	if (!m_sensor_hal) {
		ERR("cannot load sensor_hal[%s]", sensor_base::get_name());
		return false;
	}

	INFO("%s is created!\n", sensor_base::get_name());
	return true;
}

sensor_type_t proxi_sensor::get_type(void)
{
	return PROXIMITY_SENSOR;
}

bool proxi_sensor::working(void *inst)
{
	proxi_sensor *sensor = (proxi_sensor*)inst;
	return sensor->process_event();
}

bool proxi_sensor::process_event(void)
{
	sensor_event_t event;
	int state;

	if (!m_sensor_hal->is_data_ready(true))
		return true;

	m_sensor_hal->get_sensor_data(event.data);

	AUTOLOCK(m_client_info_mutex);
	AUTOLOCK(m_mutex);

	event.sensor_id = get_id();
	if (get_client_cnt(PROXIMITY_EVENT_DISTANCE_DATA_REPORT_ON_TIME)) {
		event.event_type = PROXIMITY_EVENT_DISTANCE_DATA_REPORT_ON_TIME;
		raw_to_base(event.data);
		push(event);
	}

	state = event.data.values[0];

	if (m_state != state) {
		AUTOLOCK(m_value_mutex);
		m_state = state;

		if (get_client_cnt(PROXIMITY_EVENT_CHANGE_STATE)) {
			event.event_type = PROXIMITY_EVENT_CHANGE_STATE;
			raw_to_base(event.data);
			push(event);
		}
	}

	return true;
}

bool proxi_sensor::on_start(void)
{
	if (!m_sensor_hal->enable()) {
		ERR("m_sensor_hal start fail\n");
		return false;
	}

	return start_poll();
}

bool proxi_sensor::on_stop(void)
{
	if (!m_sensor_hal->disable()) {
		ERR("m_sensor_hal stop fail\n");
		return false;
	}

	return stop_poll();
}

bool proxi_sensor::get_properties(sensor_properties_t &properties)
{
	m_sensor_hal->get_properties(properties);

	properties.min_range = properties.min_range * 5;
	properties.max_range = properties.max_range * 5;

	return true;
}

int proxi_sensor::get_sensor_data(unsigned int type, sensor_data_t &data)
{
	int state;

	if ((type != PROXIMITY_BASE_DATA_SET) && (type != PROXIMITY_DISTANCE_DATA_SET))
		return -1;

	state = m_sensor_hal->get_sensor_data(data);

	if (state < 0) {
		ERR("m_sensor_hal get struct_data fail\n");
		return -1;
	}

	raw_to_base(data);

	return 0;
}

void proxi_sensor::raw_to_base(sensor_data_t &data)
{
	data.values[0] = (float)(data.values[0] * 5);
	data.value_count = 1;
}

extern "C" sensor_module* create(void)
{
	proxi_sensor *sensor;

	try {
		sensor = new(std::nothrow) proxi_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
