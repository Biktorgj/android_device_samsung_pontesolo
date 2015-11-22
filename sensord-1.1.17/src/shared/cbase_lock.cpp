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

#include <pthread.h>
#include <cbase_lock.h>
#include <stdio.h>
#include <common.h>
#include <errno.h>
#include <sys/time.h>


cbase_lock::cbase_lock()
{
	m_history_mutex = PTHREAD_MUTEX_INITIALIZER;
}

cbase_lock::~cbase_lock()
{
	pthread_mutex_destroy(&m_history_mutex);
}

void cbase_lock::lock(lock_type type, const char* expr, const char *module, const char *func, int line)
{
	int ret = 0;
	char m_curent_info[OWNER_INFO_LEN];
	struct timeval sv;
	unsigned long long lock_waiting_start_time = 0;
	unsigned long long lock_acquired_time = 0;
	unsigned long long waiting_time = 0;


	snprintf(m_curent_info, OWNER_INFO_LEN, "%s:%s(%d)", module, func, line);

	if (type == LOCK_TYPE_MUTEX)
		ret = try_lock_impl();
	else if (type == LOCK_TYPE_READ)
		ret = try_read_lock_impl();
	else if (type == LOCK_TYPE_WRITE)
		ret = try_write_lock_impl();

	if (ret == 0) {
		pthread_mutex_lock(&m_history_mutex);
		snprintf(m_owner_info, OWNER_INFO_LEN, "%s", m_curent_info);
		pthread_mutex_unlock(&m_history_mutex);
		return;
	}

	gettimeofday(&sv, NULL);
	lock_waiting_start_time = MICROSECONDS(sv);

	pthread_mutex_lock(&m_history_mutex);
	INFO("%s is waiting for getting %s(0x%x) owned in %s",
		m_curent_info, expr, this, m_owner_info);
	pthread_mutex_unlock(&m_history_mutex);


	if (type == LOCK_TYPE_MUTEX)
		lock_impl();
	else if (type == LOCK_TYPE_READ)
		read_lock_impl();
	else if (type == LOCK_TYPE_WRITE)
		write_lock_impl();

	gettimeofday(&sv, NULL);
	lock_acquired_time = MICROSECONDS(sv);

	waiting_time = lock_acquired_time - lock_waiting_start_time;

	pthread_mutex_lock(&m_history_mutex);
	INFO("%s acquires lock after waiting %lluus, %s(0x%x) was previously owned in %s",
		m_curent_info, waiting_time, expr, this, m_owner_info);
	snprintf(m_owner_info, OWNER_INFO_LEN, "%s", m_curent_info);
	pthread_mutex_unlock(&m_history_mutex);
}


void cbase_lock::lock(lock_type type)
{
	if (type == LOCK_TYPE_MUTEX)
		lock_impl();
	else if (type == LOCK_TYPE_READ)
		read_lock_impl();
	else if (type == LOCK_TYPE_WRITE)
		write_lock_impl();
}

void cbase_lock::unlock(void)
{
	unlock_impl();
}


int cbase_lock::lock_impl(void)
{
	return 0;
}

int cbase_lock::read_lock_impl(void)
{
	return 0;
}

int cbase_lock::write_lock_impl(void)
{
	return 0;
}

int cbase_lock::try_lock_impl(void)
{
	return 0;
}

int cbase_lock::try_read_lock_impl(void)
{
	return 0;
}

int cbase_lock::try_write_lock_impl(void)
{
	return 0;
}

int cbase_lock::unlock_impl(void)
{
	return 0;
}

Autolock::Autolock(cbase_lock &m, lock_type type, const char* expr, const char *module, const char *func, int line)
: m_lock(m)
{
	m_lock.lock(type, expr, module, func, line);
}

Autolock::Autolock(cbase_lock &m, lock_type type)
: m_lock(m)
{
	m_lock.lock(type);
}

Autolock::~Autolock()
{
	m_lock.unlock();
}
