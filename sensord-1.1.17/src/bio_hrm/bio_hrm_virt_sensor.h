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

#ifndef _BIO_HRM_VIRT_SENSOR_H_
#define _BIO_HRM_VIRT_SENSOR_H_

#include <sensor_internal.h>
#include <vconf.h>
#include <string>

using std::string;

class hrm_alg;

class bio_hrm_virt_sensor : public virtual_sensor {
public:
	bio_hrm_virt_sensor();
	virtual ~bio_hrm_virt_sensor();

	bool init();
	sensor_type_t get_type(void);

	static bool working(void *inst);

	void synthesize(const sensor_event_t& event, vector<sensor_event_t> &outs);

	int get_sensor_data(unsigned int data_id, sensor_data_t &data);
	virtual bool get_properties(sensor_properties_t &properties);
private:
	sensor_base *m_accel_sensor;
	sensor_base *m_bio_sensor;
	hrm_alg	*m_alg;

	float m_acc[3];
	int m_hr;
	int m_spo2;
	int m_peek_to_peek;
	float m_snr;

	unsigned long long m_fired_time;

	cmutex m_value_mutex;

	virtual bool on_start(void);
	virtual bool on_stop(void);
	void reset(void);
	hrm_alg* get_alg(const string &model_id);
};

#endif
