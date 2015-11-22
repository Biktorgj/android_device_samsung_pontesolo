/*
 * pir_long_sensor_hal
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
#include <pir_long_sensor_hal.h>
#include <sys/ioctl.h>
#include <fstream>

using std::ifstream;

#define SENSOR_TYPE_PIR_LONG	"PIR_LONG"
#define ELEMENT_NAME 			"NAME"
#define ELEMENT_VENDOR			"VENDOR"

#define INPUT_NAME							"pir_long_sensor"
#define IIO_ENABLE_NODE_NAME				"pir_long_enable"
#define PIR_LONG_SENSORHUB_POLL_NODE_NAME 	"pir_long_poll_delay"

pir_long_sensor_hal::pir_long_sensor_hal()
: m_pir_long(PIR_LONG_STATE_ABSENT)
, m_node_handle(-1)
, m_polling_interval(POLL_1HZ_MS)
, m_fired_time(0)
{
	const string sensorhub_interval_node_name = PIR_LONG_SENSORHUB_POLL_NODE_NAME;

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_PIR_LONG, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;
	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_PIR_LONG;
	query.key = INPUT_NAME;
	query.iio_enable_node_name = IIO_ENABLE_NODE_NAME;
	query.sensorhub_interval_node_name = sensorhub_interval_node_name;

	if (!get_node_info(query, info)) {
		ERR("Failed to get node info");
		throw ENXIO;
	}

	show_node_info(info);

	m_data_node = info.data_node_path;
	m_enable_node = info.enable_node_path;
	m_interval_node = info.interval_node_path;

	csensor_config &config = csensor_config::get_instance();

	if (!config.get(SENSOR_TYPE_PIR_LONG, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty");
		throw ENXIO;
	}

	if (!config.get(SENSOR_TYPE_PIR_LONG, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty");
		throw ENXIO;
	}

	if ((m_node_handle = open(m_data_node.c_str(), O_RDWR)) < 0) {
		ERR("PIR_LONG handle(%d) open fail", m_node_handle);
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0)
		ERR("Fail to set monotonic timestamp for %s", m_data_node.c_str());

	INFO("m_vendor = %s", m_vendor.c_str());
	INFO("m_chip_name = %s",m_chip_name.c_str());
	INFO("pir_long_sensor_hal is created!");
}

pir_long_sensor_hal::~pir_long_sensor_hal()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("pir_long_sensor_hal is destroyed!\n");
}

string pir_long_sensor_hal::get_model_id(void)
{
	return m_model_id;
}

sensor_type_t pir_long_sensor_hal::get_type(void)
{
	return PIR_LONG_SENSOR;
}

bool pir_long_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_PIR_ENABLE_BIT);
	set_interval(m_polling_interval);

	m_fired_time = 0;
	INFO("PIR_LONG sensor real starting");
	return true;
}

bool pir_long_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_PIR_ENABLE_BIT);

	INFO("PIR_LONG sensor real stopping");
	return true;
}

bool pir_long_sensor_hal::set_interval(unsigned long val)
{
	unsigned long long polling_interval_ns;

	AUTOLOCK(m_mutex);

	polling_interval_ns = ((unsigned long long)(val) * 1000llu * 1000llu);

	if (!set_node_value(m_interval_node, polling_interval_ns)) {
		ERR("Failed to set polling resource: %s\n", m_interval_node.c_str());
		return false;
	}

	INFO("Interval is changed from %dms to %dms]", m_polling_interval, val);
	m_polling_interval = val;
	return true;
}

bool pir_long_sensor_hal::update_value(bool wait)
{
	int pir_long_raw = 0;
	bool pir_long = false;
	int read_input_cnt = 0;
	const int INPUT_MAX_BEFORE_SYN = 10;
	unsigned long long fired_time = 0;
	bool syn = false;

	struct input_event pir_long_event;
	DBG("pir_long event detection!");

	while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
		int len = read(m_node_handle, &pir_long_event, sizeof(pir_long_event));
		if (len != sizeof(pir_long_event)) {
			ERR("pir_long_file read fail, read_len = %d\n",len);
			return false;
		}

		++read_input_cnt;

		if (pir_long_event.type == EV_ABS) {
			switch (pir_long_event.code) {
				case ABS_DISTANCE:
					pir_long_raw = (int)pir_long_event.value;
					pir_long = true;
					break;
				default:
					ERR("pir_long_event event[type = %d, code = %d] is unknown.", pir_long_event.type, pir_long_event.code);
					return false;
					break;
			}
		} else if (pir_long_event.type == EV_SYN) {
			syn = true;
			fired_time = sensor_hal::get_timestamp(&pir_long_event.time);
		} else {
			ERR("pir_long_event event[type = %d, code = %d] is unknown.", pir_long_event.type, pir_long_event.code);
			return false;
		}
	}

	if (syn == false) {
		ERR("EV_SYN didn't come until %d inputs had come", read_input_cnt);
		return false;
	}

	AUTOLOCK(m_value_mutex);

	if (pir_long)
		m_pir_long = pir_long_raw;

	m_fired_time = fired_time;

	DBG("m_pir_long = %d, time = %lluus", m_pir_long, m_fired_time);

	return true;
}

bool pir_long_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int pir_long_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	AUTOLOCK(m_value_mutex);

	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_fired_time;
	data.value_count = 1;
	data.values[0] = m_pir_long;

	return 0;
}

bool pir_long_sensor_hal::get_properties(sensor_properties_t &properties)
{
	properties.name = m_chip_name;
	properties.vendor = m_vendor;
	properties.min_range = 0;
	properties.max_range = 16383;
	properties.min_interval = 1;
	properties.resolution = 1;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	return true;
}

extern "C" sensor_module* create(void)
{
	pir_long_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) pir_long_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
