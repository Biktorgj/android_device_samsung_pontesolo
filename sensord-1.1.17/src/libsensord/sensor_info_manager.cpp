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

#include <sensor_info_manager.h>
#include <utility>

using std::pair;
using std::make_pair;

sensor_info_manager::sensor_info_manager()
{
}

sensor_info_manager::~sensor_info_manager()
{
	auto it_info = m_sensor_infos.begin();

	while (it_info != m_sensor_infos.end()) {

		delete it_info->second;
		++it_info;
	}
}

sensor_info_manager& sensor_info_manager::get_instance(void)
{
	static sensor_info_manager inst;
	return inst;
}

const sensor_info* sensor_info_manager::get_info(sensor_type_t type)
{
	auto it_info = m_sensor_infos.find(type);

	if (it_info == m_sensor_infos.end())
		return NULL;

	return it_info->second;
}

void sensor_info_manager::add_info(sensor_info* info)
{
	m_sensor_infos.insert(make_pair(info->get_type(), info));
	m_id_to_info_map.insert(make_pair(info->get_id(), info));
	m_info_set.insert(info);
}

vector<sensor_info *> sensor_info_manager::get_infos(sensor_type_t type)
{
	vector<sensor_info *> sensor_infos;

	pair<sensor_infos::iterator, sensor_infos::iterator> ret;

	if (type == ALL_SENSOR)
		ret = std::make_pair(m_sensor_infos.begin(), m_sensor_infos.end());
	else
		ret = m_sensor_infos.equal_range(type);

	for (auto it_info = ret.first; it_info != ret.second; ++it_info) {
		sensor_infos.push_back(it_info->second);
	}

	return sensor_infos;
}

const sensor_info* sensor_info_manager::get_info(sensor_id_t id)
{
	auto it_info = m_id_to_info_map.find(id);

	if (it_info == m_id_to_info_map.end())
		return NULL;

	return it_info->second;
}


bool sensor_info_manager::is_valid(sensor_info* info)
{
	auto it_info = m_info_set.find(info);

	if (it_info == m_info_set.end())
		return false;

	return true;
}
