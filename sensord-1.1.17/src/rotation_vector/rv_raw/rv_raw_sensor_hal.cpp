/*
 * rv_raw_sensor_hal
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
#include <rv_raw_sensor_hal.h>
#include <sys/ioctl.h>
#include <fstream>

using std::ifstream;

#define SENSOR_TYPE_RV_RAW	"ROTATION_VECTOR"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ATTR_VALUE				"value"

rv_raw_sensor_hal::rv_raw_sensor_hal()
: m_quat_a(0)
, m_quat_b(0)
, m_quat_c(0)
, m_quat_d(0)
, m_polling_interval(POLL_1HZ_MS)
{
	const string sensorhub_interval_node_name = "rot_poll_delay";
	csensor_config &config = csensor_config::get_instance();

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_RV_RAW, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;

	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_RV_RAW;
	query.key = "rot_sensor";
	query.iio_enable_node_name = "rot_enable";
	query.sensorhub_interval_node_name = sensorhub_interval_node_name;

	if (!get_node_info(query, info)) {
		ERR("Failed to get node info");
		throw ENXIO;
	}

	show_node_info(info);

	m_data_node = info.data_node_path;
	m_enable_node = info.enable_node_path;
	m_interval_node = info.interval_node_path;

	if (!config.get(SENSOR_TYPE_RV_RAW, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_RV_RAW, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	INFO("m_chip_name = %s", m_chip_name.c_str());

	if ((m_node_handle = open(m_data_node.c_str(),O_RDWR)) < 0) {
		ERR("Failed to open handle(%d)", m_node_handle);
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0)
		ERR("Fail to set monotonic timestamp for %s", m_data_node.c_str());

	INFO("rv_raw_sensor_hal is created!\n");

}

rv_raw_sensor_hal::~rv_raw_sensor_hal()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("rv_raw_sensor_hal is destroyed!\n");
}

string rv_raw_sensor_hal::get_model_id(void)
{
	return m_model_id;
}

sensor_type_t rv_raw_sensor_hal::get_type(void)
{
	return RV_RAW_SENSOR;
}

bool rv_raw_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_ROTATION_VECTOR_ENABLE_BIT);
	set_interval(m_polling_interval);

	m_fired_time = 0;
	INFO("Rotation vector raw sensor real starting");
	return true;
}

bool rv_raw_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_ROTATION_VECTOR_ENABLE_BIT);

	INFO("Rotation vector raw sensor real stopping");
	return true;
}

bool rv_raw_sensor_hal::set_interval(unsigned long val)
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

bool rv_raw_sensor_hal::update_value(bool wait)
{
	int rot_raw[5] = {0,};
	bool quat_a,quat_b,quat_c,quat_d,acc_rot;
	int read_input_cnt = 0;
	const int INPUT_MAX_BEFORE_SYN = 10;
	unsigned long long fired_time = 0;
	bool syn = false;

	quat_a = quat_b = quat_c = quat_d = acc_rot = false;

	struct input_event rot_input;
	DBG("geo event detection!");

	while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
		int len = read(m_node_handle, &rot_input, sizeof(rot_input));
		if (len != sizeof(rot_input)) {
			ERR("rot_file read fail, read_len = %d\n",len);
			return false;
		}

		++read_input_cnt;

		if (rot_input.type == EV_REL) {
			switch (rot_input.code) {
				case REL_X:
					rot_raw[0] = (int)rot_input.value;
					quat_a = true;
					break;
				case REL_Y:
					rot_raw[1] = (int)rot_input.value;
					quat_b = true;
					break;
				case REL_Z:
					rot_raw[2] = (int)rot_input.value;
					quat_c = true;
					break;
				case REL_RX:
					rot_raw[3] = (int)rot_input.value;
					quat_d = true;
					break;
				case REL_RY:
					rot_raw[4] = (int)rot_input.value;
					acc_rot = true;
					break;
				default:
					ERR("rot_input event[type = %d, code = %d] is unknown.", rot_input.type, rot_input.code);
					return false;
					break;
			}
		} else if (rot_input.type == EV_SYN) {
			syn = true;
			fired_time = get_timestamp(&rot_input.time);
		} else {
			ERR("rot_input event[type = %d, code = %d] is unknown.", rot_input.type, rot_input.code);
			return false;
		}
	}

	AUTOLOCK(m_value_mutex);

	if (quat_a)
		m_quat_a =  rot_raw[0];
	if (quat_b)
		m_quat_b =  rot_raw[1];
	if (quat_c)
		m_quat_c =  rot_raw[2];
	if (quat_d)
		m_quat_d =  rot_raw[3];
	if (acc_rot)
		m_accuracy =  rot_raw[4] - 1; /* accuracy bias: -1 */

	m_fired_time = fired_time;

	DBG("m_quat_a = %d, m_quat_a = %d, m_quat_a = %d, m_quat_d = %d, m_accuracy = %d, time = %lluus",
		m_quat_a, m_quat_a, m_quat_a, m_quat_d, m_accuracy, m_fired_time);

	return true;
}


bool rv_raw_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int rv_raw_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	const float QUAT_SIG_FIGS = 1000000.0f;

	data.accuracy = (m_accuracy == 1) ? 0 : m_accuracy; /* hdst 0 and 1 are needed to calibrate */
	data.timestamp = m_fired_time;
	data.value_count = 4;
	data.values[0] = (float)m_quat_a / QUAT_SIG_FIGS;
	data.values[1] = (float)m_quat_b / QUAT_SIG_FIGS;
	data.values[2] = (float)m_quat_c / QUAT_SIG_FIGS;
	data.values[3] = (float)m_quat_d / QUAT_SIG_FIGS;
	return 0;
}

bool rv_raw_sensor_hal::get_properties(sensor_properties_t &properties)
{
	properties.name = m_chip_name;
	properties.vendor = m_vendor;
	properties.min_range = 0;
	properties.max_range = 1200;
	properties.min_interval = 1;
	properties.resolution = 1;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	return true;
}

extern "C" sensor_module* create(void)
{
	rv_raw_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) rv_raw_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
