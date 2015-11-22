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

#include <hrm_alg_adi.h>
#include <common.h>
#include <sensor_common.h>
#include <dlfcn.h>

#define HR_LIB "/usr/lib/sensord/libHRM.so"

hrm_alg_adi::hrm_alg_adi()
: m_handle(NULL)
, m_running(false)
{

}

hrm_alg_adi::~hrm_alg_adi()
{
	close();
}

bool hrm_alg_adi::open(void)
{
	m_handle = dlopen(HR_LIB, RTLD_NOW);

	if (!m_handle) {
		ERR("dlopen(%s) error, cause: %s", HR_LIB, dlerror());
		return false;
	}

	hr_calc = (hr_calc_t) dlsym(m_handle, "Cal_HR");

	if (!hr_calc) {
		ERR("Fail to load HR lib symbols");
		dlclose(m_handle);
		m_handle = NULL;
		return false;
	}

	return true;
}

bool hrm_alg_adi::close(void)
{
	if (m_handle) {
		dlclose(m_handle);
		m_handle = NULL;
	}

	return true;
}

bool hrm_alg_adi::start(void)
{
	AUTOLOCK(m_mutex);
	m_running = true;

	return true;
}

bool hrm_alg_adi::stop(void)
{
	const int MODE_STOP = -2;
	HR_Result_t result;
	float irch[4] = {0,};
	float rch[4] = {0,};
	float acc[3] = {0,};

	{	/* the scope for the lock */
		AUTOLOCK(m_mutex);
		m_running = false;
		hr_calc(&result, rch, irch, 0, acc, MODE_STOP);
	}
	return true;
}

bool hrm_alg_adi::get_data(float acc[3], const sensor_event_t& bio_event, hrm_data &data)
{
	HR_Result_t result;
	const float* bio_values = bio_event.data.values;

	float irch[4] = {bio_values[2], bio_values[3], bio_values[4], bio_values[5]};
	float rch[4] = {bio_values[6], bio_values[7], bio_values[8], bio_values[9]};

	DBG("timestamp = %llu, mode = %d, acc = {%f, %f, %f}, irch = {%f, %f, %f, %f}, rch = {%f, %f, %f, %f}",
		bio_event.data.timestamp, (int) bio_values[10],
		acc[0], acc[1], acc[2], irch[0], irch[1], irch[2], irch[3], rch[0], rch[1], rch[2], rch[3]);

	int ret;

	{	/* the scope for the lock */
		AUTOLOCK(m_mutex);
		if (!m_running) {
			WARN("hr_calc() is called in stop state!");
			return false;
		}
		ret = hr_calc(&result, rch, irch, (long long) bio_event.data.timestamp, acc, (int) bio_values[10]);
	}

	if (ret) {

		data.hr = result.hr;
		data.peek_to_peek = result.peek_to_peek;
		data.snr = result.snr;
		data.spo2 = -1;
		DBG("HR: %d, SPO2: %d, PtoP: %d, SNR: %f", data.hr, data.spo2, data.peek_to_peek, data.snr);
		return true;
	}

	return false;
}
