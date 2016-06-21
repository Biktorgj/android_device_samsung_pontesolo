/*
 * Copyright (C) 2015 The CyanogenMod Project
 * Copyright (C) 2008 The Android Open SLVurce Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, SLVftware
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <string.h>
#include <cutils/log.h>

#include "PressureSensor.h"
#define DELAYPATH "/sys/devices/virtual/sensors/ssp_sensor/accel_poll_delay"

PressureSensor::PressureSensor()
    : SensorBase(NULL, "barometer_sensor"), //fix this plz
      mEnabled(0),
      mOriEnabled(false),
      mInputReader(8),
      mPendingEventsMask(0)
{
    mPendingEvents[PRS].version = sizeof(sensors_event_t);
    mPendingEvents[PRS].sensor = ID_PRS;
    mPendingEvents[PRS].type = SENSOR_TYPE_PRESSURE;
    memset(mPendingEvents[PRS].data, 0, sizeof(mPendingEvents[PRS].data));
    mPendingEventsFlushCount[PRS] = 0;

    mPendingEvents[TMP].version = sizeof(sensors_event_t);
    mPendingEvents[TMP].sensor = ID_TMP;
    mPendingEvents[TMP].type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
    memset(mPendingEvents[TMP].data, 0, sizeof(mPendingEvents[TMP].data));
    mPendingEventsFlushCount[TMP] = 0;

}

PressureSensor::~PressureSensor()
{
    if (mEnabled & MODE_PRESSURE)
        enable(ID_PRS, 0);

    if (mEnabled & MODE_TEMPERATURE)
        enable(ID_TMP, 0);

    if (mOriEnabled)
        enable(ID_O, 0);
}

int PressureSensor::enable(int32_t handle, int en)
{
    uint32_t enable = en ? 1 : 0;
    uint32_t flag = mEnabled;
    int mask;

    switch (handle) {
    case ID_PRS:
        ALOGE("PressureSensor (PRS): enable=%d", en);
        mask = MODE_PRESSURE;
		mPressureActive = en;
		mLastTemperature = 0;
        break;
    case ID_TMP:
        ALOGE("PressureSensor (TMP): enable=%d", en);
        mask = MODE_TEMPERATURE;
		mTemperatureActive = en;
        break;

    default:
        ALOGE("PressureSensor: unknown handle %d", handle);
        return -1;
    }

    if ((mEnabled & mask) == enable)
        return 0;

    if (enable)
        flag |= mask;
    else
        flag &= ~mask;

	int ret = sspEnable("PressureSensor", SSP_PRESS, en);
    if (ret < 0) {
        ALOGE("PressureSensor: could not change sensor state");
        return ret;
    }
    mEnabled = flag;

    return 0;
}

int PressureSensor::setDelay(int32_t handle, int64_t ns)
{
    int delay = ns / 1000000;
    int ret;
	int fd; 
    switch (handle) {
    case ID_PRS:
        ALOGE("PressureSensor (PRS): delay=%d", delay);
        break;
    case ID_TMP:
        ALOGE("PressureSensor (TMP): delay=%d", delay);
		break;
    }

  	fd = open(DELAYPATH, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%i", delay); // Some flooring to match stock value
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return 0;
}

int PressureSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    for (int i = 0; i < NUM_SENSORS; i++) {
        if (!count)
            break;
        if (!mPendingEventsFlushCount[i])
            continue;
        sensors_meta_data_event_t flushEvent;
        flushEvent.version = META_DATA_VERSION;
        flushEvent.type = SENSOR_TYPE_META_DATA;
        flushEvent.meta_data.what = META_DATA_FLUSH_COMPLETE;
        flushEvent.meta_data.sensor = mPendingEvents[i].sensor;
        flushEvent.reserved0 = 0;
        flushEvent.timestamp = getTimestamp();
        *data++ = flushEvent;
        mPendingEventsFlushCount[i]--;
        count--;
        numEventReceived++;
    }

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_REL) {
            float value = event->value;
            if (event->code == REL_HWHEEL || event->code == REL_X) {
				ALOGE("Pressure Sensor: Pressure! %d", event->value);
				if (mPressureActive){
					mPendingEventsMask |= 1 << PRS;
					mPendingEvents[PRS].pressure = event-> value;
				}
            } else if (event->code == REL_DIAL || event->value == REL_Y) {
				ALOGE("Pressure Sensor: Sea Level! %d", event->value);
                mPendingEventsMask |= 1 << PRS;
                mSeaLevel = event->value;
            } else if (event->code == REL_WHEEL || event->code == REL_Z) {
				ALOGE("Pressure Sensor: Temperature! %d", event->value);
				if (mPressureActive){
					mPendingEventsMask |= 1 << PRS;
					mPendingEvents[PRS].temperature = event->value;
				}else if (mTemperatureActive && mLastTemperature != event->value){
					mPendingEventsMask |= 1 << TMP;
					mPendingEvents[TMP].temperature = event->value;
					mLastTemperature = event->value;
				}
            } else {
                ALOGE("PressureSensor: unknown event (type=%d, code=%d)",
                        type, event->code);
            }

        } else if (type == EV_SYN) {
            for (int i = 0; count && mPendingEventsMask && i < NUM_SENSORS; i++) {
                if (mPendingEventsMask & (1 << i)) {
                    mPendingEventsMask &= ~(1 << i);
                    mPendingEvents[i].timestamp = timevalToNano(event->time);
                    if (mEnabled & indexToMask(i)) {
                        *data++ = mPendingEvents[i];
                        count--;
                        numEventReceived++;
                    }
				}
            }
        } else {
            ALOGE("PressureSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

int PressureSensor::flush(int handle)
{
    int id;

    switch (handle) {
    case ID_PRS:
        id = PRS;
    case ID_TMP:
        /* One-shot sensors must return -EINVAL */
        return -EINVAL;
    default:
        ALOGE("PressureSensor: unknown handle %d", handle);
        return -EINVAL;
    }

    if (!(mEnabled & indexToMask(id)))
        return -EINVAL;

    mPendingEventsFlushCount[id]++;

    return 0;
}
