/*
 * humidity_sensor_hal
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
#include <linux/input.h>
#include <csensor_config.h>
#include <humidity_sensor_hal.h>
#include <sys/ioctl.h>


#define SENSOR_TYPE_HUMIDITY	"HUMIDITY"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_RAW_DATA_UNIT	"RAW_DATA_UNIT"

#define HUM_INPUT_NAME					"humidity_sensor"
#define HUM_IIO_ENABLE_NODE_NAME		"humi_enable"
#define HUM_SENSORHUB_POLL_NODE_NAME 	"humi_poll_delay"

humidity_sensor_hal::humidity_sensor_hal()
: m_humidity(0)
, m_node_handle(-1)
, m_polling_interval(POLL_1HZ_MS)
, m_fired_time(0)
{
	const string sensorhub_interval_node_name = HUM_SENSORHUB_POLL_NODE_NAME;

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_HUMIDITY, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;
	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_HUMIDITY;
	query.key = HUM_INPUT_NAME;
	query.iio_enable_node_name = HUM_IIO_ENABLE_NODE_NAME;
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

	if (!config.get(SENSOR_TYPE_HUMIDITY, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty");
		throw ENXIO;
	}

	if (!config.get(SENSOR_TYPE_HUMIDITY, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	double raw_data_unit;

	if (!config.get(SENSOR_TYPE_HUMIDITY, m_model_id, ELEMENT_RAW_DATA_UNIT, raw_data_unit)) {
		ERR("[RAW_DATA_UNIT] is empty\n");
		throw ENXIO;
	}
	m_raw_data_unit = (float)(raw_data_unit);

	if ((m_node_handle = open(m_data_node.c_str(),O_RDWR)) < 0) {
		ERR("Failed to open handle(%d)", m_node_handle);
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0)
		ERR("Fail to set monotonic timestamp for %s", m_data_node.c_str());

	INFO("m_vendor = %s", m_vendor.c_str());
	INFO("m_chip_name = %s", m_chip_name.c_str());
	INFO("m_raw_data_unit = %f", m_raw_data_unit);

	INFO("humidity_sensor_hal is created!");
}

humidity_sensor_hal::~humidity_sensor_hal()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("humidity_sensor_hal is destroyed!\n");
}

string humidity_sensor_hal::get_model_id(void)
{
	return m_model_id;
}

sensor_type_t humidity_sensor_hal::get_type(void)
{
	return HUMIDITY_SENSOR;
}

bool humidity_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_TEMPERATURE_HUMIDITY_ENABLE_BIT);
	set_interval(m_polling_interval);

	m_fired_time = 0;
	INFO("Humidity sensor real starting");
	return true;
}

bool humidity_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_TEMPERATURE_HUMIDITY_ENABLE_BIT);

	INFO("Humidity sensor real stopping");
	return true;
}

bool humidity_sensor_hal::set_interval(unsigned long val)
{
	unsigned long long polling_interval_ns;

	AUTOLOCK(m_mutex);

	polling_interval_ns = ((unsigned long long)(val) * 1000llu * 1000llu);

	if (!set_node_value(m_interval_node, polling_interval_ns)) {
		ERR("Failed to set polling node: %s\n", m_interval_node.c_str());
		return false;
	}

	INFO("Interval is changed from %dms to %dms]", m_polling_interval, val);
	m_polling_interval = val;
	return true;
}

bool humidity_sensor_hal::update_value(bool wait)
{
	int humidity_raw = 0;
	bool humidity = false;
	int read_input_cnt = 0;
	const int INPUT_MAX_BEFORE_SYN = 10;
	unsigned long long fired_time = 0;
	bool syn = false;

	struct input_event humidity_event;
	DBG("humidity event detection!");

	while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
		int len = read(m_node_handle, &humidity_event, sizeof(humidity_event));
		if (len != sizeof(humidity_event)) {
			ERR("humidity_file read fail, read_len = %d\n",len);
			return false;
		}

		++read_input_cnt;

		if (humidity_event.type == EV_REL) {
			switch (humidity_event.code) {
				case REL_DIAL:
					humidity_raw = (int)humidity_event.value;
					humidity = true;
					break;
				default:
					ERR("humidity_event event[type = %d, code = %d] is unknown.", humidity_event.type, humidity_event.code);
					return false;
			}
		} else if (humidity_event.type == EV_SYN) {
			syn = true;
			fired_time = sensor_hal::get_timestamp(&humidity_event.time);
		} else {
			ERR("humidity_event event[type = %d, code = %d] is unknown.", humidity_event.type, humidity_event.code);
			return false;
		}
	}

	if (syn == false) {
		ERR("EV_SYN didn't come until %d inputs had come", read_input_cnt);
		return false;
	}

	AUTOLOCK(m_value_mutex);

	if (humidity)
		m_humidity = humidity_raw;

	m_fired_time = fired_time;

	DBG("m_humidity = %d, time = %lluus", m_humidity, m_fired_time);

	return true;
}

bool humidity_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int humidity_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	AUTOLOCK(m_value_mutex);
	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_fired_time ;
	data.value_count = 1;
	data.values[0] = (float) m_humidity;

	return 0;
}


bool humidity_sensor_hal::get_properties(sensor_properties_t &properties)
{
	properties.name = m_chip_name;
	properties.vendor = m_vendor;
	properties.min_range = 0;
	properties.max_range = 100;
	properties.min_interval = 1;
	properties.resolution = m_raw_data_unit;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;

	return true;
}
extern "C" sensor_module* create(void)
{
	humidity_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) humidity_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
