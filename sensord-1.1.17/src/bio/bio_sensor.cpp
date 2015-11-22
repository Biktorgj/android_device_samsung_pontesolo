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

#include <bio_sensor.h>
#include <sensor_plugin_loader.h>


#define SENSOR_NAME "BIO_SENSOR"

bio_sensor::bio_sensor()
: m_sensor_hal(NULL)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(BIO_EVENT_RAW_DATA_REPORT_ON_TIME);

	physical_sensor::set_poller(bio_sensor::working, this);
}

bio_sensor::~bio_sensor()
{
	INFO("bio_sensor is destroyed!\n");
}

bool bio_sensor::init()
{
	m_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(BIO_SENSOR);

	if (!m_sensor_hal) {
		ERR("cannot load sensor_hal[%s]", sensor_base::get_name());
		return false;
	}

	set_privilege(SENSOR_PRIVILEGE_INTERNAL);
	set_permission(SENSOR_PERMISSION_BIO);

	INFO("%s is created!\n", sensor_base::get_name());

	return true;
}

sensor_type_t bio_sensor::get_type(void)
{
	return BIO_SENSOR;
}

bool bio_sensor::working(void *inst)
{
	bio_sensor *sensor = (bio_sensor*)inst;
	return sensor->process_event();
}

bool bio_sensor::process_event(void)
{
	sensor_event_t event;

	if (!m_sensor_hal->is_data_ready(true))
		return true;

	m_sensor_hal->get_sensor_data(event.data);

	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(BIO_EVENT_RAW_DATA_REPORT_ON_TIME)) {
		event.sensor_id = get_id();
		event.event_type = BIO_EVENT_RAW_DATA_REPORT_ON_TIME;
		push(event);
	}

	return true;
}

bool bio_sensor::on_start(void)
{
	if (!m_sensor_hal->enable()) {
		ERR("m_sensor_hal start fail\n");
		return false;
	}

	return start_poll();
}

bool bio_sensor::on_stop(void)
{
	if (!m_sensor_hal->disable()) {
		ERR("m_sensor_hal stop fail\n");
		return false;
	}

	return stop_poll();
}

bool bio_sensor::get_properties(sensor_properties_t &properties)
{
	return m_sensor_hal->get_properties(properties);
}

int bio_sensor::get_sensor_data(unsigned int type, sensor_data_t &data)
{
	int ret;

	if (type != BIO_BASE_DATA_SET)
		return -1;

	ret = m_sensor_hal->get_sensor_data(data);

	if (ret < 0)
		return -1;

	return 0;
}

bool bio_sensor::set_interval(unsigned long interval)
{
	AUTOLOCK(m_mutex);

	INFO("Polling interval is set to %dms", interval);

	return m_sensor_hal->set_interval(interval);
}


extern "C" sensor_module* create(void)
{
	bio_sensor *sensor;

	try {
		sensor = new(std::nothrow) bio_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
