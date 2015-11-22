/*
 * hrm_alg_maxim
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

#ifndef _HRM_ALG_MAXIM_H_
#define _HRM_ALG_MAXIM_H_

class hrm_alg_maxim : public hrm_alg
{
private:
	typedef void (*hr_reset_state_t)(int log_enable);
    typedef void (*hr_close_state_t)(void);
    typedef void (*hr_alg_t)(int ir, int red, int temperature, float *hr, float *tmp, float *sq, int *interval_cnt, int *ready, float x, float y, float z);

	hr_reset_state_t hr_reset_state;
	hr_close_state_t hr_close_state;
	hr_alg_t hr_alg;

	void *m_handle;

public:
	hrm_alg_maxim();
	~hrm_alg_maxim();

	virtual bool open(void);
	virtual bool close(void);
	virtual bool start(void);
	virtual bool stop(void);

	virtual bool get_data(float acc[3], const sensor_event_t& bio_event, hrm_data &data);
};
#endif
