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

#if !defined(_CBASE_LOCK_CLASS_H_)
#define _CBASE_LOCK_CLASS_H_

#include <pthread.h>

enum lock_type {
	LOCK_TYPE_MUTEX,
	LOCK_TYPE_READ,
	LOCK_TYPE_WRITE,
};

#ifdef _LOCK_DEBUG
#define AUTOLOCK(x) Autolock x##_autolock((x),LOCK_TYPE_MUTEX, #x, __MODULE__, __func__, __LINE__)
#define AUTOLOCK_R(x) Autolock x##_autolock_r((x),LOCK_TYPE_READ, #x,  __MODULE__, __func__, __LINE__)
#define AUTOLOCK_W(x) Autolock x##_autolock_w((x),LOCK_TYPE_WRITE, #x, __MODULE__, __func__, __LINE__)
#define LOCK(x)		(x).lock(#x, __MODULE__, __func__, __LINE__)
#define LOCK_R(x)	(x).lock(LOCK_TYPE_READ, #x, __MODULE__, __func__, __LINE__)
#define LOCK_W(x)	(x).lock(LOCK_TYPE_WRITE, #x, __MODULE__, __func__, __LINE__)
#define UNLOCK(x)	(x).unlock()
#else
#define AUTOLOCK(x) Autolock x##_autolock((x),LOCK_TYPE_MUTEX)
#define AUTOLOCK_R(x) Autolock x##_autolock_r((x),LOCK_TYPE_READ)
#define AUTOLOCK_W(x) Autolock x##_autolock_w((x),LOCK_TYPE_WRITE)
#define LOCK(x)		(x).lock()
#define LOCK_R(x)	(x).lock(LOCK_TYPE_READ)
#define LOCK_W(x)	(x).lock(LOCK_TYPE_WRITE)
#define UNLOCK(x)	(x).unlock()
#endif


class cbase_lock
{
public:
	cbase_lock();
	virtual ~cbase_lock();

	void lock(lock_type type, const char* expr, const char *module, const char *func, int line);
	void lock(lock_type type);
	void unlock(void);

protected:
	virtual int lock_impl(void);
	virtual int read_lock_impl(void);
	virtual int write_lock_impl(void);

	virtual int try_lock_impl(void);
	virtual int try_read_lock_impl(void);
	virtual int try_write_lock_impl(void);

	virtual int unlock_impl(void);
private:
	pthread_mutex_t m_history_mutex;
	static const int OWNER_INFO_LEN = 256;
	char m_owner_info[OWNER_INFO_LEN];
};

class Autolock
{
private:
	cbase_lock& m_lock;
public:
	Autolock(cbase_lock &m, lock_type type, const char* expr, const char *module, const char *func, int line);
	Autolock(cbase_lock &m, lock_type type);
	~Autolock();
};

#endif
// End of a file
