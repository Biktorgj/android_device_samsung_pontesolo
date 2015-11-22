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

#ifndef _PHYSICAL_SENSOR_H_
#define _PHYSICAL_SENSOR_H_

#include <sensor_base.h>
#include <sf_common.h>
#include <worker_thread.h>

class physical_sensor : public sensor_base
{
public:
	typedef worker_thread::trans_func_t working_func_t;

private:
	worker_thread m_sensor_data_poller;

protected:
	physical_sensor();
	virtual ~physical_sensor();

	bool push(sensor_event_t const &event);
	bool push(sensorhub_event_t const &event);

	void set_poller(working_func_t func, void *arg);
	bool start_poll(void);
	bool stop_poll(void);
};

#endif
