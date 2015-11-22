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

#include <cmutex.h>
#include "common.h"

cmutex::cmutex()
{
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &mutex_attr);
	pthread_mutexattr_destroy(&mutex_attr);
}

cmutex::~cmutex()
{
	pthread_mutex_destroy(&m_mutex);
}

void cmutex::lock()
{
#ifdef _LOCK_DEBUG
	cbase_lock::lock(LOCK_TYPE_MUTEX, "mutex", __MODULE__, __func__, __LINE__);
#else
	cbase_lock::lock(LOCK_TYPE_MUTEX);
#endif
}

void cmutex::lock(const char* expr, const char *module, const char *func, int line)
{
	cbase_lock::lock(LOCK_TYPE_MUTEX, expr, module, func, line);
}

int cmutex::lock_impl(void)
{
	return pthread_mutex_lock(&m_mutex);
}

int cmutex::try_lock_impl(void)
{
	return pthread_mutex_trylock(&m_mutex);

}

int cmutex::unlock_impl()
{
	return pthread_mutex_unlock(&m_mutex);
}
