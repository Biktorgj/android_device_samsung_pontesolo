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

#include "SSPContextSensor.h"
#include "SensorBase.h"

#define LOGTAG "SSPContextSensor"
#define SSP_SENSORHUB_PATH "/dev/ssp_sensorhub"

#define SENSORHUB_ID_SENSOR 			(0)
#define SENSORHUB_ID_GESTURE			(1)
#define IOCTL_READ_LARGE_CONTEXT_DATA	_IOR(SENSORHUB_IOCTL_MAGIC, 3, char *)

/* This is a different kind of sensor
	This sensor driver connects to /dev/ssp_sensorhub directly,
	and reads the data calculated by the SSP MCU (in this case the STM microcontroller)
	This gives us the Step counter, the rotation vectors etc.
	OK, corrections...
	This sensor has, on one side, an  input node. On the Gear S this is input14
	That's where we receive the notifications about sensor updates.
	On the other side, we have the device node, SSP_SENSORHUB_PATH, where we manually
	read all the pending data after having requested it and being notified in the input
	node... I think. We'll see.
	*/
/*****************************************************************************/

SSPContextSensor::SSPContextSensor()
    : SensorBase(NULL, "ssp_context"), // Gear S: input14
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

SSPContextSensor::~SSPContextSensor() {
    if (mEnabled) {
        enable(0, 0);
    }
}

int SSPContextSensor::openDevice() {
    int data_fd = -1;
    data_fd = open(SSP_SENSORHUB_PATH, O_RDONLY);
    ALOGE_IF(data_fd<0, "couldn't find '%s' input device", SSP_SENSORHUB_PATH);
    return data_fd;
}

int SSPContextSensor::setInitialState() {
	/*
	*	Context Sensor may return three different types of data:
	*	REL_RX : Context data (small)
	*	REL_RY : Large Context Data (obviously large, 
				it's a char * with an int declaring size,
				so up to 32768 bytes)
	*	REL_RZ	: Context Notification (wtf is this?)
	*/
	ALOGE("SSPContextSensor: Set Initial State");
    struct input_absinfo absinfox;
    struct input_absinfo absinfoy;
    struct input_absinfo absinfoz;
    if (!ioctl(data_fd, EVIOCGABS(REL_RX), &absinfox) ||
		!ioctl(data_fd, EVIOCGABS(REL_RY), &absinfoy) ||
		!ioctl(data_fd, EVIOCGABS(REL_RZ), &absinfoz)) {
        // make sure to report an event immediately
		ALOGE("SSPContextSensor: There were pending events!");
        mHasPendingEvent = true;
        //mPendingEvent.distance = indexToValue(absinfo.value);
    } else{
		ALOGE("SSPContextSensor: It seems there weren't pending events. Fuck it!, we'll do it anyway");
		mHasPendingEvent = true;
	}
    return 0;
}

int SSPContextSensor::setDelay(int32_t handle, int64_t ns)
{
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
	ALOGE("SSPContextSensor: Set delay");
	/*
	*	We can call it as many times as we want
	*
	*/
	return 0;
}

int SSPContextSensor::enable(int32_t handle, int en) {

    int flags = en ? 1 : 0;
    int err;
    ALOGE("SSPContextSensor: Enable: %i", en);
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

bool SSPContextSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}


int SSPContextSensor::read_context_data(bool big, int size){
	int ret = 0;
	int fd = -1;
	char data[256]; // data buffer. According to the kernel it tops at 256bytes
	ALOGE("SSPContextSensor: Starting Read Context Data");
	fd = SSPContextSensor::openDevice();
	/*
	* We send request commands to the mcu via /dev/ssp_sensorhub.
	*  
	* We send hex codes this way:
	* __ <instruction
	* 0xb1 0x32 0x00 <-- DATA
	*	   ___ <sensortype
	*
	*	This will enable the HRM and put it to read :
	*		\xb1\x2e\x02 > ssp_sensorhub
	*/
	if (size > 255)
		size = 255; // just in case...
	if (fd>0){
		switch (big){
			case false:
				ALOGE("SSPContextSensor:: We have small data pending");
				ret = read(fd, &data, size);
				ALOGE("SSPContextSensor:: **DEBUG** %s", data);
				break;
			case true:
				ALOGE("SSPContextSensor: We have big data to parse!");
				// We'll wait for this...
				break;
			default:
				ALOGE("SSPContextSensor: Whoops, we hit the default case -readcontext-");
				break;
			}
	ret = close(fd);
	}else{
		ALOGE("ERROR OPENING DEVICE!");
	}
	return 0;
}
int SSPContextSensor::readEvents(sensors_event_t* data, int count)
{ int ret = 0;

    if (count < 1)
        return -EINVAL;
	ALOGW("SSPContextSensor: Read Events function called.");
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
			switch (event->code){
				case REL_RX: // Context Data
					ALOGW("SSPContextSensor: REL_RX");
					ret = SSPContextSensor::read_context_data(false, value);
					break;
				case REL_RY: // Large Context Data
					ALOGW("SSPContextSensor: REL_RY");
					ret = SSPContextSensor::read_context_data(true, value);
					break;
				case REL_RZ: // Context Notification
					ALOGW("SSPContextSensor: REL_RZ");
					// Should I do something with this? trigger something?
					break;
				default: // just in case so we know what came in
					ALOGE("SSPContextSensor: Default case hit: Type: %i, Code %i", event->type, value);
					break;
                }
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            ALOGE("%s: unknown event (type=%d, code=%d)",LOGTAG,
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

float SSPContextSensor::indexToValue(size_t index) const
{
    ALOGV("%s: Index = %zu",LOGTAG, index);
    return index;
}
