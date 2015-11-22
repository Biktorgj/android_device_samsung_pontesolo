/*
 * uncal_geo_sensor_hal
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
#include <uncal_geo_sensor_hal.h>
#include <sys/ioctl.h>
#include <fstream>

using std::ifstream;

#define SENSOR_TYPE_UNCAL_MAGNETIC	"MAGNETIC"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_RAW_DATA_UNIT	"RAW_DATA_UNIT"
#define ELEMENT_MIN_RANGE		"MIN_RANGE"
#define ELEMENT_MAX_RANGE		"MAX_RANGE"
#define ATTR_VALUE				"value"

uncal_geo_sensor_hal::uncal_geo_sensor_hal()
: m_x(0)
, m_y(0)
, m_z(0)
, m_x_offset(0)
, m_y_offset(0)
, m_z_offset(0)
, m_polling_interval(POLL_1HZ_MS)
, m_fired_time(0)
, m_node_handle(-1)
{
	const string sensorhub_interval_node_name = "uncal_mag_poll_delay";
	csensor_config &config = csensor_config::get_instance();

	node_info_query query;
	node_info info;

	if (!find_model_id(SENSOR_TYPE_UNCAL_MAGNETIC, m_model_id)) {
		ERR("Failed to find model id");
		throw ENXIO;

	}

	query.sensorhub_controlled = m_sensorhub_controlled = is_sensorhub_controlled(sensorhub_interval_node_name);
	query.sensor_type = SENSOR_TYPE_UNCAL_MAGNETIC;
	query.key = "uncal_geomagnetic_sensor";
	query.iio_enable_node_name = "uncal_geomagnetic_enable";
	query.sensorhub_interval_node_name = sensorhub_interval_node_name;

	if (!get_node_info(query, info)) {
		ERR("Failed to get node info");
		throw ENXIO;
	}

	show_node_info(info);

	m_data_node = info.data_node_path;
	m_enable_node = info.enable_node_path;
	m_interval_node = info.interval_node_path;

	if (!config.get(SENSOR_TYPE_UNCAL_MAGNETIC, m_model_id, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_UNCAL_MAGNETIC, m_model_id, ELEMENT_NAME, m_chip_name)) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n",m_chip_name.c_str());

	double min_range;

	if (!config.get(SENSOR_TYPE_UNCAL_MAGNETIC, m_model_id, ELEMENT_MIN_RANGE, min_range)) {
		ERR("[MIN_RANGE] is empty\n");
		throw ENXIO;
	}

	m_min_range = (float)min_range;
	INFO("m_min_range = %f\n",m_min_range);

	double max_range;

	if (!config.get(SENSOR_TYPE_UNCAL_MAGNETIC, m_model_id, ELEMENT_MAX_RANGE, max_range)) {
		ERR("[MAX_RANGE] is empty\n");
		throw ENXIO;
	}

	m_max_range = (float)max_range;
	INFO("m_max_range = %f\n",m_max_range);

	double raw_data_unit;

	if (!config.get(SENSOR_TYPE_UNCAL_MAGNETIC, m_model_id, ELEMENT_RAW_DATA_UNIT, raw_data_unit)) {
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

	INFO("m_raw_data_unit = %f\n", m_raw_data_unit);
	INFO("uncal_geo_sensor_hal is created!\n");

}

uncal_geo_sensor_hal::~uncal_geo_sensor_hal()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("uncal_geo_sensor is destroyed!\n");
}

string uncal_geo_sensor_hal::get_model_id(void)
{
	return m_model_id;
}

sensor_type_t uncal_geo_sensor_hal::get_type(void)
{
	return UNCAL_GEOMAGNETIC_SENSOR;
}

bool uncal_geo_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, true, SENSORHUB_GEOMAGNETIC_UNCALIB_ENABLE_BIT);
	set_interval(m_polling_interval);

	m_fired_time = 0;
	INFO("Geo sensor real starting");
	return true;
}

bool uncal_geo_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	set_enable_node(m_enable_node, m_sensorhub_controlled, false, SENSORHUB_GEOMAGNETIC_UNCALIB_ENABLE_BIT);

	INFO("Uncal Geo sensor real stopping");
	return true;
}

bool uncal_geo_sensor_hal::set_interval(unsigned long val)
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

bool uncal_geo_sensor_hal::update_value(bool wait)
{
	int uncal_geo_raw[6] = {0,};
	bool x,y,z,x_offset,y_offset,z_offset;
	int read_input_cnt = 0;
	const int INPUT_MAX_BEFORE_SYN = 10;
	unsigned long long fired_time = 0;
	bool syn = false;

	x = y = z = x_offset = y_offset = z_offset = false;

	struct input_event uncal_geoinput;
	DBG("uncal geo event detection!");

	while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
		int len = read(m_node_handle, &uncal_geoinput, sizeof(uncal_geoinput));
		if (len != sizeof(uncal_geoinput)) {
			ERR("uncal_geofile read fail, read_len = %d\n",len);
			return false;
		}

		++read_input_cnt;

		if (uncal_geoinput.type == EV_REL) {
			switch (uncal_geoinput.code) {
				case REL_RX:
					uncal_geo_raw[0] = (int)uncal_geoinput.value;
					x = true;
					break;
				case REL_RY:
					uncal_geo_raw[1] = (int)uncal_geoinput.value;
					y = true;
					break;
				case REL_RZ:
					uncal_geo_raw[2] = (int)uncal_geoinput.value;
					z = true;
					break;
				case REL_HWHEEL:
					uncal_geo_raw[3] = (int)uncal_geoinput.value;
					x_offset= true;
					break;
				case REL_DIAL:
					uncal_geo_raw[4] = (int)uncal_geoinput.value;
					y_offset= true;
					break;
				case REL_WHEEL:
					uncal_geo_raw[5] = (int)uncal_geoinput.value;
					z_offset= true;
					break;
				default:
					ERR("uncal_geoinput event[type = %d, code = %d] is unknown.", uncal_geoinput.type, uncal_geoinput.code);
					return false;
					break;
			}
		} else if (uncal_geoinput.type == EV_SYN) {
			syn = true;
			fired_time = get_timestamp(&uncal_geoinput.time);
		} else {
			ERR("uncal_geoinput event[type = %d, code = %d] is unknown.", uncal_geoinput.type, uncal_geoinput.code);
			return false;
		}
	}

	AUTOLOCK(m_value_mutex);

	if (x)
		m_x =  uncal_geo_raw[0];
	if (y)
		m_y =  uncal_geo_raw[1];
	if (z)
		m_z =  uncal_geo_raw[2];
	if (x_offset)
		m_x_offset=  uncal_geo_raw[3];
	if (y_offset)
		m_y_offset=  uncal_geo_raw[4];
	if (z_offset)
		m_z_offset=  uncal_geo_raw[5];


	m_fired_time = fired_time;

	DBG("m_x = %d, m_y = %d, m_z = %d, m_x_offset = %d, m_y_offset = %d, m_z_offset = %d, time = %lluus", m_x, m_y, m_z, m_x_offset, m_y_offset, m_z_offset, m_fired_time);

	return true;
}


bool uncal_geo_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int uncal_geo_sensor_hal::get_sensor_data(sensor_data_t &data)
{
	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_fired_time;
	data.value_count = 6;
	data.values[0] = (float)m_x;
	data.values[1] = (float)m_y;
	data.values[2] = (float)m_z;
	data.values[3] = (float)m_x_offset;
	data.values[4] = (float)m_y_offset;
	data.values[5] = (float)m_z_offset;
	return 0;
}

bool uncal_geo_sensor_hal::get_properties(sensor_properties_t &properties)
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
	uncal_geo_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) uncal_geo_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
