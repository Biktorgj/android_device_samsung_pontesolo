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

#include "LightSensor.h"
#define DELAYPATH "/sys/devices/virtual/sensors/ssp_sensor/light_poll_delay"

LightSensor::LightSensor()
    : SensorBase(NULL, "light_sensor"),
      mInputReader(4),
      mPendingEventsMask(0)
{
    mEnabled[LIGHT] = false;
    mPendingEvents[LIGHT].version = sizeof(sensors_event_t);
    mPendingEvents[LIGHT].sensor = ID_L;
    mPendingEvents[LIGHT].type = SENSOR_TYPE_LIGHT;
    memset(mPendingEvents[LIGHT].data, 0, sizeof(mPendingEvents[LIGHT].data));
    mPendingEventsFlushCount[LIGHT] = 0;

   if (data_fd) {
       strcpy(input_sysfs_path, "/sys/class/input/");
       strcat(input_sysfs_path, input_name);
       strcat(input_sysfs_path, "/device/");
       input_sysfs_path_len = strlen(input_sysfs_path);
       enable(0, 1);
    }
}

LightSensor::~LightSensor()
{
    if (mEnabled[LIGHT])
        enable(ID_L, 0);
}

int LightSensor::setDelay(int32_t handle, int64_t ns)
{
	int fd;
	int delay = ns;
    switch (handle) {
    case ID_L:
        ALOGE("Light: delay=%lld ns", ns);
        break;
    default:
        ALOGE("LightSensor: unknown handle %d", handle);
        return -EINVAL;
    }

  	fd = open(DELAYPATH, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%i", delay); // Some flooring to match stock value
        write(fd, buf, strlen(buf)+1);
        close(fd);
       }
    return 0;
}

int LightSensor::enable(int32_t handle, int en)
{
    char enable_path[PATH_MAX];
    bool enable = !!en;
    int sensor;
	int ret;

    switch (handle) {
    case ID_L:
        ALOGE("Light: enable=%d", en);
        sensor = LIGHT;
        ret = sspEnable("Light Sensor", SSP_LIGHT, en);
        break;
    default:
        ALOGE("LightSensor: unknown handle %d", handle);
        return -EINVAL;
    }

    if (enable == mEnabled[sensor])
        return 0;

    mEnabled[sensor] = enable;
    return 0;
}

int LightSensor::readEvents(sensors_event_t* data, int count)
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
        if (type == EV_ABS) {
            if (event->code == ABS_MISC) {
                mPendingEventsMask |= 1 << LIGHT;
                mPendingEvents[LIGHT].light = event->value;
            } else {
                ALOGE("Light: unknown event (type=%d, code=%d)",
                        type, event->code);
            }
        }else if (type == EV_REL){
			if (event->code == REL_RX) {
					 mPendingEventsMask |= 1 << LIGHT;
                     mPendingEvents[LIGHT].light = event->value -1; // Substract 1 from BIAS def as in tizen
					 }
        } else if (type == EV_SYN) {
            for (int i = 0; count && mPendingEventsMask && i < NUM_SENSORS; i++) {
                if (mPendingEventsMask & (1 << i)) {
                    mPendingEventsMask &= ~(1 << i);
                    mPendingEvents[i].timestamp = timevalToNano(event->time);
                    if (mEnabled[i]) {
                        *data++ = mPendingEvents[i];
                        count--;
                        numEventReceived++;
                    }
                }
            }
        } else {
            ALOGE("LightSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

int LightSensor::flush(int handle)
{
    int id;

    switch (handle) {
    case ID_L:
        id = LIGHT;
        break;
    default:
        ALOGE("LightSensor: unknown handle %d", handle);
        return -EINVAL;
    }

    if (!mEnabled[id])
        return -EINVAL;

    mPendingEventsFlushCount[id]++;

    return 0;
}
