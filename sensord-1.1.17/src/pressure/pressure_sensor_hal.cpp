/*
 * pressure_sensor_hal
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
#include <pressure_sensor_hal.h>
#include <sys/ioctl.h>
#include <fstream>

using std::ifstream;

#define SENSOR_TYPE_PRESSURE		"PRESSURE"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_RAW_DATA_UNIT	"RAW_DATA_UNIT"
#define ELEMENT_RESOLUTION		"RESOLUTION"
#define ELEMENT_MIN_RANGE		"MIN_RANGE"
#define ELEMENT_MAX_RANGE		"MAX_RANGE"
#define ELEMENT_TEMPERATURE_RESOLUTION	"TEMPERATURE_RESOLUTION"
#define ELEMENT_TEMPERATURE_OFFSET		"TEMPERATURE_OFFSET"
#define ATTR_VALUE				"value"

#define SEA_LEVEL_PRESSURE 101325.0

pressure_sensor_hal::pressure_sensor_hal()
: m_pressure(0)
, m_sea_level_pressure(SEA_LEVEL_PRESSURE)
, m_temperature(0)
, m_polling_interval(POLL_1HZ_MS)
, m_fired_time(0)
, m_node_handle(-1)
{
	const string sensorhub_interval_node_name = "pressure_poll_delay";
	csensor_config &config = csensor_config::get_instance();

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_PRESSURE, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;

	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_PRESSURE;
	query.key = "pressure_sensor";
	query.iio_enable_node_name = "pressure_enable";
	query.sensorhub_interval_node_name = sensorhub_interval_node_name;

	bool error = get_node_info(query, info);

	query.key = "barometer_sensor";
	error |= get_node_info(query, info);

	if (!error) {
		ERR("Failed to get node info");
		throw ENXIO;
	}

	show_node_info(info);

	m_data_node = info.data_node_path;
	m_enable_node = info.enable_node_path;
	m_interval_node = info.interval_node_path;

	if (!config.get(SENSOR_TYPE_PRESSURE, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_PRESSURE, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	INFO("m_chip_name = %s", m_chip_name.c_str());

	double min_range;

	if (!config.get(SENSOR_TYPE_PRESSURE, m_model_id, ELEMENT_MIN_RANGE, min_range)) {
		ERR("[MIN_RANGE] is empty\n");
		throw ENXIO;
	}

	m_min_range = (float)min_range;
	INFO("m_min_range = %f\n",m_min_range);

	double max_range;

	if (!config.get(SENSOR_TYPE_PRESSURE, m_model_id, ELEMENT_MAX_RANGE, max_range)) {
		ERR("[MAX_RANGE] is empty\n");
		throw ENXIO;
	}

	m_max_range = (float)max_range;
	INFO("m_max_range = %f\n",m_max_range);

	double raw_data_unit;

	if (!config.get(SENSOR_TYPE_PRESSURE, m_model_id, ELEMENT_RAW_DATA_UNIT, raw_data_unit)) {
		ERR("[RAW_DATA_UNIT] is empty\n");
		throw ENXIO;
	}

	m_raw_data_unit = (float)(raw_data_unit);
	INFO("m_raw_data_unit = %f\n", m_raw_data_unit);

	if ((m_node_handle = open(m_data_node.c_str(),O_RDWR)) < 0)
		ERR("Failed to open handle(%d)", m_node_handle);

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0)
		ERR("Fail to set monotonic timestamp for %s", m_data_node.c_str());

	INFO("pressure_sensor_hal is created!\n");
}

pressure_sensor_hal::~pressure_sensor_hal()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("pressure_sensor_hal is destroyed!\n");
}

string pressure_sensor_hal::get_model_id(void)
{
	return m_model_id;
}

sensor_type_t pressure_sensor_hal::get_type(void)
{
	return PRESSURE_SENSOR;
}

bool pressure_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_PRESSURE_ENABLE_BIT);

	set_interval(m_polling_interval);

	m_fired_time = 0;
	INFO("Pressure sensor real starting");
	return true;
}

bool pressure_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_PRESSURE_ENABLE_BIT);

	INFO("Pressure sensor real stopping");
	return true;
}

bool pressure_sensor_hal::set_interval(unsigned long val)
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

bool pressure_sensor_hal::update_value(bool wait)
{
	int pressure_raw[3] = {0,};
	bool pressure = false;
	bool sea_level = false;
	bool temperature = false;
	int read_input_cnt = 0;
	const int INPUT_MAX_BEFORE_SYN = 10;
	unsigned long long fired_time = 0;
	bool syn = false;

	struct input_event pressure_event;
	DBG("pressure event detection!");

	while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
		int len = read(m_node_handle, &pressure_event, sizeof(pressure_event));
		if (len != sizeof(pressure_event)) {
			ERR("pressure_file read fail, read_len = %d\n",len);
			return false;
		}

		++read_input_cnt;

		if (pressure_event.type == EV_REL) {
			switch (pressure_event.code) {
				case REL_X:
				case REL_HWHEEL:
					pressure_raw[0] = (int)pressure_event.value;
					pressure = true;
					break;
				case REL_Y:
				case REL_DIAL:
					pressure_raw[1] = (int)pressure_event.value;
					sea_level = true;
					break;
				case REL_Z:
				case REL_WHEEL:
					pressure_raw[2] = (int)pressure_event.value;
					temperature = true;
					break;
				default:
					ERR("pressure_event event[type = %d, code = %d] is unknown.", pressure_event.type, pressure_event.code);
					return false;
					break;
			}
		} else if (pressure_event.type == EV_SYN) {
			syn = true;
			fired_time = sensor_hal::get_timestamp(&pressure_event.time);
		} else {
			ERR("pressure_event event[type = %d, code = %d] is unknown.", pressure_event.type, pressure_event.code);
			return false;
		}
	}

	AUTOLOCK(m_value_mutex);

	if (pressure)
		m_pressure = pressure_raw[0];
	if (sea_level)
		m_sea_level_pressure = pressure_raw[1];
	if (temperature)
		m_temperature = pressure_raw[2];

	m_fired_time = fired_time;

	DBG("m_pressure = %d, sea_level = %d, temperature = %d, time = %lluus", m_pressure, m_sea_level_pressure, m_temperature, m_fired_time);

	return true;
}

bool pressure_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int pressure_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	AUTOLOCK(m_value_mutex);
	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_fired_time ;
	data.value_count = 3;
	data.values[0] = (float) m_pressure;
	data.values[1] = (float) m_sea_level_pressure;
	data.values[2] = (float) m_temperature;

	return 0;
}


bool pressure_sensor_hal::get_properties(sensor_properties_t &properties)
{
	properties.name = m_chip_name;
	properties.vendor = m_vendor;
	properties.min_range = m_min_range;
	properties.max_range = m_max_range;
	properties.min_interval = 1;
	properties.resolution = m_raw_data_unit;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	return true;
}

extern "C" sensor_module* create(void)
{
	pressure_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) pressure_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
