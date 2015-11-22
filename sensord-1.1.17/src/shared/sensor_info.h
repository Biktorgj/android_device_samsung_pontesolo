/*
 * libsensord
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
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

#ifndef _SENSOR_INFO_H_
#define _SENSOR_INFO_H_

#include <sf_common.h>
#include <sensor_common.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

typedef vector<char> raw_data_t;
typedef raw_data_t::iterator raw_data_iterator;

class sensor_info
{
public:
	sensor_type_t get_type(void);
	sensor_id_t get_id(void);
	sensor_privilege_t get_privilege(void);
	const char* get_name(void);
	const char* get_vendor(void);
	float get_min_range(void);
	float get_max_range(void);
	float get_resolution(void);
	int get_min_interval(void);
	int get_fifo_count(void);
	int get_max_batch_count(void);
	void get_supported_events(vector<unsigned int> &events);
	bool is_supported_event(unsigned int event);

	void set_type(sensor_type_t type);
	void set_id(sensor_id_t id);
	void set_privilege(sensor_privilege_t privilege);
	void set_name(const char *name);
	void set_vendor(const char *vendor);
	void set_min_range(float min_range);
	void set_max_range(float max_range);
	void set_resolution(float resolution);
	void set_min_interval(int min_interval);
	void set_fifo_count(int fifo_count);
	void set_max_batch_count(int max_batch_count);
	void register_supported_event(unsigned int event);
	void set_supported_events(vector<unsigned int> &events);

	void clear(void);

	void get_raw_data(raw_data_t &data);
	void set_raw_data(const char *data, int data_len);
	void show(void);
private:
	sensor_type_t m_type;
	sensor_id_t m_id;
	sensor_privilege_t m_privilege;
	string m_name;
	string m_vendor;
	float m_min_range;
	float m_max_range;
	float m_resolution;
	int m_min_interval;
	int m_fifo_count;
	int m_max_batch_count;
	vector<unsigned int> m_supported_events;

	void put(raw_data_t &data, int value);
	void put(raw_data_t &data, float value);
	void put(raw_data_t &data, string &value);
	void put(raw_data_t &data, vector<unsigned int> &value);

	raw_data_iterator get(raw_data_iterator it, int &value);
	raw_data_iterator get(raw_data_iterator it, float &value);
	raw_data_iterator get(raw_data_iterator it, string &value);
	raw_data_iterator get(raw_data_iterator it, vector<unsigned int> &value);
};

#endif /* _SENSOR_INFO_H_ */
