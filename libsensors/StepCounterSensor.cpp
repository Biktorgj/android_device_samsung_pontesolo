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

#include "StepCounterSensor.h"
#include "SensorBase.h"

#define LOGTAG "StepCounterSensor"

/*****************************************************************************/

StepCounterSensor::StepCounterSensor()
    : SensorBase(NULL, "step_counter"), // Gear S: input15
      mEnabled(0),
      mInputReader(4),
      mHasPendingEvent(false),
	  mEnabledTime(0)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_SSP;
    mPendingEvent.type = SENSOR_TYPE_STEP_COUNTER;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    if (data_fd) {
        strcpy(input_sysfs_path, "/sys/class/input/");
        strcat(input_sysfs_path, input_name);
        strcat(input_sysfs_path, "/device/");
        input_sysfs_path_len = strlen(input_sysfs_path);
    }
}

StepCounterSensor::~StepCounterSensor() {
    if (mEnabled) {
        enable(0, 0);
    }
}

int StepCounterSensor::setInitialState() {
	/*
	*
	*
	*
	*
	*
	*/
	ALOGE("StepCounterSensor: Set Initial State");
    struct input_absinfo absinfo_step;
    if (!ioctl(data_fd, EVIOCGABS(REL_MISC), &absinfo_step)){
        // make sure to report an event immediately
		ALOGE("StepCounterSensor: There were pending events!");
        mHasPendingEvent = true;
        //mPendingEvent.distance = indexToValue(absinfo.value);
    } else{
		ALOGE("StepCounterSensor: It seems there weren't pending events. Fuck it!, we'll do it anyway");
		mHasPendingEvent = true;
	}
    return 0;
}

int StepCounterSensor::setDelay(int32_t handle, int64_t ns)
{
	
	/* We're not here yet */
  /*  int fd;

    strcpy(&input_sysfs_path[input_sysfs_path_len], "prox_poll_delay");
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%lld", ns);
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return -1;*/
	ALOGE("StepCounterSensor: Set delay");
	/*
	*	We can call it as many times as we want
	*
	*/
	return 0;
}

int StepCounterSensor::enable(int32_t handle, int en) {

    int flags = en ? 1 : 0;
    int err;
    ALOGE("StepCounterSensor: Enable: %i", en);
    if (flags != mEnabled) {
         err = sspEnable(LOGTAG, SSP_STEP, en);
         if(err >= 0){
             mEnabled = flags;
             setInitialState();

             return 0;
         }
         return -1;
    }
    return 0;
}

bool StepCounterSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

	/*
	* NEW PLAN: FORGET THE OLD PLAN
	* So, with tizen, these guys are directly querying the MCU to get all the data they want.
	* But they only do it for two devices? why? god fucking knows why. I could get if they 
	* used a HAL to directly communicate to all the sensors, but accel, gyro, bio, uv, all
	* have their fucking device node, except for the step sensor, and the significant motion
	* sensor, which is used for what? for step detection! oh yes, and tilt to wake. Which is
	* not even used, because we have the fucking accelerometer for that. In fact, tilt to wake
	* is read from the fucking accelerometer.
	*
	* Getting tired of this shit Samsung. Nooo! I'm Samsung and do whatever I want! Fuck logic!
	* Fuck standards! Fuck my own code because this code makes even less sense! yayyy!
	*
	* So, I coded new device input nodes to actually _READ_ the step counter data from the MCU
	* into the Sensorhub's kernel module. WHICH IS WHAT THEY SHOULD HAVE DONE ANYWAY. They do
    * it in Android, for fucks' sake. So now I need to hook into the input device and see what 
	* is coming through. I chose REL_MISC because it was there. Let's see if this works.
	*
	* Not hoping much, since I'm copying this directly from Gear Live's IndustrialIO kernel module
	* While most of the code should work, I'm overlapping one of the indexes with the UV SensorBase
	* So, chances are, the index point for the step counter is something else and I will need to fuck
	* around with Tizen until I get to see the entry point in the MCU. We'll see. For now, just compile.
	*/

int StepCounterSensor::readEvents(sensors_event_t* data, int count)
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

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_REL) {
            float value = event->value;
            if (event->code == REL_MISC) 
                mPendingEvent.u64.step_counter = value * CONVERT_A_X;
			} else if (type == EV_SYN) {
				mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            ALOGE("%s: unknown event (type=%d, code=%d)", LOGTAG,
                    type, event->code);
        }

        mInputReader.next();
    }
    return numEventReceived++;

}

