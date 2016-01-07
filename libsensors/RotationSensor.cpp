/*
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
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <stdio.h>

#include <utils/Log.h>

#include "RotationSensor.h"
#include "SensorBase.h"

#define LOGTAG "RotationSensor"

/*****************************************************************************/

RotationSensor::RotationSensor()
    : SensorBase(NULL, "rot_sensor"),
      mEnabled(0),
      mInputReader(4),
      mHasPendingEvent(false),
	  mEnabledTime(0)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_ROT;
    mPendingEvent.type = SENSOR_TYPE_ROTATION_VECTOR;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    if (data_fd) {
        strcpy(input_sysfs_path, "/sys/class/input/");
        strcat(input_sysfs_path, input_name);
        strcat(input_sysfs_path, "/device/");
        input_sysfs_path_len = strlen(input_sysfs_path);
    }
}

RotationSensor::~RotationSensor() {
    if (mEnabled) {
        enable(0, 0);
    }
}

int RotationSensor::setInitialState() {
   struct input_absinfo absinfo_x;
    struct input_absinfo absinfo_y;
    struct input_absinfo absinfo_z;
    struct input_absinfo absinfo_rx;
    struct input_absinfo absinfo_ry;
    float value;
	/*
	* Rotation vector kernel driver returns 5 values:
	* X, Y, Z, RX & RY (first axis, then angles)
	*/
    if (!ioctl(data_fd, EVIOCGABS(REL_X), &absinfo_x) &&
        !ioctl(data_fd, EVIOCGABS(REL_Y), &absinfo_y) &&
		!ioctl(data_fd, EVIOCGABS(REL_Z), &absinfo_z) &&
		!ioctl(data_fd, EVIOCGABS(REL_RX), &absinfo_rx)) {
  // RY returns the accuracy bias and android doesn't use that...
       // !ioctl(data_fd, EVIOCGABS(REL_RY), &absinfo_ry)) {
        value = absinfo_x.value;
        mPendingEvent.data[0] = value;
        value = absinfo_y.value;
        mPendingEvent.data[1] = value;
        value = absinfo_z.value;
        mPendingEvent.data[2] = value;
        value = absinfo_rx.value;
        mPendingEvent.data[3] = value;
        //value = absinfo_ry.value;
        //mPendingEvent.data[4] = value;
        mHasPendingEvent = true;
    }
    return 0;
}

int RotationSensor::setDelay(int32_t handle, int64_t ns)
{
    int fd;

    strcpy(&input_sysfs_path[input_sysfs_path_len], "rot_poll_delay");
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%lld", ns);
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return -1;
}

int RotationSensor::enable(int32_t handle, int en) {

    int flags = en ? 1 : 0;
    int err;
    ALOGE("%s: Enable: %i", __func__, en);
    if (flags != mEnabled) {
         err = sspEnable(LOGTAG, SSP_ROTV, en);
         if(err >= 0){
             mEnabled = flags;
             setInitialState();

             return 0;
         }
         return -1;
    }
    return 0;
}

bool RotationSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int RotationSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;
#if FETCH_FULL_EVENT_BEFORE_RETURN
again:
#endif
    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_REL) {
			ALOGE("Rotation Vector: EV_REL - Code: %i", event->code);
            float value = event->value;
            if (event->code == REL_X) {
				ALOGE("Rotation Vector: REL_X %i", event->code);
                mPendingEvent.data[0] = value;
            } else if (event->code == REL_Y) {
				ALOGE("Rotation Vector: REL_Y %i", event->code);
                mPendingEvent.data[1] = value;
            } else if (event->code == REL_Z) {
				ALOGE("Rotation Vector: REL_Z %i", event->code);
                mPendingEvent.data[2] = value;
            } else if (event->code == REL_RX){
				ALOGE("Rotation Vector: REL_RX %i", event->code);
				mPendingEvent.data[3] = value;
			}
		/* If accuracy is bad, we should implement something here
		and use REL_RY data to fix it... */
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                if (mPendingEvent.timestamp >= mEnabledTime) {
                    *data++ = mPendingEvent;
                    numEventReceived++;
                }
                count--;
            }
        } else {
            ALOGE("%s: unknown event (type=%d, code=%d)", LOGTAG,
                    type, event->code);
        }
        mInputReader.next();
    }

#if FETCH_FULL_EVENT_BEFORE_RETURN
    /* if we didn't read a complete event, see if we can fill and
       try again instead of returning with nothing and redoing poll. */
    if (numEventReceived == 0 && mEnabled == 1) {
        n = mInputReader.fill(data_fd);
        if (n)
            goto again;
    }
#endif

    return numEventReceived;
}

