/*
 * light_sensor_hal
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
#include <light_sensor_hal.h>
#include <sys/ioctl.h>
#include <fstream>

using std::ifstream;

#define SENSOR_TYPE_LIGHT		"LIGHT"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_RAW_DATA_UNIT	"RAW_DATA_UNIT"
#define ELEMENT_RESOLUTION		"RESOLUTION"
#define ATTR_VALUE				"value"

#define BIAS	1
#define INVALID_VALUE	-1

light_sensor_hal::light_sensor_hal()
: m_polling_interval(POLL_1HZ_MS)
, m_adc(INVALID_VALUE)
, m_fired_time(0)
, m_node_handle(-1)
{
	const string sensorhub_interval_node_name = "light_poll_delay";
	csensor_config &config = csensor_config::get_instance();

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_LIGHT, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;

	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_LIGHT;
	query.key = "light_sensor";
	query.iio_enable_node_name = "light_enable";
	query.sensorhub_interval_node_name = sensorhub_interval_node_name;

	if (!get_node_info(query, info)) {
		ERR("Failed to get node info");
		throw ENXIO;
	}

	show_node_info(info);

	m_data_node = info.data_node_path;
	m_enable_node = info.enable_node_path;
	m_interval_node = info.interval_node_path;

	if (!config.get(SENSOR_TYPE_LIGHT, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_LIGHT, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n",m_chip_name.c_str());

	if ((m_node_handle = open(m_data_node.c_str(),O_RDWR)) < 0) {
		ERR("Failed to open handle(%d)", m_node_handle);
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0)
		ERR("Fail to set monotonic timestamp for %s", m_data_node.c_str());

	INFO("light_sensor_hal is created!\n");

}

light_sensor_hal::~light_sensor_hal()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("light_sensor_hal is destroyed!\n");
}

string light_sensor_hal::get_model_id(void)
{
	return m_model_id;
}


sensor_type_t light_sensor_hal::get_type(void)
{
	return LIGHT_SENSOR;
}

bool light_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_LIGHT_ENABLE_BIT);
	set_interval(m_polling_interval);

	m_fired_time = 0;
	INFO("Light sensor real starting");
	return true;
}

bool light_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_LIGHT_ENABLE_BIT);

	INFO("Light sensor real stopping");
	return true;
}

bool light_sensor_hal::set_interval(unsigned long val)
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


bool light_sensor_hal::update_value(bool wait)
{
	int adc = -1;

	struct input_event light_event;
	DBG("light event detection!");

	int len = read(m_node_handle, &light_event, sizeof(light_event));
	if (len == -1) {
		DBG("read(m_node_handle) is error:%s.\n", strerror(errno));
		return false;
	}

	if (light_event.type == EV_ABS && light_event.code == ABS_MISC) {
		adc = light_event.value;
	} else if (light_event.type == EV_REL && light_event.code == REL_MISC) {
		adc = light_event.value - BIAS;
	} else if (light_event.type == EV_REL && light_event.code == REL_RX) {
		adc = light_event.value - BIAS;
	} else {
		DBG("light input event[type = %d, code = %d] is unknown.", light_event.type, light_event.code);
		return false;
	}

	DBG("read event, len : %d, type : %x, code : %x, value : %x",
		len, light_event.type, light_event.code, light_event.value);

	DBG("update_value, adc : %d", adc);

	AUTOLOCK(m_value_mutex);
	m_adc = adc;
	m_fired_time = get_timestamp(&light_event.time);

	return true;
}

bool light_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int light_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	AUTOLOCK(m_value_mutex);
	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_fired_time;
	data.value_count = 1;
	data.values[0] = (float) m_adc;

	return 0;
}


bool light_sensor_hal::get_properties(sensor_properties_t &properties)
{
	properties.name = m_chip_name;
	properties.vendor = m_vendor;
	properties.min_range = 0;
	properties.max_range = 65535;
	properties.min_interval = 1;
	properties.resolution = 1;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	return true;
}

extern "C" sensor_module* create(void)
{
	light_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) light_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
