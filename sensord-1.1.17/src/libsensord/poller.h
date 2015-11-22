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

#ifndef _POLLER_H_
#define _POLLER_H_

#include <glib.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <poller.h>
#include <unistd.h>
#include <sf_common.h>
#include <algorithm>
#include <queue>

using std::queue;

class poller {
public:
	poller(int fd);
	~poller();

	bool poll(int &event);
private:
	int m_epfd;
	queue<int> m_event_queue;

	bool create(int fd);
	bool fill_event_queue(void);
};

#endif /* _POLLER_H_ */
