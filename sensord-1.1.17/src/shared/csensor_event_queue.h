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

#if !defined(_CSENSOR_EVENT_QUEUE_CLASS_H_)
#define _CSENSOR_EVENT_QUEUE_CLASS_H_
#include <sf_common.h>
#include <queue>
#include <mutex>
#include <condition_variable>

using std::queue;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::condition_variable;

class csensor_event_queue
{
private:
	static const unsigned int QUEUE_FULL_SIZE = 1000;

	queue<void* > m_queue;
	mutex m_mutex;
	condition_variable m_cond_var;

	typedef lock_guard<mutex> lock;
	typedef unique_lock<mutex> ulock;

	csensor_event_queue();
	csensor_event_queue(csensor_event_queue const&) {};
	csensor_event_queue& operator=(csensor_event_queue const&);
	void push_internal(void *event);

public:
	static csensor_event_queue& get_instance();
	void push(sensor_event_t const &event);
	void push(sensorhub_event_t const &event);
	void* pop(void);
};

#endif
