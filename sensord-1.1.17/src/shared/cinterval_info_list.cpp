/*
 * libsensord-share
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

#include <cinterval_info_list.h>
#include <algorithm>


cinterval_info::cinterval_info(int client_id, bool is_processor, unsigned int interval)
{
	this->client_id = client_id;
	this->is_processor = is_processor;
	this->interval = interval;
}

bool cinterval_info_list::comp_interval_info(cinterval_info a, cinterval_info b)
{
	return a.interval < b.interval;
}

cinterval_info_iterator cinterval_info_list::find_if(int client_id, bool is_processor)
{
	auto iter = m_list.begin();

	while (iter != m_list.end()) {
		if ((iter->client_id == client_id) && (iter->is_processor == is_processor))
			break;

		++iter;
	}

	return iter;
}


bool cinterval_info_list::add_interval(int client_id, unsigned int interval, bool is_processor)
{
	auto iter = find_if(client_id, is_processor);

	if (iter != m_list.end())
		*iter = cinterval_info(client_id, is_processor, interval);
	else
		m_list.push_back(cinterval_info(client_id, is_processor, interval));

	return true;
}

bool cinterval_info_list::delete_interval(int client_id, bool is_processor)
{
	auto iter = find_if(client_id, is_processor);

	if (iter == m_list.end())
		return false;

	m_list.erase(iter);

	return true;
}

unsigned int cinterval_info_list::get_interval(int client_id, bool is_processor)
{
	auto iter = find_if(client_id, is_processor);

	if (iter == m_list.end())
		return 0;

	return iter->interval;
}

unsigned int cinterval_info_list::get_min(void)
{
	if (m_list.empty())
		return 0;

	auto iter = min_element(m_list.begin(), m_list.end(), comp_interval_info);

	return iter->interval;
}


