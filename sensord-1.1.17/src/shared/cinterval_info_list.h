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

#if !defined(_CINTERVAL_INFO_LIST_CLASS_H_)
#define _CINTERVAL_INFO_LIST_CLASS_H_

#include <list>
using std::list;

class cinterval_info
{
public:
	cinterval_info(int client_id, bool is_processor, unsigned int interval);
	int client_id;
	bool is_processor;
	unsigned int interval;
};

typedef list<cinterval_info>::iterator cinterval_info_iterator;

class cinterval_info_list
{
private:
	static bool comp_interval_info(cinterval_info a, cinterval_info b);
	cinterval_info_iterator find_if(int client_id, bool is_processor);

	list<cinterval_info> m_list;

public:
	bool add_interval(int client_id, unsigned int interval, bool is_processor);
	bool delete_interval(int client_id, bool is_processor);
	unsigned int get_interval(int client_id, bool is_processor);
	unsigned int get_min(void);
};
#endif
