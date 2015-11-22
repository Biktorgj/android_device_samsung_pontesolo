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

#include "crw_lock.h"
#include "common.h"


crw_lock::crw_lock()
{
	m_rw_lock = PTHREAD_RWLOCK_INITIALIZER;
}

crw_lock::~crw_lock()
{
	pthread_rwlock_destroy(&m_rw_lock);
}

void crw_lock::read_lock()
{
#ifdef _LOCK_DEBUG
	cbase_lock::lock(LOCK_TYPE_READ, "read lock", __MODULE__, __func__, __LINE__);
#else
	cbase_lock::lock(LOCK_TYPE_READ);
#endif
}

void crw_lock::write_lock()
{
#ifdef _LOCK_DEBUG
	cbase_lock::lock(LOCK_TYPE_WRITE, "write lock", __MODULE__, __func__, __LINE__);
#else
	cbase_lock::lock(LOCK_TYPE_WRITE);
#endif
}

int crw_lock::read_lock_impl(void)
{
	return pthread_rwlock_rdlock(&m_rw_lock);
}
int crw_lock::write_lock_impl(void)
{
	return pthread_rwlock_wrlock(&m_rw_lock);
}

int crw_lock::try_read_lock_impl(void)
{
	return pthread_rwlock_tryrdlock(&m_rw_lock);
}

int crw_lock::try_write_lock_impl(void)
{
	return pthread_rwlock_trywrlock(&m_rw_lock);
}

int crw_lock::unlock_impl()
{
	return pthread_rwlock_unlock(&m_rw_lock);
}
