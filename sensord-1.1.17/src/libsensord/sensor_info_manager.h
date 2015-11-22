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

#ifndef _SENSOR_INFO_MANAGER_H_
#define _SENSOR_INFO_MANAGER_H_

#include <sensor_common.h>
#include <sensor_info.h>
#include <map>
#include <unordered_map>
#include <unordered_set>

using std::multimap;
using std::unordered_map;
using std::unordered_set;

class sensor_info_manager {
public:
	static sensor_info_manager& get_instance(void);
	const sensor_info* get_info(sensor_type_t type);
	vector<sensor_info *> get_infos(sensor_type_t type);
	const sensor_info* get_info(sensor_id_t id);
	bool is_valid(sensor_info* info);
	void add_info(sensor_info* info);

private:
	typedef multimap<sensor_type_t, sensor_info*> sensor_infos;
	typedef unordered_map<sensor_id_t, sensor_info*> id_to_info_map;
	typedef unordered_set<sensor_info*> info_set;

	sensor_info_manager();
	~sensor_info_manager();

	sensor_info_manager(const sensor_info_manager&) {};
	sensor_info_manager& operator=(const sensor_info_manager&);

	sensor_infos m_sensor_infos;
	id_to_info_map m_id_to_info_map;
	info_set m_info_set;
};
#endif /* _SENSOR_INFO_MANAGER_H_ */
