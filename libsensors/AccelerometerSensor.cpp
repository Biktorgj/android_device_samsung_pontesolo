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

#include "AccelerometerSensor.h"
#define DELAYPATH "/sys/devices/virtual/sensors/ssp_sensor/accel_poll_delay"
AccelerometerSensor::AccelerometerSensor()
    : SensorBase(NULL, "accelerometer_sensor"),
      mEnabled(0),
      mOriEnabled(false),
      mInputReader(8),
      mPendingEventsMask(0)
{
    mPendingEvents[ACC].version = sizeof(sensors_event_t);
    mPendingEvents[ACC].sensor = ID_A;
    mPendingEvents[ACC].type = SENSOR_TYPE_ACCELEROMETER;
    memset(mPendingEvents[ACC].data, 0, sizeof(mPendingEvents[ACC].data));
    mPendingEventsFlushCount[ACC] = 0;


	if (data_fd) {
      strcpy(input_sysfs_path, "/sys/class/input/");
      strcat(input_sysfs_path, input_name);
      strcat(input_sysfs_path, "/device/");
      input_sysfs_path_len = strlen(input_sysfs_path);
    }
}

AccelerometerSensor::~AccelerometerSensor()
{
    if (mEnabled & MODE_ACCEL)
        enable(ID_A, 0);
}

int AccelerometerSensor::enable(int32_t handle, int en)
{
    uint32_t enable = en ? 1 : 0;
    uint32_t flag = mEnabled;
    int mask;

    switch (handle) {
    case ID_A:
        ALOGV("Accelerometer (ACC): enable=%d", en);
        mask = MODE_ACCEL;
        break;
      default:
        ALOGV("Accelerometer: unknown handle %d", handle);
        return -1;
    }

    if ((mEnabled & mask) == enable)
        return 0;

    if (enable)
        flag |= mask;
    else
        flag &= ~mask;

	int ret = sspEnable("Accelerometer", SSP_ACCEL, en);
    if (ret < 0) {
        ALOGV("Accelerometer: could not change sensor state");
        return ret;
    }
    mEnabled = flag;

    return 0;
}

int AccelerometerSensor::setDelay(int32_t handle, int64_t ns)
{
    int delay = ns;
	int fd;

    switch (handle) {
    case ID_A:
        ALOGV("Accelerometer (ACC): delay=%lld", ns);
        break;
    case ID_SO:
        ALOGV("Accelerometer (SO): ignoring delay=%lld", ns);
        return 0;
    case ID_SM:
        /* Significant motion sensors should not set any delay */
        ALOGV("Accelerometer (SM): ignoring delay=%lld", ns);
        return 0;
    }

    if (delay > ACC_MAX_DELAY_NS)
        delay = ACC_MAX_DELAY_NS;

    switch (handle) {
    case ID_A:
        ALOGV("Accelerometer (ACC): delay=%d", delay);
        break;
    case ID_SO:
        ALOGV("Accelerometer (SO): delay=%d", delay);
        return 0;
    case ID_SM:
        /* Significant motion sensors should not set any delay */
        ALOGV("Accelerometer (SM): delay=%d", delay);
        return 0;
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

int AccelerometerSensor::readEvents(sensors_event_t* data, int count)
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
            if (event->code == EVENT_TYPE_ACCEL_X) {
                mPendingEventsMask |= 1 << ACC;
                mPendingEvents[ACC].acceleration.x = value * CONVERT_A_X;
            } else if (event->code == EVENT_TYPE_ACCEL_Y) {
                mPendingEventsMask |= 1 << ACC;
                mPendingEvents[ACC].acceleration.y = value * CONVERT_A_Y;
            } else if (event->code == EVENT_TYPE_ACCEL_Z) {
                mPendingEventsMask |= 1 << ACC;
                mPendingEvents[ACC].acceleration.z = value * CONVERT_A_Z;
            } else {
                ALOGV("Accelerometer: unknown event (type=%d, code=%d)",
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
            ALOGV("Accelerometer: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

int AccelerometerSensor::flush(int handle)
{
    int id;

    switch (handle) {
    case ID_A:
        id = ACC;
        break;
    default:
        ALOGV("Accelerometer: unknown handle %d", handle);
        return -EINVAL;
    }

    if (!(mEnabled & indexToMask(id)))
        return -EINVAL;

    mPendingEventsFlushCount[id]++;

    return 0;
}
