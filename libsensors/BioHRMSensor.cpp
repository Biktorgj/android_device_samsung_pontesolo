/*
 * Copyright (C) 2015 The CyanogenMod Project
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <string.h>
#include <cutils/log.h>

#include "BioHRMSensor.h"
#define DELAYPATH "/sys/devices/virtual/sensors/ssp_sensor/hrm_poll_delay"
BioHRMSensor::BioHRMSensor()
    : SensorBase(NULL, "hrm_lib_sensor"),
	  mEnabled(0),
      mInputReader(4),
	  mPendingEventsFlushCount(0)
{
    mPendingEvents.version = sizeof(sensors_event_t);
    mPendingEvents.sensor = ID_HRM;
    mPendingEvents.type = SENSOR_TYPE_HEART_RATE;
    memset(mPendingEvents.data, 0, sizeof(mPendingEvents.data));

	if (data_fd) {
      strcpy(input_sysfs_path, "/sys/class/input/");
      strcat(input_sysfs_path, input_name);
      strcat(input_sysfs_path, "/device/");
      input_sysfs_path_len = strlen(input_sysfs_path);
    }
}

BioHRMSensor::~BioHRMSensor()
{
    if (mEnabled)
        enable(ID_HRM, 0);
}

int BioHRMSensor::enable(int32_t, int en)
{
    uint32_t enable = en ? 1 : 0;
    uint32_t flag = mEnabled;
	
	int ret = sspEnable("HRM", SSP_HRM, en);
    if (ret < 0) {
        ALOGE("HRM: could not change sensor state");
        return ret;
    }
    mEnabled = flag;
	mLastBPM = 0;
    return 0;
}

int BioHRMSensor::setDelay(int32_t, int64_t ns)
{
    int delay = ns;
	int fd;

	fd = open(DELAYPATH, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%i", delay); // Some flooring to match stock value
        write(fd, buf, strlen(buf)+1);
        close(fd);
    }
	
    return 0;
}

int BioHRMSensor::readEvents(sensors_event_t* data, int count)
{
	const float SNR_SIG_FIGS = 10000.0f;
	float HR_MAX = 300;
	float RAW_UNIT = 1;
	struct timespec ts;
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;
	bool hasEvent = false;

    while (count && mPendingEventsFlushCount > 0) {
		ALOGE("BioHRM: Count: %i, FlushCount: %i", count, mPendingEventsFlushCount);
        sensors_meta_data_event_t flushEvent;
        flushEvent.version = META_DATA_VERSION;
        flushEvent.type = SENSOR_TYPE_META_DATA;
        flushEvent.meta_data.what = META_DATA_FLUSH_COMPLETE;
        flushEvent.meta_data.sensor = mPendingEvents.sensor;
        flushEvent.reserved0 = 0;
        flushEvent.timestamp = getTimestamp();
        *data++ = flushEvent;
        mPendingEventsFlushCount--;
        count--;
        numEventReceived++;
    }


    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
		int bpmm;
        if (type == EV_REL) {
            float value = event->value;
			
			switch (event->code){
				case REL_X:
					// Beats per minute
					if (event->value >= HR_MAX || event->value <= RAW_UNIT){
						ALOGV("BioHRM: Sensor Unreliable!");
					}else{
						
						if (value != 0 && value != mLastBPM){
							ALOGE("BioHRM: %f BPM", value);
							mPendingEvents.timestamp = timevalToNano(event->time);
							mPendingEvents.heart_rate.bpm = value;
							mPendingEvents.heart_rate.status = SENSOR_STATUS_ACCURACY_HIGH;
							mLastBPM = value;
							*data++ = mPendingEvents;
							count--;
							numEventReceived++;
							ALOGE("BioHRM: Sync!");
							}
			
					}
					break;
				case REL_Y:
					// Peek to Peek value
					ALOGV("BioHRM - Peek to Peek: %i", event->value);
					break;
				case REL_Z:
					// SNR level for accuracy, not used in wear, and since we're
					// using SSP for processing, we dont care either
					ALOGV("BioHRM - SNR: %i", event->value);
					if (event->value == 1){
						ALOGV("HRM: First run!");
					}
					break;
				default:
					ALOGE("BioHRM Sensor Error: Default case. Code %i, Value %i", event->code, event->value);
					break;
				}
          
        } else if (type == EV_SYN) {
            mPendingEvents.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvents;
                count--;
                numEventReceived++;
				ALOGE("BioHRM: Sync!");
            }
        } else {
            ALOGE("HRM: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

int BioHRMSensor::flush(int)
{
    if (!(mEnabled))
        return -EINVAL;

    mPendingEventsFlushCount++;

    return 0;
}
