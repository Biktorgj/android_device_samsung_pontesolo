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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <dirent.h>

#include <common.h>
#include <sf_common.h>

#include <virtual_sensor.h>
#include <bio_hrm_virt_sensor.h>
#include <sensor_plugin_loader.h>
#include <hrm_alg_maxim.h>
#include <hrm_alg_adi.h>
#include <sensor_hal.h>
#include <fstream>

using std::ifstream;

#define SENSOR_NAME "BIO_HRM_SENSOR"

bio_hrm_virt_sensor::bio_hrm_virt_sensor()
: m_accel_sensor(NULL)
, m_bio_sensor(NULL)
, m_alg(NULL)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(BIO_HRM_EVENT_CHANGE_STATE);

	reset();
}

bio_hrm_virt_sensor::~bio_hrm_virt_sensor()
{
	delete m_alg;

	INFO("bio_hrm_virt_sensor is destroyed!\n");
}

bool bio_hrm_virt_sensor::init()
{
	sensor_hal *bio_hrm_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(BIO_HRM_SENSOR);

	if (bio_hrm_sensor_hal)
		return false;

	m_accel_sensor = sensor_plugin_loader::get_instance().get_sensor(ACCELEROMETER_SENSOR);
	m_bio_sensor = sensor_plugin_loader::get_instance().get_sensor(BIO_SENSOR);

	if (!m_accel_sensor || !m_bio_sensor) {
		ERR("Fail to load sensors, accel: 0x%x, bio: 0x%x", m_accel_sensor, m_bio_sensor);
		return false;
	}

	sensor_hal *bio_sensor_hal = sensor_plugin_loader::get_instance().get_sensor_hal(BIO_SENSOR);

	if (!bio_sensor_hal) {
		ERR("Fail to load bio_sensor_hal");
		return false;
	}

	string model_id = bio_sensor_hal->get_model_id();

	m_alg = get_alg(model_id);

	if (!m_alg) {
		ERR("Not supported HRM sensor: %s", model_id.c_str());
		return false;
	}

	if (!m_alg->open())
		return false;

	set_permission(SENSOR_PERMISSION_BIO);

	INFO("%s is created!\n", sensor_base::get_name());

	return true;
}

sensor_type_t bio_hrm_virt_sensor::get_type(void)
{
	return BIO_HRM_SENSOR;
}


void bio_hrm_virt_sensor::reset(void)
{
	memset(m_acc, 0, sizeof(m_acc));

	m_hr = 0;
	m_spo2 = 0;
	m_peek_to_peek = 0;
	m_snr = 0;
	m_fired_time = 0;
}

bool bio_hrm_virt_sensor::on_start(void)
{
	const int ACCEL_SAMPLING_TIME = 10;
	const int BIO_SAMPLING_TIME = 200;

	m_accel_sensor->add_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->add_interval((int)this , ACCEL_SAMPLING_TIME, true);
	m_accel_sensor->start();


	m_bio_sensor->add_client(BIO_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_bio_sensor->add_interval((int)this , BIO_SAMPLING_TIME, true);
	m_bio_sensor->start();

	reset();

	m_alg->start();

	activate();
	return true;
}

bool bio_hrm_virt_sensor::on_stop(void)
{

	m_accel_sensor->delete_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->delete_interval((int)this , true);
	m_accel_sensor->stop();


	m_bio_sensor->delete_client(BIO_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_bio_sensor->delete_interval((int)this , true);
	m_bio_sensor->stop();

	m_alg->stop();

	deactivate();
	return true;
}

void bio_hrm_virt_sensor::synthesize(const sensor_event_t& event, vector<sensor_event_t> &outs)
{
	if (event.event_type == ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME) {
		m_acc[0] = event.data.values[0];
		m_acc[1] = event.data.values[1];
		m_acc[2] = event.data.values[2];
		return;
	}

	if (event.event_type == BIO_EVENT_RAW_DATA_REPORT_ON_TIME) {
		if ((m_acc[0] == 0) && (m_acc[1] == 0) && (m_acc[2] == 0))
			return;

		hrm_data data;

		if (m_alg->get_data(m_acc, event, data)) {
			AUTOLOCK(m_value_mutex);
			sensor_event_t hrm_event;

			hrm_event.sensor_id = get_id();
			hrm_event.data.accuracy = SENSOR_ACCURACY_GOOD;
			hrm_event.event_type = BIO_HRM_EVENT_CHANGE_STATE;
			m_fired_time = hrm_event.data.timestamp = event.data.timestamp;
			m_hr = hrm_event.data.values[0] = data.hr;
			m_spo2 = hrm_event.data.values[1] = data.spo2;
			m_peek_to_peek = hrm_event.data.values[2] = data.peek_to_peek;
			m_snr = hrm_event.data.values[3] = data.snr;
			DBG("HR: %d, SPO2: %d, PtoP: %d, SNR: %f", m_hr, m_spo2, m_peek_to_peek, m_snr);
			hrm_event.data.value_count = 4;
			outs.push_back(hrm_event);
			return;

		}
	}
	return;
}


int bio_hrm_virt_sensor::get_sensor_data(unsigned int data_id, sensor_data_t &data)
{
	if (data_id != BIO_HRM_BASE_DATA_SET)
		return -1;

	AUTOLOCK(m_value_mutex);

	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_fired_time ;
	data.value_count = 4;
	data.values[0] = m_hr;
	data.values[1] = m_spo2;
	data.values[2] = m_peek_to_peek;
	data.values[3] = m_snr;

	return 0;
}

bool bio_hrm_virt_sensor::get_properties(sensor_properties_t &properties)
{
	m_bio_sensor->get_properties(properties);
	properties.name = properties.name + " BIO HRM";
	return true;
}

hrm_alg* bio_hrm_virt_sensor::get_alg(const string &model_id)
{
	static const string ADI("ADPD");
	static const string MAXIM("MAX");

	if (model_id.compare(0, MAXIM.length(), MAXIM) == 0)
		return new hrm_alg_maxim();
	else if (model_id.compare(0, ADI.length(), ADI) == 0)
		return new hrm_alg_adi();

	return NULL;
}

extern "C" sensor_module* create(void)
{
	bio_hrm_virt_sensor *sensor;

	try {
		sensor = new(std::nothrow) bio_hrm_virt_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
