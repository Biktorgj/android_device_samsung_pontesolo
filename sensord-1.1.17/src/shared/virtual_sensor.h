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

#ifndef _VIRTUAL_SENSOR_H_
#define _VIRTUAL_SENSOR_H_

#include <sensor_base.h>

class virtual_sensor : public sensor_base
{
public:
	virtual_sensor();
	virtual ~virtual_sensor();

	virtual void synthesize(const sensor_event_t &event, vector<sensor_event_t> &outs) = 0;
	virtual int get_sensor_data(const unsigned int event_type, sensor_data_t &data) = 0;
	bool is_virtual(void);

protected:
	cmutex m_fusion_mutex;

	bool activate(void);
	bool deactivate(void);

	bool push(sensor_event_t const &event);
};

#endif
