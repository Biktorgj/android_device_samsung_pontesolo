/* hrm_alg
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
#ifndef _HRM_ALG_H_
#define _HRM_ALG_H_


#include <hrm_alg.h>
#include <sys/time.h>
#include <sensor_common.h>
#include <sf_common.h>

typedef struct hrm_data {
	int hr;
	int spo2;
	int peek_to_peek;
	float snr;
} hrm_data;

class hrm_alg
{
public:
	hrm_alg();
	virtual ~hrm_alg();

	virtual bool open(void);
	virtual bool close(void);
	virtual bool start(void);
	virtual bool stop(void);
	virtual bool get_data(float acc[3], const sensor_event_t& bio_event, hrm_data &data) = 0;
};
#endif
