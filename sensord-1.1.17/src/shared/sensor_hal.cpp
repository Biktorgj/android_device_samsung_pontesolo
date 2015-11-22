/*
 * libsensord-share
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

#include <sensor_hal.h>
#include <dirent.h>
#include <string.h>
#include <fstream>
#include <csensor_config.h>

using std::ifstream;
using std::fstream;

cmutex sensor_hal::m_shared_mutex;

sensor_hal::sensor_hal()
{
}

sensor_hal::~sensor_hal()
{
}

bool sensor_hal::init(void *data)
{
	return true;
}

bool sensor_hal::set_interval(unsigned long val)
{
	return true;
}

long sensor_hal::set_command(unsigned int cmd, long val)
{
	return -1;
}

int sensor_hal::send_sensorhub_data(const char* data, int data_len)
{
	return -1;
}

int sensor_hal::get_sensor_data(sensor_data_t &data)
{
	return -1;
}

int sensor_hal::get_sensor_data(sensorhub_data_t &data)
{
	return -1;
}

unsigned long long sensor_hal::get_timestamp(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return ((unsigned long long)(t.tv_sec)*1000000000LL + t.tv_nsec) / 1000;
}

unsigned long long sensor_hal::get_timestamp(timeval *t)
{
	if (!t) {
		ERR("t is NULL");
		return 0;
	}

	return ((unsigned long long)(t->tv_sec)*1000000LL +t->tv_usec);
}

bool sensor_hal::is_sensorhub_controlled(const string &key)
{
	string key_node =  string("/sys/class/sensors/ssp_sensor/") + key;

	if (access(key_node.c_str(), F_OK) == 0)
		return true;

	return false;
}

bool sensor_hal::get_node_info(const node_info_query &query, node_info &info)
{
	bool ret = false;
	int method;
	string device_num;

	if (!get_input_method(query.key, method, device_num)) {
		ERR("Failed to get input method for %s", query.key.c_str());
		return false;
	}

	info.method = method;

	if (method == IIO_METHOD) {
		if (query.sensorhub_controlled)
			ret = get_sensorhub_iio_node_info(query.sensorhub_interval_node_name, device_num, info);
		else
			ret = get_iio_node_info(query.iio_enable_node_name, device_num, info);
	} else {
		if (query.sensorhub_controlled)
			ret = get_sensorhub_input_event_node_info(query.sensorhub_interval_node_name, device_num, info);
		else
			ret = get_input_event_node_info(device_num, info);
	}

	return ret;
}


void sensor_hal::show_node_info(node_info &info)
{
	if (info.data_node_path.size())
		INFO("Data node: %s", info.data_node_path.c_str());
	if (info.enable_node_path.size())
		INFO("Enable node: %s", info.enable_node_path.c_str());
	if (info.interval_node_path.size())
		INFO("Interval node: %s", info.interval_node_path.c_str());
	if (info.buffer_enable_node_path.size())
		INFO("Buffer enable node: %s", info.buffer_enable_node_path.c_str());
	if (info.buffer_length_node_path.size())
		INFO("Buffer length node: %s", info.buffer_length_node_path.c_str());
	if (info.trigger_node_path.size())
		INFO("Trigger node: %s", info.trigger_node_path.c_str());
}

bool sensor_hal::get_iio_node_info(const string& enable_node_name, const string& device_num, node_info &info)
{
	const string base_dir = string("/sys/bus/iio/devices/iio:device") + device_num + string("/");

	info.data_node_path = string("/dev/iio:device") + device_num;
	info.enable_node_path = base_dir + enable_node_name;
	info.interval_node_path = base_dir + string("sampling_frequency");
	info.buffer_enable_node_path = base_dir + string("buffer/enable");
	info.buffer_length_node_path = base_dir + string("buffer/length");
	info.trigger_node_path = base_dir + string("trigger/current_trigger");

	return true;
}

bool sensor_hal::get_sensorhub_iio_node_info(const string &interval_node_name, const string& device_num, node_info &info)
{
	const string base_dir = string("/sys/bus/iio/devices/iio:device") + device_num + string("/");
	const string hub_dir = "/sys/class/sensors/ssp_sensor/";

	info.data_node_path = string("/dev/iio:device") + device_num;
	info.enable_node_path = hub_dir + string("enable");
	info.interval_node_path = hub_dir + interval_node_name;
	info.buffer_enable_node_path = base_dir + string("buffer/enable");
	info.buffer_length_node_path = base_dir + string("buffer/length");
	return true;
}

bool sensor_hal::get_input_event_node_info(const string& device_num, node_info &info)
{
	string base_dir;
	string event_num;

	base_dir = string("/sys/class/input/input") + device_num + string("/");

	if (!get_event_num(base_dir, event_num))
		return false;

	info.data_node_path = string("/dev/input/event") + event_num;

	info.enable_node_path = base_dir + string("enable");
	info.interval_node_path = base_dir + string("poll_delay");
	return true;
}

bool sensor_hal::get_sensorhub_input_event_node_info(const string &interval_node_name, const string& device_num, node_info &info)
{
	const string base_dir = "/sys/class/sensors/ssp_sensor/";
	string event_num;

	string input_dir = string("/sys/class/input/input") + device_num + string("/");

	if (!get_event_num(input_dir, event_num))
		return false;

	info.data_node_path = string("/dev/input/event") + event_num;
	info.enable_node_path = base_dir + string("enable");
	info.interval_node_path = base_dir + interval_node_name;
	return true;
}

bool sensor_hal::set_node_value(const string &node_path, int value)
{
	fstream node(node_path, fstream::out);

	if (!node)
		return false;

	node << value;

	return true;
}

bool sensor_hal::set_node_value(const string &node_path, unsigned long long value)
{
	fstream node(node_path, fstream::out);

	if (!node)
		return false;

	node << value;

	return true;
}


bool sensor_hal::get_node_value(const string &node_path, int &value)
{
	fstream node(node_path, fstream::in);

	if (!node)
		return false;

	node >> value;

	return true;
}

bool sensor_hal::set_enable_node(const string &node_path, bool sensorhub_controlled, bool enable, int enable_bit)
{
	int prev_status, status;

	AUTOLOCK(m_shared_mutex);

	if (!get_node_value(node_path, prev_status)) {
		ERR("Failed to get node: %s", node_path.c_str());
		return false;
	}

	int _enable_bit = sensorhub_controlled ? enable_bit : 0;

	if (enable)
		status = prev_status | (1 << _enable_bit);
	else
		status = prev_status & (~(1 << _enable_bit));

	if (!set_node_value(node_path, status)) {
		ERR("Failed to set node: %s", node_path.c_str());
		return false;
	}

	return true;
}


bool sensor_hal::find_model_id(const string &sensor_type, string &model_id)
{
	string dir_path = "/sys/class/sensors/";
	string name_node, name;
	string d_name;
	DIR *dir = NULL;
	struct dirent *dir_entry = NULL;
	bool find = false;

	dir = opendir(dir_path.c_str());
	if (!dir) {
		DBG("Failed to open dir: %s", dir_path.c_str());
		return false;
	}

	while (!find && (dir_entry = readdir(dir))) {
		d_name = string(dir_entry->d_name);

		if ((d_name != ".") && (d_name != "..") && (dir_entry->d_ino != 0)) {
			name_node = dir_path + d_name + string("/name");

			ifstream infile(name_node.c_str());

			if (!infile)
				continue;

			infile >> name;

			if (csensor_config::get_instance().is_supported(sensor_type, name)) {
				model_id = name;
				find = true;
				break;
			}
		}
	}

	closedir(dir);

	return find;
}

bool sensor_hal::get_event_num(const string &input_path, string &event_num)
{
	const string event_prefix = "event";
	DIR *dir = NULL;
	struct dirent *dir_entry = NULL;
	string node_name;
	bool find = false;

	dir = opendir(input_path.c_str());
	if (!dir) {
		ERR("Failed to open dir: %s", input_path.c_str());
		return false;
	}

	int prefix_size = event_prefix.size();

	while (!find && (dir_entry = readdir(dir))) {
		node_name = dir_entry->d_name;

		if (node_name.compare(0, prefix_size, event_prefix) == 0) {
			event_num = node_name.substr(prefix_size, node_name.size() - prefix_size);
			find = true;
			break;
		}
	}

	closedir(dir);

	return find;
}

bool sensor_hal::get_input_method(const string &key, int &method, string &device_num)
{
	input_method_info input_info[2] = {
		{INPUT_EVENT_METHOD, "/sys/class/input/", "input"},
		{IIO_METHOD, "/sys/bus/iio/devices/", "iio:device"}
	};

	const int input_info_len = sizeof(input_info)/sizeof(input_info[0]);
	size_t prefix_size;
	string name_node, name;
	string d_name;
	DIR *dir;
	struct dirent *dir_entry;
	bool find = false;

	for (int i = 0; i < input_info_len; ++i) {

		prefix_size = input_info[i].prefix.size();

		dir = opendir(input_info[i].dir_path.c_str());
		if (!dir) {
			ERR("Failed to open dir: %s", input_info[i].dir_path.c_str());
			return false;
		}

		find = false;

		while (!find && (dir_entry = readdir(dir))) {
			d_name = string(dir_entry->d_name);

			if (d_name.compare(0, prefix_size, input_info[i].prefix) == 0) {
				name_node = input_info[i].dir_path + d_name + string("/name");

				ifstream infile(name_node.c_str());
				if (!infile)
					continue;

				infile >> name;

				if (name == key) {
					device_num = d_name.substr(prefix_size, d_name.size() - prefix_size);
					find = true;
					method = input_info[i].method;
					break;
				}
			}
		}

		closedir(dir);

		if (find)
			break;
	}

	return find;
}
