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

#ifndef CREG_EVENT_INFO_H_
#define CREG_EVENT_INFO_H_

#include <sensor_internal.h>
#include <sf_common.h>

typedef enum {
	SENSOR_EVENT_CB,
	SENSORHUB_EVENT_CB,
	SENSOR_LEGACY_CB,
} event_cb_type_t;

class creg_event_info {
public:
	unsigned long long m_id;
	int m_handle;
	unsigned int type;
	unsigned int m_interval;
	int m_cb_type;
	void *m_cb;
	void *m_user_data;
	unsigned long long m_previous_event_time;
	bool	m_fired;

	creg_event_info():m_id(0), m_handle(-1),
			type(0), m_interval(POLL_1HZ_MS),
			m_cb_type(SENSOR_EVENT_CB), m_cb(NULL), m_user_data(NULL),
			m_previous_event_time(0), m_fired(false){}

	~creg_event_info(){}
};


#endif /* CREG_EVENT_INFO_H_ */
