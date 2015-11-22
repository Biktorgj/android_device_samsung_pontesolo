/*
 * accel_sensor_hal
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
#include <accel_sensor_hal.h>
#include <sys/ioctl.h>
#include <poll.h>

using std::ifstream;

#define GRAVITY 9.80665
#define G_TO_MG 1000
#define RAW_DATA_TO_G_UNIT(X) (((float)(X))/((float)G_TO_MG))
#define RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(X) (GRAVITY * (RAW_DATA_TO_G_UNIT(X)))

#define MIN_RANGE(RES) (-((1 << (RES))/2))
#define MAX_RANGE(RES) (((1 << (RES))/2)-1)

#define SENSOR_TYPE_ACCEL		"ACCEL"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_RAW_DATA_UNIT	"RAW_DATA_UNIT"
#define ELEMENT_RESOLUTION		"RESOLUTION"

#define ATTR_VALUE				"value"

#define INPUT_NAME	"accelerometer_sensor"
#define ACCEL_SENSORHUB_POLL_NODE_NAME "accel_poll_delay"

accel_sensor_hal::accel_sensor_hal()
: m_x(-1)
, m_y(-1)
, m_z(-1)
, m_node_handle(-1)
, m_polling_interval(POLL_1HZ_MS)
, m_fired_time(0)
{
	const string sensorhub_interval_node_name = "accel_poll_delay";
	csensor_config &config = csensor_config::get_instance();

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_ACCEL, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;
	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_ACCEL;
	query.key = "accelerometer_sensor";
	query.iio_enable_node_name = "accel_enable";
	query.sensorhub_interval_node_name = sensorhub_interval_node_name;

	if (!get_node_info(query, info)) {
		ERR("Failed to get node info");
		throw ENXIO;
	}

	show_node_info(info);

	m_method = info.method;
	m_data_node = info.data_node_path;
	m_enable_node = info.enable_node_path;
	m_interval_node = info.interval_node_path;

	if (!config.get(SENSOR_TYPE_ACCEL, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_ACCEL, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n",m_chip_name.c_str());

	long resolution;

	if (!config.get(SENSOR_TYPE_ACCEL, m_model_id, ELEMENT_RESOLUTION, resolution)) {
		ERR("[RESOLUTION] is empty\n");
		throw ENXIO;
	}

	m_resolution = (int)resolution;

	INFO("m_resolution = %d\n",m_resolution);

	double raw_data_unit;

	if (!config.get(SENSOR_TYPE_ACCEL, m_model_id, ELEMENT_RAW_DATA_UNIT, raw_data_unit)) {
		ERR("[RAW_DATA_UNIT] is empty\n");
		throw ENXIO;
	}

	m_raw_data_unit = (float)(raw_data_unit);
	INFO("m_raw_data_unit = %f\n", m_raw_data_unit);

	if ((m_node_handle = open(m_data_node.c_str(), O_RDWR)) < 0) {
		ERR("accel handle open fail for accel processor, error:%s\n", strerror(errno));
		throw ENXIO;
	}

	if (m_method == INPUT_EVENT_METHOD) {
		int clockId = CLOCK_MONOTONIC;
		if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0)
			ERR("Fail to set monotonic timestamp for %s", m_data_node.c_str());

		update_value = [=](bool wait) {
			return this->update_value_input_event(wait);
		};
	} else {
		if (!info.buffer_length_node_path.empty())
			set_node_value(info.buffer_length_node_path, 480);

		if (!info.buffer_enable_node_path.empty())
			set_node_value(info.buffer_enable_node_path, 1);

		update_value = [=](bool wait) {
			return this->update_value_iio(wait);
		};
	}

	INFO("accel_sensor is created!\n");
}

accel_sensor_hal::~accel_sensor_hal()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("accel_sensor is destroyed!\n");
}

string accel_sensor_hal::get_model_id(void)
{
	return m_model_id;
}

sensor_type_t accel_sensor_hal::get_type(void)
{
	return ACCELEROMETER_SENSOR;
}

bool accel_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_ACCELEROMETER_ENABLE_BIT);
	set_interval(m_polling_interval);

	m_fired_time = 0;
	INFO("Accel sensor real starting");
	return true;
}

bool accel_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_ACCELEROMETER_ENABLE_BIT);

	INFO("Accel sensor real stopping");
	return true;
}

bool accel_sensor_hal::set_interval(unsigned long val)
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


bool accel_sensor_hal::update_value_input_event(bool wait)
{
	int accel_raw[3] = {0,};
	bool x,y,z;
	int read_input_cnt = 0;
	const int INPUT_MAX_BEFORE_SYN = 10;
	unsigned long long fired_time = 0;
	bool syn = false;

	x = y = z = false;

	struct input_event accel_input;
	DBG("accel event detection!");

	while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
		int len = read(m_node_handle, &accel_input, sizeof(accel_input));
		if (len != sizeof(accel_input)) {
			ERR("accel_file read fail, read_len = %d\n",len);
			return false;
		}

		++read_input_cnt;

		if (accel_input.type == EV_REL) {
			switch (accel_input.code) {
			case REL_X:
				accel_raw[0] = (int)accel_input.value;
				x = true;
				break;
			case REL_Y:
				accel_raw[1] = (int)accel_input.value;
				y = true;
				break;
			case REL_Z:
				accel_raw[2] = (int)accel_input.value;
				z = true;
				break;
			default:
				ERR("accel_input event[type = %d, code = %d] is unknown.", accel_input.type, accel_input.code);
				return false;
				break;
			}
		} else if (accel_input.type == EV_SYN) {
			syn = true;
			fired_time = sensor_hal::get_timestamp(&accel_input.time);
		} else {
			ERR("accel_input event[type = %d, code = %d] is unknown.", accel_input.type, accel_input.code);
			return false;
		}
	}

	if (syn == false) {
		ERR("EV_SYN didn't come until %d inputs had come", read_input_cnt);
		return false;
	}

	AUTOLOCK(m_value_mutex);

	if (x)
		m_x =  accel_raw[0];
	if (y)
		m_y =  accel_raw[1];
	if (z)
		m_z =  accel_raw[2];

	m_fired_time = fired_time;

	DBG("m_x = %d, m_y = %d, m_z = %d, time = %lluus", m_x, m_y, m_z, m_fired_time);

	return true;
}


bool accel_sensor_hal::update_value_iio(bool wait)
{
	const int READ_LEN = 14;
	char data[READ_LEN] = {0,};

	struct pollfd pfd;

	pfd.fd = m_node_handle;
	pfd.events = POLLIN | POLLERR;
	pfd.revents = 0;

	int ret = poll(&pfd, 1, -1);

	if (ret == -1) {
		ERR("poll error:%s m_node_handle:d", strerror(errno), m_node_handle);
		return false;
	} else if (!ret) {
		ERR("poll timeout m_node_handle:%d", m_node_handle);
		return false;
	}

	if (pfd.revents & POLLERR) {
		ERR("poll exception occurred! m_node_handle:%d", m_node_handle);
		return false;
	}

	if (!(pfd.revents & POLLIN)) {
		ERR("poll nothing to read! m_node_handle:%d, pfd.revents = %d", m_node_handle, pfd.revents);
		return false;
	}

	int len = read(m_node_handle, data, sizeof(data));

	if (len != sizeof(data)) {
		ERR("Failed to read data, m_node_handle:%d read_len:%d", m_node_handle, len);
		return false;
	}

	AUTOLOCK(m_value_mutex);

	m_x = *((short *)(data));
	m_y = *((short *)(data + 2));
	m_z = *((short *)(data + 4));

	m_fired_time = *((long long*)(data + 6));

	INFO("m_x = %d, m_y = %d, m_z = %d, time = %lluus", m_x, m_y, m_z, m_fired_time);

	return true;

}

bool accel_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int accel_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	AUTOLOCK(m_value_mutex);

	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_fired_time;
	data.value_count = 3;
	data.values[0] = m_x;
	data.values[1] = m_y;
	data.values[2] = m_z;

	return 0;
}


bool accel_sensor_hal::get_properties(sensor_properties_t &properties)
{
	properties.name = m_chip_name;
	properties.vendor = m_vendor;
	properties.min_range = MIN_RANGE(m_resolution)* RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(m_raw_data_unit);
	properties.max_range = MAX_RANGE(m_resolution)* RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(m_raw_data_unit);
	properties.min_interval = 1;
	properties.resolution = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(m_raw_data_unit);
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	return true;
}

extern "C" sensor_module* create(void)
{
	accel_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) accel_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
