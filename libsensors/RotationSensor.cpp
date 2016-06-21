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

#include "RotationSensor.h"
#define DELAYPATH "/sys/devices/virtual/sensor_event/symlink/rot_sensor/poll_delay"
RotationSensor::RotationSensor()
    : SensorBase(NULL, "rot_sensor"),
      mEnabled(0),
      mOriEnabled(false),
      mInputReader(8),
      mPendingEventsMask(0)
{
    mPendingEvents.version = sizeof(sensors_event_t);
    mPendingEvents.sensor = ID_ROT;
    mPendingEvents.type = SENSOR_TYPE_ROTATION_VECTOR;
    memset(mPendingEvents.data, 0, sizeof(mPendingEvents.data));
    mPendingEventsFlushCount = 0;

	if (data_fd) {
      strcpy(input_sysfs_path, "/sys/class/input/");
      strcat(input_sysfs_path, input_name);
      strcat(input_sysfs_path, "/device/");
      input_sysfs_path_len = strlen(input_sysfs_path);
    }
}

RotationSensor::~RotationSensor()
{
    if (mEnabled)
        enable(ID_ROT, 0);

}

int RotationSensor::enable(int32_t, int en)
{
    uint32_t enable = en ? 1 : 0;
    uint32_t flag = mEnabled;


    if ( mEnabled == enable)
        return 0;

	int ret = sspEnable("Rotation Vector Sensor", SSP_ROTV, en);
    if (ret < 0) {
        ALOGE("Rotation Vector Sensor: could not change sensor state");
        return ret;
    }
    mEnabled = enable;

    return 0;
}

int RotationSensor::setDelay(int32_t, int64_t ns)
{
    int delay = ns;
	int fd;

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

int RotationSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

 while (count && mPendingEventsFlushCount > 0) {
		ALOGE("Rotation Vector: Count: %i, FlushCount: %i", count, mPendingEventsFlushCount);
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
		if (type == EV_MSC) {
            if (event->code == EV_REL) {
			   float value = event->value;
				if (event->code == REL_X) {
					ALOGE("Rotation Vector: REL_X %f", value);
					mPendingEvents.data[0] = value;
				} else if (event->code == REL_Y) {
					ALOGE("Rotation Vector: REL_Y %f", value);
					mPendingEvents.data[1] = value;
				} else if (event->code == REL_Z) {
					ALOGE("Rotation Vector: REL_Z %f", value);
					mPendingEvents.data[2] = value;
				} else if (event->code == REL_RX){
					ALOGE("Rotation Vector: REL_RX %f", value);
					mPendingEvents.data[3] = value;
				} else {
					ALOGE("Rotation Vector Sensor: unknown event (type=%d, code=%d)",
                        type, event->code);
				}
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
            ALOGE("Rotation Vector Sensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

int RotationSensor::flush(int)
{
    if (!(mEnabled ))
        return -EINVAL;

    mPendingEventsFlushCount++;

    return 0;
}
