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

#ifndef _SENSOR_HAL_H_
#define _SENSOR_HAL_H_
#include <sys/time.h>
#include <sf_common.h>
#include <cmutex.h>
#include <common.h>
#include <sensor_internal.h>
#include <string>

using std::string;

/*
* As of Linux 3.4, there is a new EVIOCSCLOCKID ioctl to set the desired clock
* Current kernel-headers package doesn't have it so we should define it here.
*/

#ifndef EVIOCSCLOCKID
#define EVIOCSCLOCKID		_IOW('E', 0xa0, int)			/* Set clockid to be used for timestamps */
#endif

typedef struct {
	int method;
	string data_node_path;
	string enable_node_path;
	string interval_node_path;
	string buffer_enable_node_path;
	string buffer_length_node_path;
	string trigger_node_path;
} node_info;

typedef struct {
	bool sensorhub_controlled;
	string sensor_type;
	string key;
	string iio_enable_node_name;
	string sensorhub_interval_node_name;
} node_info_query;

enum input_method {
	IIO_METHOD = 0,
	INPUT_EVENT_METHOD = 1,
};

typedef struct {
	int method;
	std::string dir_path;
	std::string prefix;
} input_method_info;

#define DEFAULT_WAIT_TIME 0

class sensor_hal
{
public:
	sensor_hal();
	virtual ~sensor_hal();

	virtual bool init(void *data = NULL);
	virtual string get_model_id(void) = 0;
	virtual sensor_type_t get_type(void) = 0;
	virtual bool enable(void) = 0;
	virtual bool disable(void) = 0;
	virtual bool set_interval(unsigned long val);
	virtual bool is_data_ready(bool wait) = 0;
	virtual bool get_properties(sensor_properties_t &properties) = 0;
	virtual int get_sensor_data(sensor_data_t &data);
	virtual int get_sensor_data(sensorhub_data_t &data);
	virtual long set_command(unsigned int cmd, long val);
	virtual int send_sensorhub_data(const char *data, int data_len);

protected:
	cmutex m_mutex;
	static cmutex m_shared_mutex;

	virtual bool set_enable_node(const string &node_path, bool sensorhub_controlled, bool enable, int enable_bit = 0);

	static unsigned long long get_timestamp(void);
	static unsigned long long get_timestamp(timeval *t);
	static bool find_model_id(const string &sensor_type, string &model_id);
	static bool is_sensorhub_controlled(const string &key);
	static bool get_node_info(const node_info_query &query, node_info &info);
	static void show_node_info(node_info &info);
	static bool set_node_value(const string &node_path, int value);
	static bool set_node_value(const string &node_path, unsigned long long value);
	static bool get_node_value(const string &node_path, int &value);
private:
	static bool get_event_num(const string &node_path, string &event_num);
	static bool get_input_method(const string &key, int &method, string &device_num);

	static bool get_iio_node_info(const string& enable_node_name, const string& device_num, node_info &info);
	static bool get_sensorhub_iio_node_info(const string &interval_node_name, const string& device_num, node_info &info);
	static bool get_input_event_node_info(const string& device_num, node_info &info);
	static bool get_sensorhub_input_event_node_info(const string &interval_node_name, const string& device_num, node_info &info);
};
#endif /*_SENSOR_HAL_CLASS_H_*/
