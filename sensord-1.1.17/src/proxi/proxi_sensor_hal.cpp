/*
 * proxi_sensor_hal
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
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/input.h>
#include <csensor_config.h>
#include <proxi_sensor_hal.h>
#include <sys/ioctl.h>
#include <fstream>

using std::ifstream;

#define SENSOR_TYPE_PROXI		"PROXI"
#define ELEMENT_NAME 			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ATTR_VALUE 				"value"

#define PROXI_CODE	0x0019

proxi_sensor_hal::proxi_sensor_hal()
: m_state(PROXIMITY_STATE_FAR)
, m_fired_time(0)
, m_node_handle(-1)
{
	const string sensorhub_interval_node_name = "prox_poll_delay";
	csensor_config &config = csensor_config::get_instance();

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_PROXI, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;

	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_PROXI;
	query.key = "proximity_sensor";
	query.iio_enable_node_name = "proximity_enable";
	query.sensorhub_interval_node_name = sensorhub_interval_node_name;

	if (!get_node_info(query, info)) {
		ERR("Failed to get node info");
		throw ENXIO;
	}

	show_node_info(info);

	m_data_node = info.data_node_path;
	m_enable_node = info.enable_node_path;

	if (!config.get(SENSOR_TYPE_PROXI, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_PROXI, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n",m_chip_name.c_str());

	if ((m_node_handle = open(m_data_node.c_str(), O_RDWR)) < 0) {
		ERR("Proxi handle(%d) open fail", m_node_handle);
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0)
		ERR("Fail to set monotonic timestamp for %s", m_data_node.c_str());

	INFO("Proxi_sensor_hal is created!\n");

}

proxi_sensor_hal::~proxi_sensor_hal()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("Proxi_sensor_hal is destroyed!\n");
}

string proxi_sensor_hal::get_model_id(void)
{
	return m_model_id;
}

sensor_type_t proxi_sensor_hal::get_type(void)
{
	return PROXIMITY_SENSOR;
}

bool proxi_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_PROXIMITY_ENABLE_BIT);

	m_fired_time = sensor_hal::get_timestamp();
	INFO("Proxi sensor real starting");
	return true;
}

bool proxi_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_PROXIMITY_ENABLE_BIT);

	INFO("Proxi sensor real stopping");
	return true;
}

bool proxi_sensor_hal::update_value(bool wait)
{
	struct input_event proxi_event;
	INFO("proxi event detection!");

	int len = read(m_node_handle, &proxi_event, sizeof(proxi_event));

	if (len == -1) {
		DBG("read(m_node_handle) is error:%s.\n", strerror(errno));
		return false;
	}

	DBG("read event,  len : %d , type : %x , code : %x , value : %x", len, proxi_event.type, proxi_event.code, proxi_event.value);
	if ((proxi_event.type == EV_ABS) && (proxi_event.code == PROXI_CODE)) {
		AUTOLOCK(m_value_mutex);
		if (proxi_event.value == PROXIMITY_NODE_STATE_FAR) {
			INFO("PROXIMITY_STATE_FAR state occured\n");
			m_state = PROXIMITY_STATE_FAR;
		} else if (proxi_event.value == PROXIMITY_NODE_STATE_NEAR) {
			INFO("PROXIMITY_STATE_NEAR state occured\n");
			m_state = PROXIMITY_STATE_NEAR;
		} else {
			ERR("PROXIMITY_STATE Unknown: %d\n",proxi_event.value);
			return false;
		}
		m_fired_time = sensor_hal::get_timestamp(&proxi_event.time);
	} else {
		return false;
	}

	return true;
}

bool proxi_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int proxi_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	AUTOLOCK(m_value_mutex);
	data.accuracy = SENSOR_ACCURACY_UNDEFINED;
	data.timestamp = m_fired_time;
	data.value_count = 1;
	data.values[0] = m_state;

	return 0;
}

bool proxi_sensor_hal::get_properties(sensor_properties_t &properties)
{
	properties.name = m_chip_name;
	properties.vendor = m_vendor;
	properties.min_range = 0;
	properties.max_range = 1;
	properties.min_interval = 1;
	properties.resolution = 1;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	return true;
}

extern "C" sensor_module* create(void)
{
	proxi_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) proxi_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
