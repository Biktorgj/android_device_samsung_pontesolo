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

#include <physical_sensor.h>
#include <csensor_event_queue.h>


physical_sensor::physical_sensor()
{

}

physical_sensor::~physical_sensor()
{

}

bool physical_sensor::push(sensor_event_t const &event)
{
	csensor_event_queue::get_instance().push(event);
	return true;
}

bool physical_sensor::push(sensorhub_event_t const &event)
{
	csensor_event_queue::get_instance().push(event);
	return true;
}

void physical_sensor::set_poller(working_func_t func, void *arg)
{
	m_sensor_data_poller.set_context(arg);
	m_sensor_data_poller.set_working(func);
}


bool physical_sensor::start_poll(void)
{
	return m_sensor_data_poller.start();

}

bool physical_sensor::stop_poll(void)
{
	return m_sensor_data_poller.pause();
}
