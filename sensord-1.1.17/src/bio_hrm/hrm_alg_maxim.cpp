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

#include <hrm_alg_maxim.h>
#include <common.h>
#include <sensor_common.h>
#include <dlfcn.h>

#define HR_LIB "/usr/lib/sensord/libhr.so"

hrm_alg_maxim::hrm_alg_maxim()
: m_handle(NULL)
{

}

hrm_alg_maxim::~hrm_alg_maxim()
{
	close();
}

bool hrm_alg_maxim::open(void)
{
	m_handle = dlopen(HR_LIB, RTLD_NOW);

	if (!m_handle) {
		ERR("dlopen(%s) error, cause: %s", HR_LIB, dlerror());
		return false;
	}

	hr_reset_state = (hr_reset_state_t) dlsym(m_handle, "reset_state");
	hr_close_state = (hr_close_state_t) dlsym(m_handle, "close_state");
	hr_alg = (hr_alg_t) dlsym(m_handle, "hrspo2_alg");


	if (!hr_reset_state || !hr_close_state || !hr_alg) {
		ERR("Fail to load HR lib symbols");
		dlclose(m_handle);
		m_handle = NULL;
		return false;
	}

	return true;
}

bool hrm_alg_maxim::close(void)
{
	if (m_handle) {
		dlclose(m_handle);
		m_handle = NULL;
	}

	return true;
}

bool hrm_alg_maxim::start(void)
{
	hr_reset_state(0);
	return true;
}

bool hrm_alg_maxim::stop(void)
{
	hr_close_state();
	return true;
}

bool hrm_alg_maxim::get_data(float acc[3], const sensor_event_t& bio_event, hrm_data &data)
{
	float hr = 0.0f;
	float spo2 = 0.0f;
	float snr = 0.0f;
	int ready = 0;
	int interval_cnt =0;

	DBG("bio[0]: %d, bio[1]: %d, bio[2]: %d, acc[0]: %f, acc[1]: %f, acc[2]: %f",
		(int)bio_event.data.values[0], (int)bio_event.data.values[1], (int)bio_event.data.values[2],
		acc[0], acc[1], acc[2]);

	hr_alg((int)bio_event.data.values[0], (int)bio_event.data.values[1], (int)bio_event.data.values[2],
		&hr, &spo2, &snr, &interval_cnt, &ready, acc[0], acc[1], acc[2]);

	if (ready) {
		data.hr = hr;
		data.spo2 = spo2;
		data.peek_to_peek = interval_cnt * 10;
		data.snr = snr;
		DBG("HR: %d, SPO2: %d, PtoP: %d, SNR: %f", data.hr, data.spo2, data.peek_to_peek, data.snr);
		return true;
	}

	return false;
}
