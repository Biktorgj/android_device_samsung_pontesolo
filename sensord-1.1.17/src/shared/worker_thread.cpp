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

#include <common.h>
#include <worker_thread.h>
#include <thread>

using std::thread;

worker_thread::worker_thread()
: m_state(WORKER_STATE_INITIAL)
, m_context(NULL)
, m_thread_created(false)
{
	for (int i = 0; i < TRANS_FUNC_CNT; ++i)
		m_trans_func[i] = NULL;
}

worker_thread::~worker_thread()
{
	stop();
}

bool worker_thread::transition_function(trans_func_index index)
{
	if (m_trans_func[index] != NULL) {
		if(!m_trans_func[index](m_context)) {
			_T("Transition[%d] function returning false", index);
			return false;
		}
	}

	return true;
}

worker_thread::worker_state_t worker_thread::get_state(void)
{
	lock l(m_mutex);
	return m_state;
}


bool worker_thread::start(void)
{
	lock l(m_mutex);

	if (m_state == WORKER_STATE_WORKING) {
		_T("Worker thread is already working");
		return true;
	}

	if ((m_state == WORKER_STATE_INITIAL) || (m_state == WORKER_STATE_STOPPED)) {
		m_state = WORKER_STATE_WORKING;

		if (!m_thread_created) {
			thread th(&worker_thread::main, this);
			th.detach();
		}
		return true;
	} else if (m_state == WORKER_STATE_PAUSED) {
		m_state = WORKER_STATE_WORKING;
		m_cond_working.notify_one();
		return true;
	}

	_T("Failed to start, because current state(%d) is not for START", m_state);

	return false;
}

bool worker_thread::stop(void)
{
	lock l(m_mutex);

	if (m_state == WORKER_STATE_STOPPED) {
		_T("Worker thread is already stopped");
		return true;
	}

	if ((m_state == WORKER_STATE_WORKING) || (m_state == WORKER_STATE_PAUSED)) {

		if (m_state == WORKER_STATE_PAUSED)
			m_cond_working.notify_one();

		m_state = WORKER_STATE_STOPPED;
		return true;
	}

	_T("Failed to stop, because current state(%d) is not for STOP", m_state);
	return false;
}

bool worker_thread::pause(void)
{
	lock l(m_mutex);

	if (m_state == WORKER_STATE_PAUSED) {
		_T("Worker thread is already paused");
		return true;
	}

	if (m_state == WORKER_STATE_WORKING) {
		m_state = WORKER_STATE_PAUSED;
		return true;
	}

	_T("Failed to pause, because current state(%d) is not for PAUSE", m_state);

	return false;

}

bool worker_thread::resume(void)
{
	lock l(m_mutex);

	if (m_state == WORKER_STATE_WORKING) {
		_T("Worker thread is already working");
		return true;
	}

	if (m_state == WORKER_STATE_PAUSED) {
		m_state = WORKER_STATE_WORKING;
		m_cond_working.notify_one();
		return true;
	}

	_T("Failed to resume, because current state(%d) is not for RESUME", m_state);
	return false;
}


/*
 * After state changed to STOPPED, it should not access member fields,
    because some transition funciton of STOPPED delete this pointer
 */

void worker_thread::main(void)
{
	_T("Worker thread(0x%x) is created", std::this_thread::get_id());

	transition_function(STARTED);

	while (true) {
		worker_state_t state;
		state = get_state();

		if (state == WORKER_STATE_WORKING) {
			if (!transition_function(WORKING)) {
				m_state = WORKER_STATE_STOPPED;
				_T("Worker thread(0x%x) exits from working state", std::this_thread::get_id());
				m_thread_created = false;
				transition_function(STOPPED);
				break;
			}
			continue;
		}

		ulock u(m_mutex);

		if (m_state == WORKER_STATE_PAUSED) {
			transition_function(PAUSED);

			_T("Worker thread(0x%x) is paused", std::this_thread::get_id());
			m_cond_working.wait(u);

			if (m_state == WORKER_STATE_WORKING) {
				transition_function(RESUMED);
				_T("Worker thread(0x%x) is resumed", std::this_thread::get_id());
			} else if (m_state == WORKER_STATE_STOPPED) {
				m_thread_created = false;
				transition_function(STOPPED);
				break;
			}
		} else if (m_state == WORKER_STATE_STOPPED) {
			m_thread_created = false;
			transition_function(STOPPED);
			break;
		}
	}
	_T("Worker thread(0x%x)'s main is terminated", std::this_thread::get_id());
}

void worker_thread::set_started(trans_func_t func)
{
	m_trans_func[STARTED] = func;
}

void worker_thread::set_stopped(trans_func_t func)
{
	m_trans_func[STOPPED] = func;
}

void worker_thread::set_paused(trans_func_t func)
{
	m_trans_func[PAUSED] = func;
}

void worker_thread::set_resumed(trans_func_t func)
{
	m_trans_func[RESUMED] = func;
}

void worker_thread::set_working(trans_func_t func)
{
	m_trans_func[WORKING] = func;
}

void worker_thread::set_context(void *ctx)
{
	m_context = ctx;
}


