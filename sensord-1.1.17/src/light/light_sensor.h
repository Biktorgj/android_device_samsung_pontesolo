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

#ifndef _LIGHT_SENSOR_H_
#define _LIGHT_SENSOR_H_

#include <sensor_common.h>

#include <physical_sensor.h>
#include <sensor_hal.h>

class light_sensor : public physical_sensor {
public:
	light_sensor();
	virtual ~light_sensor();

	bool init();
	sensor_type_t get_type(void);

	static bool working(void *inst);

	bool set_interval(unsigned long interval);
	virtual bool get_properties(sensor_properties_t &properties);
	int get_sensor_data(unsigned int type, sensor_data_t &data);

private:
	static const int m_light_level[];

	sensor_hal *m_sensor_hal;
	int m_level;

	virtual bool on_start(void);
	virtual bool on_stop(void);

	bool process_event(void);
	int adc_to_light_level(int adc);

	void raw_to_level(sensor_data_t &data);
	void raw_to_state(sensor_data_t &data);
};

#endif
