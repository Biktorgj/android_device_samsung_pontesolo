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

#if !defined(_CMUTEX_CLASS_H_)
#define _CMUTEX_CLASS_H_

#include "cbase_lock.h"

class cmutex : public cbase_lock
{
public:
	cmutex();
	virtual ~cmutex();

	void lock(void);
	void lock(const char* expr, const char *module, const char *func, int line);

protected:
	int lock_impl(void);
	int try_lock_impl(void);
	int unlock_impl();
private:
	pthread_mutex_t m_mutex;
};

#endif
// End of a file
