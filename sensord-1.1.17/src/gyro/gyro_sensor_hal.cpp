/*
 * gyro_sensor_hal
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
#include <gyro_sensor_hal.h>
#include <sys/ioctl.h>
#include <fstream>

using std::ifstream;

#define DPS_TO_MDPS 1000
#define MIN_RANGE(RES) (-((1 << (RES))/2))
#define MAX_RANGE(RES) (((1 << (RES))/2)-1)
#define RAW_DATA_TO_DPS_UNIT(X) ((float)(X)/((float)DPS_TO_MDPS))

#define SENSOR_TYPE_GYRO		"GYRO"
#define ELEMENT_NAME 			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_RAW_DATA_UNIT	"RAW_DATA_UNIT"
#define ELEMENT_RESOLUTION 		"RESOLUTION"

#define ATTR_VALUE 				"value"

gyro_sensor_hal::gyro_sensor_hal()
: m_x(-1)
, m_y(-1)
, m_z(-1)
, m_node_handle(-1)
, m_polling_interval(POLL_1HZ_MS)
, m_fired_time(0)
{

	const string sensorhub_interval_node_name = "gyro_poll_delay";
	csensor_config &config = csensor_config::get_instance();

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_GYRO, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;

	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_GYRO;
	query.key = "gyro_sensor";
	query.iio_enable_node_name = "gyro_enable";
	query.sensorhub_interval_node_name = sensorhub_interval_node_name;

	if (!get_node_info(query, info)) {
		ERR("Failed to get node info");
		throw ENXIO;
	}

	show_node_info(info);

	m_data_node = info.data_node_path;
	m_enable_node = info.enable_node_path;
	m_interval_node = info.interval_node_path;

	if (!config.get(SENSOR_TYPE_GYRO, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_GYRO, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n",m_chip_name.c_str());

	long resolution;

	if (!config.get(SENSOR_TYPE_GYRO, m_model_id, ELEMENT_RESOLUTION, resolution)) {
		ERR("[RESOLUTION] is empty\n");
		throw ENXIO;
	}

	m_resolution = (int)resolution;

	double raw_data_unit;

	if (!config.get(SENSOR_TYPE_GYRO, m_model_id, ELEMENT_RAW_DATA_UNIT, raw_data_unit)) {
		ERR("[RAW_DATA_UNIT] is empty\n");
		throw ENXIO;
	}

	m_raw_data_unit = (float)(raw_data_unit);

	if ((m_node_handle = open(m_data_node.c_str(), O_RDWR)) < 0) {
		ERR("gyro handle open fail for gyro processor, error:%s\n", strerror(errno));
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0)
		ERR("Fail to set monotonic timestamp for %s", m_data_node.c_str());

	INFO("m_raw_data_unit = %f\n",m_raw_data_unit);
	INFO("RAW_DATA_TO_DPS_UNIT(m_raw_data_unit) = [%f]",RAW_DATA_TO_DPS_UNIT(m_raw_data_unit));
	INFO("gyro_sensor is created!\n");
}

gyro_sensor_hal::~gyro_sensor_hal()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("gyro_sensor is destroyed!\n");
}

string gyro_sensor_hal::get_model_id(void)
{
	return m_model_id;
}

sensor_type_t gyro_sensor_hal::get_type(void)
{
	return GYROSCOPE_SENSOR;
}

bool gyro_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_GYROSCOPE_ENABLE_BIT);
	set_interval(m_polling_interval);

	m_fired_time = 0;
	INFO("Gyro sensor real starting");
	return true;
}

bool gyro_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_GYROSCOPE_ENABLE_BIT);

	INFO("Gyro sensor real stopping");
	return true;

}

bool gyro_sensor_hal::set_interval(unsigned long val)
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


bool gyro_sensor_hal::update_value(bool wait)
{
	int gyro_raw[3] = {0,};
	bool x,y,z;
	int read_input_cnt = 0;
	const int INPUT_MAX_BEFORE_SYN = 10;
	unsigned long long fired_time = 0;
	bool syn = false;

	x = y = z = false;

	struct input_event gyro_input;
	DBG("gyro event detection!");

	while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
		int len = read(m_node_handle, &gyro_input, sizeof(gyro_input));
		if (len != sizeof(gyro_input)) {
			ERR("gyro_file read fail, read_len = %d\n, %s",len, strerror(errno));
			return false;
		}

		++read_input_cnt;

		if (gyro_input.type == EV_REL) {
			switch (gyro_input.code) {
				case REL_RX:
					gyro_raw[0] = (int)gyro_input.value;
					x = true;
					break;
				case REL_RY:
					gyro_raw[1] = (int)gyro_input.value;
					y = true;
					break;
				case REL_RZ:
					gyro_raw[2] = (int)gyro_input.value;
					z = true;
					break;
				default:
					ERR("gyro_input event[type = %d, code = %d] is unknown.", gyro_input.type, gyro_input.code);
					return false;
					break;
			}
		} else if (gyro_input.type == EV_SYN) {
			syn = true;
			fired_time = sensor_hal::get_timestamp(&gyro_input.time);
		} else {
			ERR("gyro_input event[type = %d, code = %d] is unknown.", gyro_input.type, gyro_input.code);
			return false;
		}
	}

	AUTOLOCK(m_value_mutex);

	if (x)
		m_x =  gyro_raw[0];
	if (y)
		m_y =  gyro_raw[1];
	if (z)
		m_z =  gyro_raw[2];

	m_fired_time = fired_time;

	DBG("m_x = %d, m_y = %d, m_z = %d, time = %lluus", m_x, m_y, m_z, m_fired_time);

	return true;
}

bool gyro_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int gyro_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	AUTOLOCK(m_value_mutex);

	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_fired_time ;
	data.value_count = 3;
	data.values[0] = m_x;
	data.values[1] = m_y;
	data.values[2] = m_z;

	return 0;
}


bool gyro_sensor_hal::get_properties(sensor_properties_t &properties)
{
	properties.name = m_chip_name;
	properties.vendor = m_vendor;
	properties.min_range = MIN_RANGE(m_resolution)* RAW_DATA_TO_DPS_UNIT(m_raw_data_unit);
	properties.max_range = MAX_RANGE(m_resolution)* RAW_DATA_TO_DPS_UNIT(m_raw_data_unit);
	properties.min_interval = 1;
	properties.resolution = RAW_DATA_TO_DPS_UNIT(m_raw_data_unit);
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	return true;

}

extern "C" sensor_module* create(void)
{
	gyro_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) gyro_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
