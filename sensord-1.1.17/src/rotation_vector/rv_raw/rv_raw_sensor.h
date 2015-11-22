/*
 * sensord
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

#ifndef _RV_RAW_SENSOR_H_
#define _RV_RAW_SENSOR_H_

#include <sensor_common.h>

#include <physical_sensor.h>
#include <sensor_hal.h>
#include <sensor_fusion.h>
#include <time.h>

class rv_raw_sensor : public physical_sensor {
public:
	rv_raw_sensor();
	virtual ~rv_raw_sensor();

	virtual bool init();
	virtual sensor_type_t get_type(void);

	static bool working(void *inst);

	virtual bool add_interval(int client_id, unsigned int interval, bool is_processor = false);
	virtual bool delete_interval(int client_id, bool is_processor = false);
	virtual bool set_interval(unsigned long interval);
	virtual bool get_properties(sensor_properties_t &properties);
	int get_sensor_data(unsigned int type, sensor_data_t &data);
private:
	sensor_hal *m_sensor_hal;
	cmutex m_value_mutex;

	float m_x;
	float m_y;
	float m_z;
	float m_w;
	time_t m_time;
	unsigned long m_interval;
	struct timeval m_prev_time;

	virtual bool on_start(void);
	virtual bool on_stop(void);

	bool process_event(void);
};

#endif /*_RV_RAW_SENSOR_H_*/
