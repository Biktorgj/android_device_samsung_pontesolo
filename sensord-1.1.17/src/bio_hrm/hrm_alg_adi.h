/*
 * hrm_alg_adi
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
#include <hrm_alg.h>
#include <cmutex.h>

#ifndef _HRM_ALG_ADI_H_
#define _HRM_ALG_ADI_H_

class hrm_alg_adi : public hrm_alg
{
private:
	typedef struct HR_Result {
		float hr;
		float snr;
		float spo2;
		float peek_to_peek;
	} HR_Result_t;

	typedef int (*hr_calc_t)(HR_Result_t *result, float red_data[4],
		float ir_data[4], long long timestamp, float acc[3], int adpd_mode);

	hr_calc_t hr_calc;
	void *m_handle;
	cmutex m_mutex;
	bool m_running;
public:
	hrm_alg_adi();
	~hrm_alg_adi();

	virtual bool open(void);
	virtual bool close(void);
	virtual bool start(void);
	virtual bool stop(void);

	virtual bool get_data(float acc[3], const sensor_event_t& bio_event, hrm_data &data);
};
#endif
