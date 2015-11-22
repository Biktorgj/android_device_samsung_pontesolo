/*
 * bio_sensor_hal
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
#include <bio_sensor_hal.h>
#include <bio_data_reader_standard.h>
#include <bio_data_reader_adi.h>
#include <sys/ioctl.h>
#include <fstream>
#include <csensor_config.h>

using std::ifstream;

#define SENSOR_TYPE_BIO			"BIO"
#define ELEMENT_NAME 			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_READER			"READER"
#define ATTR_VALUE 				"value"

bio_sensor_hal::bio_sensor_hal()
: m_interval_supported(false)
, m_polling_interval(POLL_1HZ_MS)
, m_reader(NULL)
, m_node_handle(-1)
{
	const string sensorhub_interval_node_name = "hrm_poll_delay";
	csensor_config &config = csensor_config::get_instance();

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_BIO, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;
	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_BIO;
	query.key = "hrm_raw_sensor";
	query.iio_enable_node_name = "hrm_raw_enable";
	query.sensorhub_interval_node_name = sensorhub_interval_node_name;

	if (!get_node_info(query, info)) {
		ERR("Failed to get node info");
		throw ENXIO;
	}

	show_node_info(info);

	m_data_node = info.data_node_path;
	m_enable_node = info.enable_node_path;
	m_interval_node = info.interval_node_path;

	if (access(m_interval_node.c_str(), F_OK) == 0)
		m_interval_supported = true;

	if (!config.get(SENSOR_TYPE_BIO, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_BIO, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n",m_chip_name.c_str());

	string reader;

	if (!config.get(SENSOR_TYPE_BIO, m_model_id, ELEMENT_READER, reader)) {
		ERR("[READER] is empty\n");
		throw ENXIO;
	}

	INFO("reader = %s", reader.c_str());

	if ((m_node_handle = open(m_data_node.c_str(), O_RDWR)) < 0) {
		ERR("Bio handle(%d) open fail", m_node_handle);
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0)
		ERR("Fail to set monotonic timestamp for %s", m_data_node.c_str());

	m_reader = get_reader(reader);

	if (!m_reader) {
		ERR("Not supported HRM sensor: %s", m_model_id.c_str());
		throw ENXIO;
	}

	if (!m_reader->open())
		throw ENXIO;

	INFO("bio_sensor_hal is created!");
}

bio_sensor_hal::~bio_sensor_hal()
{
	delete m_reader;
	close(m_node_handle);
	INFO("bio_sensor_hal is destroyed!\n");
}

string bio_sensor_hal::get_model_id(void)
{
	return m_model_id;
}

sensor_type_t bio_sensor_hal::get_type(void)
{
	return BIO_SENSOR;
}

bool bio_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_BIO_HRM_RAW_ENABLE_BIT);
	if (m_interval_supported)
		set_interval(m_polling_interval);

	m_data.timestamp = 0;

	m_reader->start();
	INFO("Bio sensor real starting");
	return true;
}

bool bio_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_BIO_HRM_RAW_ENABLE_BIT);

	m_reader->stop();
	INFO("Bio sensor real stopping");
	return true;
}

bool bio_sensor_hal::set_interval(unsigned long val)
{
	unsigned long long polling_interval_ns;

	if (!m_interval_supported)
		return true;

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


bool bio_sensor_hal::update_value(bool wait)
{
	sensor_data_t data;

	if (!m_reader->get_data(m_node_handle, data))
		return false;

	AUTOLOCK(m_value_mutex);
	copy_sensor_data(&m_data, &data);
	return true;
}

bool bio_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int bio_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	AUTOLOCK(m_value_mutex);
	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_data.timestamp;
	data.value_count = m_data.value_count;
	memcpy(data.values, m_data.values, m_data.value_count * sizeof(m_data.values[0]));
	return 0;
}

bool bio_sensor_hal::get_properties(sensor_properties_t &properties)
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

bio_data_reader* bio_sensor_hal::get_reader(const string& reader)
{
	static const string ADI("adi");

	if (reader == ADI)
		return new(std::nothrow) bio_data_reader_adi();
	else
		return new(std::nothrow) bio_data_reader_standard();
	return NULL;
}

extern "C" sensor_module* create(void)
{
	bio_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) bio_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
