/*
 * bio_hrm_sensor_hal
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
#include <bio_hrm_sensor_hal.h>
#include <sys/ioctl.h>
#include <fstream>

using std::ifstream;

#define SENSOR_TYPE_BIO_HRM		"BIO_HRM"
#define ELEMENT_NAME 			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_RAW_DATA_UNIT 	"RAW_DATA_UNIT"
#define ATTR_VALUE 				"value"

bio_hrm_sensor_hal::bio_hrm_sensor_hal()
: m_hr(0)
, m_spo2(0)
, m_peek_to_peek(0)
, m_snr(0.0f)
, m_fired_time(0)
, m_raw_data_unit(1)
, m_node_handle(-1)
, m_sensorhub_controlled(false)
{
	const string sensorhub_interval_node_name = "hrm_poll_delay";
	csensor_config &config = csensor_config::get_instance();

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_BIO_HRM, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;
	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_BIO_HRM;
	query.key = "hrm_lib_sensor";
	query.iio_enable_node_name = "hrm_lib_enable";
	query.sensorhub_interval_node_name = sensorhub_interval_node_name;

	if (!get_node_info(query, info)) {
		ERR("Failed to get node info");
		throw ENXIO;
	}

	show_node_info(info);

	m_data_node = info.data_node_path;
	m_enable_node = info.enable_node_path;

	if (!config.get(SENSOR_TYPE_BIO_HRM, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_BIO_HRM, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n",m_chip_name.c_str());

	if ((m_node_handle = open(m_data_node.c_str(), O_RDWR)) < 0) {
		ERR("Faild to open bio_hrm handle(%d)",m_node_handle);
		throw ENXIO;
	}

	double raw_data_unit;

	if (!config.get(SENSOR_TYPE_BIO_HRM, m_model_id, ELEMENT_RAW_DATA_UNIT, raw_data_unit)) {
		ERR("[RAW_DATA_UNIT] is empty\n");
		throw ENXIO;
	}

	m_raw_data_unit = (float)(raw_data_unit);
	INFO("m_raw_data_unit = %f\n", m_raw_data_unit);

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0)
		ERR("Fail to set monotonic timestamp for %s", m_data_node.c_str());

	INFO("bio_hrm_sensor_hal is created!\n");
}

bio_hrm_sensor_hal::~bio_hrm_sensor_hal()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("bio_hrm_sensor_hal is destroyed!\n");
}

string bio_hrm_sensor_hal::get_model_id(void)
{
	return m_model_id;
}

sensor_type_t bio_hrm_sensor_hal::get_type(void)
{
	return BIO_HRM_SENSOR;
}

bool bio_hrm_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_BIO_HRM_LIB_ENABLE_BIT);

	m_fired_time = 0;
	INFO("Bio hrm sensor real starting");
	return true;
}

bool bio_hrm_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_BIO_HRM_LIB_ENABLE_BIT);

	INFO("Bio hrm sensor real stopping");
	return true;
}

bool bio_hrm_sensor_hal::update_value(bool wait)
{
	const float SNR_SIG_FIGS = 10000.0f;
	const int HR_MAX = 300.0f / m_raw_data_unit;
	int bio_hrm_raw[4] = {0,};
	unsigned long long fired_time = 0;
	int read_input_cnt = 0;
	const int INPUT_MAX_BEFORE_SYN = 10;
	bool syn = false;

	struct input_event bio_hrm_input;
	DBG("bio_hrm event detection!");

	while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
		int len = read(m_node_handle, &bio_hrm_input, sizeof(bio_hrm_input));
		if (len != sizeof(bio_hrm_input)) {
			ERR("bio_hrm_file read fail, read_len = %d\n", len);
			return false;
		}

		++read_input_cnt;

		if (bio_hrm_input.type == EV_REL) {
			switch (bio_hrm_input.code) {
			case REL_X:
				bio_hrm_raw[0] = (int)bio_hrm_input.value - 1;
				break;
			case REL_Y:
				bio_hrm_raw[1] = (int)bio_hrm_input.value - 1;
				break;
			case REL_Z:
				bio_hrm_raw[2] = (int)bio_hrm_input.value - 1;
				break;
			default:
				ERR("bio_hrm_input event[type = %d, code = %d] is unknown.", bio_hrm_input.type, bio_hrm_input.code);
				return false;
				break;
			}
		} else if (bio_hrm_input.type == EV_SYN) {
			syn = true;
			fired_time = get_timestamp(&bio_hrm_input.time);
		} else {
			ERR("bio_hrm_input event[type = %d, code = %d] is unknown.", bio_hrm_input.type, bio_hrm_input.code);
			return false;
		}
	}

	if (bio_hrm_raw[0] > HR_MAX) {
		ERR("Drop abnormal HR: %d", bio_hrm_raw[0], HR_MAX);
		return false;
	}

	AUTOLOCK(m_value_mutex);

	m_hr = bio_hrm_raw[0];
	m_peek_to_peek = bio_hrm_raw[1];
	m_snr = ((float)bio_hrm_raw[2] / SNR_SIG_FIGS);
	m_spo2 = 0;
	m_fired_time = fired_time;

	return true;
}

bool bio_hrm_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int bio_hrm_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	AUTOLOCK(m_value_mutex);
	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_fired_time ;
	data.value_count = 4;
	data.values[0] = m_hr;
	data.values[1] = m_spo2;
	data.values[2] = m_peek_to_peek;
	data.values[3] = m_snr;

	return 0;
}


bool bio_hrm_sensor_hal::get_properties(sensor_properties_t &properties)
{
	properties.name = m_chip_name;
	properties.vendor = m_vendor;
	properties.min_range = 0.0f;
	properties.max_range = 1.0f;
	properties.min_interval = 1;
	properties.resolution = m_raw_data_unit;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	return true;
}

extern "C" sensor_module* create(void)
{
	bio_hrm_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) bio_hrm_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
