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


#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include <mutex>
#include <condition_variable>

using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::condition_variable;

class worker_thread
{
public:
	enum worker_state_t {
		WORKER_STATE_INITIAL,
		WORKER_STATE_WORKING,
		WORKER_STATE_PAUSED,
		WORKER_STATE_STOPPED,
	};

	typedef bool(*trans_func_t)(void *data);
private:
	enum trans_func_index {
		STARTED = 0,
		STOPPED,
		PAUSED,
		RESUMED,
		WORKING,
		TRANS_FUNC_CNT,
	};

	typedef lock_guard<mutex>  lock;
	typedef unique_lock<mutex> ulock;

	worker_state_t m_state;
	void *m_context;
	mutex m_mutex;
	condition_variable m_cond_working;
	bool m_thread_created;

	trans_func_t m_trans_func[TRANS_FUNC_CNT];

	bool transition_function(trans_func_index index);
	void main(void);
public:
	worker_thread();
	virtual ~worker_thread();

	bool start(void);
	bool stop(void);
	bool pause(void);
	bool resume(void);

	worker_state_t get_state(void);

	void set_started(trans_func_t func);
	void set_stopped(trans_func_t func);
	void set_paused(trans_func_t func);
	void set_resumed(trans_func_t func);
	void set_working(trans_func_t func);

	void set_context(void *ctx);
};

#endif
