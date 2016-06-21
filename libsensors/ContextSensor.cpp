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

#include "ContextSensor.h"

/* SSP Sensorhub local quirks */
#define DELAYPATH 						"/sys/devices/virtual/sensors/ssp_sensor/accel_poll_delay"

#define SENSORHUB_ID_SENSOR 			(0)
#define SENSORHUB_ID_GESTURE			(1)

#define SENSORHUB_TYPE_CONTEXT			1
#define SENSORHUB_TYPE_GESTURE			2

#define EVENT_TYPE_CONTEXT_DATA		 	REL_RX
#define EVENT_TYPE_LARGE_CONTEXT_DATA   REL_RY
#define EVENT_TYPE_CONTEXT_NOTI		 	REL_RZ

#define SSPCONTEXT_DEVICE				"/dev/ssp_sensorhub"
#define SSP_INPUT_NODE_NAME				"ssp_context"
#define SSP_ENABLE_PATH					"/sys/devices/virtual/sensors/ssp_sensor/enable"
#define SENSORHUB_IOCTL_MAGIC			'S'
#define IOCTL_READ_LARGE_CONTEXT_DATA	_IOR(SENSORHUB_IOCTL_MAGIC, 3, char *)

// Based command reverse engineered from tizen
#define SSP_CMD_ENABLE 			(char)-79 // library add
#define SSP_CMD_DISABLE			(char)-78 // library remove
#define SSP_CMD_STEP_COUNTER	44 // step counter - sensor type
#define SSP_CMD_STEP_DETECTOR	45 // step detector - sensor type
#define SSP_CMD_GESTURE			50 // tilt gesture - sensor type
#define SSP_CMD_LPM_MODE		8 // This should be called when screen goes off

/* End */
/*
 *	TODO:
 *		Context sensors depends on knowing wether display is on,
 *		battery is charging and current time to accurately report
 *		step data and enter a low power mode to not drain the entire
 *		battery. Unless I can find how to be called from somewhere
 *		for reporting, I can't finish that part
 *
 *	There are a fucking lot of codes to retrieve data from the context
 *	sensor. I need to find a way of knowing which code is what and what
 *  are the available answers to actually do a good job here.
 */
ContextSensor::ContextSensor()
    : SensorBase(SSPCONTEXT_DEVICE, SSP_INPUT_NODE_NAME),
      mEnabled(0),
      mOriEnabled(false),
      mInputReader(8),
      mPendingEventsMask(0)
{
	
	static const char SMART_ALERT_MOTION_ENABLE[] =  { SSP_CMD_ENABLE,  8, 0, 0 };
	static const char SMART_ALERT_MOTION_DISABLE[] = { SSP_CMD_DISABLE,  8, 0, 0 };
	int dev_fd, ret;
	mStepsSinceReboot = 0;
	/* SSP Gesture detection */
    mPendingEvents[GESTURE].version = sizeof(sensors_event_t);
    mPendingEvents[GESTURE].sensor = ID_GES;
    mPendingEvents[GESTURE].type = SENSOR_TYPE_WAKE_GESTURE;
    memset(mPendingEvents[GESTURE].data, 0, sizeof(mPendingEvents[GESTURE].data));
    mPendingEventsFlushCount[GESTURE] = 0;
	/* SSP Step Detector */
    mPendingEvents[STEP_DETECT].version = sizeof(sensors_event_t);
    mPendingEvents[STEP_DETECT].sensor = ID_STD;
    mPendingEvents[STEP_DETECT].type = SENSOR_TYPE_STEP_DETECTOR;
    memset(mPendingEvents[STEP_DETECT].data, 0, sizeof(mPendingEvents[STEP_DETECT].data));
    mPendingEventsFlushCount[STEP_DETECT] = 0;
	/* SSP Step Counter */
    mPendingEvents[STEP_COUNT].version = sizeof(sensors_event_t);
    mPendingEvents[STEP_COUNT].sensor = ID_STC;
    mPendingEvents[STEP_COUNT].type = SENSOR_TYPE_STEP_COUNTER;
    memset(mPendingEvents[STEP_COUNT].data, 0, sizeof(mPendingEvents[STEP_COUNT].data));
    mPendingEventsFlushCount[STEP_COUNT] = 0;

	if (data_fd) {
      strcpy(input_sysfs_path, "/sys/class/input/");
      strcat(input_sysfs_path, input_name);
      strcat(input_sysfs_path, "/device/");
      input_sysfs_path_len = strlen(input_sysfs_path);
    }
	// First init of the SSP_Sensor
	dev_fd = open(SSPCONTEXT_DEVICE, O_RDWR);
	ret = write(dev_fd, SMART_ALERT_MOTION_ENABLE, sizeof(SMART_ALERT_MOTION_ENABLE));
	close(dev_fd);
}

ContextSensor::~ContextSensor()
{
    if (mEnabled & MODE_GESTURE)
        enable(ID_GES, 0);
    else if (mEnabled & MODE_STEPDETECT)
        enable(ID_STD, 0);
	else if (mEnabled & MODE_STEPCOUNT)
        enable(ID_STC, 0);
}
/*
 *	SSP Command structure:
 *	Commands are sent to /dev/ssp_sensorhub, structured as follows:
 *
 *	[INSTRUCTION], [SENSOR_TYPE], [DATA0], [DATA1],...[DATA_N]
 *	
 *	There are a lot of commands we're not interested in, but we need some of them:
 *	-79 == MSG2SSP_INST_LIBRARY_ADD
 *  -78 == MSG2SSP_INST_LIBRARY_REMOVE
 *
 * Commans starting by 1 are received from SSP to the kernel,
 * and they are the returned data
 */
int ContextSensor::enable(int32_t handle, int en)
{
    uint32_t enable = en ? 1 : 0;
	static const char pedo_noti[] = {1, 1, 3};
	static const char pedo_noti_ext[] = {1, 3, 3, 1};
	
	static const char  display_on_noti_cmd[] = {(char)-76, 13, (char)-47, 0};
	static const char display_off_noti_cmd[] = {(char)-76, 13, (char)-46, 0};
	
	static const char charger_connected_noti_cmd[] = {(char)-76, 13, (char)-42, 0};
	static const char charger_disconnected_noti_cmd[] = {(char)-76, 13,(char)-41, 0};
	
	static const char current_time_noti_cmd[] = {(char)-63, 14, 0, 0, 0};
	static const char reset_noti[] = {2, 1, (char)-43};
																	// First param is part of sensor type, the rest... What is all this data? timestamps?
	static const char SSP_Pedometer_01_On[] = {SSP_CMD_ENABLE, SSP_CMD_STEP_COUNTER, 0, 0, 2,  88, 0, 19, 43, 45 }; // 177 == library add
	static const char SSP_Pedometer_02_On[] = {SSP_CMD_ENABLE, SSP_CMD_STEP_COUNTER, 3, 2, 14, 16, 2, 19, 43, 45};
	static const char SSP_Pedometer_03_On[] = {SSP_CMD_ENABLE, SSP_CMD_STEP_COUNTER, 2, 0, 0,   0, 0, 19, 43, 45 };
	
	static const char SSP_Step_Counter_On[] = {SSP_CMD_ENABLE, 3, (char)-80, 67, 1};
	static const char SSP_Step_Counter_Off[] = {SSP_CMD_DISABLE, 3, (char)-80, 67, 1};
	static const char SSP_Step_Counter_Notify_Req[] = {(char)-72, 3, 1, 0};
	
	static const char SSP_Step_Detector_On[] = {SSP_CMD_ENABLE, SSP_CMD_STEP_DETECTOR, 0, 0, 0};
	static const char SSP_Step_Detector_Off[] = {SSP_CMD_DISABLE, SSP_CMD_STEP_DETECTOR, 0, 0, 0};
	
	static const char SSP_Pedometer_01_Off[] = {SSP_CMD_DISABLE, SSP_CMD_STEP_COUNTER, 0 }; // 178 == library remove
	static const char SSP_Pedometer_02_Off[] = {SSP_CMD_DISABLE, SSP_CMD_STEP_COUNTER, 3 };
	static const char SSP_Pedometer_03_Off[] = {SSP_CMD_DISABLE, SSP_CMD_STEP_COUNTER, 2 };

	static const char SSP_Sensor_Tilt_On[] ={ SSP_CMD_ENABLE, 19, 0, 0 }; // {SSP_CMD_ENABLE, SSP_CMD_GESTURE, 0};
	static const char SSP_Sensor_Tilt_Off[] ={ SSP_CMD_DISABLE, 19, 0, 0 };// {SSP_CMD_DISABLE, SSP_CMD_GESTURE, 0};

	/* These aren't sent, they are received : 1,1, ...*/

	static const char SSP_Context_TEST_Str02[] = {1, 1, 8, 1};
	static const char SSP_Context_TEST_Str03[] = {1, 1, 19, 1};
	static const char ssplist_unkn01[] = {1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	static const char ssp_unkn02[] = {(char)-78, 50, 0, 0};
	static const char ssp_unkn03[] = { 1, 1, 44, 1, 1, 1, 4, 59, (char)-56, 59, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 72, 28 };
	
    uint32_t flag = mEnabled;
    int mask;
	int ret;
	int dev_fd;
	/* Open device FD and send the contextual data */
	dev_fd = open(SSPCONTEXT_DEVICE, O_RDWR);
	if (dev_fd>0){
    switch (handle) {
		case ID_GES:
			mask = MODE_GESTURE;
			if ((mEnabled & mask) == enable){
				ALOGE("SSPContext: Gesture Detect - mode already %i", enable);
				return 0;
			}
			if (en){
				ALOGE("SSPContext: Tilt Gesture Detector ON");
				ret = write(dev_fd, SSP_Sensor_Tilt_On, sizeof(SSP_Sensor_Tilt_On));
			}else{
				ALOGE("SSPContext: Tilt Gesture Detector OFF");
				ret = write(dev_fd, SSP_Sensor_Tilt_Off, sizeof(SSP_Sensor_Tilt_Off));
			}
			close (dev_fd);
			if (ret < 0) {
				ALOGE("SSPContext: could not change sensor state");
			return ret;
			}
			break;
		case ID_STD:
			mask = MODE_STEPDETECT;
			if ((mEnabled & mask) == enable){
				ALOGE("SSPContext: Step Detect - mode already %i", enable);
				return 0;
			}
			if (en){
				ALOGE("SSPContext: Step Detector ON");
				ret = write(dev_fd, SSP_Step_Detector_On, sizeof(SSP_Step_Detector_On));
			}else{
				ALOGE("SSPContext: Step Detector OFF");
				ret = write(dev_fd, SSP_Step_Detector_On, sizeof(SSP_Step_Detector_On));
			}
			close (dev_fd);
			if (ret < 0) {
				ALOGE("SSPContext: could not change sensor state");
				return ret;
			}
			break;
		case ID_STC:
			mask = MODE_STEPCOUNT;
			if ((mEnabled & mask) == enable){
				ALOGE("SSPContext: Step Counter - mode already %i", enable);
				return 0;
			}
			if (en){
				ALOGE("SSPContext: Step Counter ON");
				ret = write(dev_fd, SSP_Step_Counter_On, sizeof(SSP_Pedometer_01_On));
				ret = write(dev_fd, SSP_Step_Counter_Notify_Req, sizeof(SSP_Pedometer_02_On));
			//	ret = write(dev_fd, SSP_Pedometer_03_On, sizeof(SSP_Pedometer_03_On));
			}else{
				ALOGE("SSPContext: Step Counter OFF");
				ret = write(dev_fd, SSP_Step_Counter_Off, sizeof(SSP_Pedometer_01_On));
				//ret = write(dev_fd, SSP_Step_Counter_Notify_Req, sizeof(SSP_Pedometer_02_On));
				//ret = write(dev_fd, SSP_Pedometer_03_Off, sizeof(SSP_Pedometer_03_Off));
			}
			close (dev_fd);
			if (ret < 0) {
				ALOGE("SSPContext: could not change sensor state");
			return ret;
			}
			break;
		default:
			ALOGE("SSPContext: unknown handle %d", handle);
			return -1;
		} // end of switch
	} // end if dev_fd is correctly opened
    if (enable)
        flag |= mask;
    else
        flag &= ~mask;

    mEnabled = flag;

    return 0;
}
/*
int ContextSensor::InformCurrentTime(){
	struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    uint64_t miliseconds = 1000.0 * res.tv_sec + (double) res.tv_nsec / 1e6;
	int seconds = (int) (milliseconds / 1000) % 60 ;
	int minutes = (int) ((milliseconds / (1000*60)) % 60);
	int hours   = (int) ((milliseconds / (1000*60*60)) % 24);
	static const char current_time_noti_cmd[] = {(char)-63, 14, hours, minutes, seconds};
	dev_fd = open(SSPCONTEXT_DEVICE, O_RDWR);
	ALOGE("SSPContext: Tick, tock, Im a clock");
	ret = write(dev_fd, current_time_noti_cmd, sizeof(current_time_noti_cmd));
	close(dev_fd);
}*/
int ContextSensor::setDelay(int32_t handle, int64_t ns)
{
    int delay = ns;
	int fd;
/*
 *
 *	Delays are set via commands through /dev/ssp_sensor, like
 *	LIBRARY_ADD and LIBRARY_REMOVE commands
 *	Need to find out exactly how or if I can bypass them and
 *	Android will still be happy
 */
    switch (handle) {
    case ID_GES:
        ALOGE("SSPContext: Tilt Gesture: delay=%lld", ns);
        break;
    case ID_STD:
        ALOGE("SSPContext: Step Detect: delay=%lld", ns);
        break;
    case ID_STC:
        /* Significant motion sensors should not set any delay */
         ALOGE("SSPContext: Step Count: delay=%lld", ns);
        break;
	default:
		ALOGE("SSPContext Delay: Unknown handle %i", handle);
		break;
	}


    return 0;
}

int ContextSensor::readEvents(sensors_event_t* data, int count)
{
	char bigBuffer[256];
	int bufferSize;
	ALOGE("SSPContext: Read Events start");
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
		int context_len = 0;
        int type = event->type;
        if (type == EV_REL) {
            float value = event->value;
            if (event->code == EVENT_TYPE_CONTEXT_DATA) {
				ALOGE("SSPContext: Context_data - Value %f", value);
				bufferSize = value;
				context_len = read_context_data(bufferSize);
            } else if (event->code == EVENT_TYPE_LARGE_CONTEXT_DATA) {
				ALOGE("SSPContext: Large Context Data - Value %f", value);
				bufferSize = value;
				context_len = read_large_context_data(bufferSize);
            } else if (event->code == EVENT_TYPE_CONTEXT_NOTI) {
				context_len = 3;
				ALOGE("SSPSSPContext: SSP Notification Code %f", value);
            } else {
                ALOGE("SSPContext: unknown event (type=%d, code=%d)",
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
            ALOGE("SSPContext: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}
int ContextSensor::read_context_data(int bufferSize)
{
	static const char SSP_Gesture[]    = { 1, 1, 19, 1 };	
	static const char SSP_StepDetect[] = { 1, 1, 46, 4, 0, 0, 0, 0, 0 };
	char fullBuff[256];
	int ret = 0;
	int device;
	int i=0;
	uint64_t steps;
	device = open(SSPCONTEXT_DEVICE, O_RDONLY);
	if (device < 0){
		ALOGE("SSPContext: Error opening device");
		return 0;
	}
	else {
		ret = read(device, fullBuff, bufferSize);
		ALOGE("SSPContext: Buffersize: %i, data: %s, ret: %i", bufferSize, fullBuff, ret);
		for (i = 0 ; i< ret; i++){
			ALOGE("SSPContext: Report: %d", (signed char)fullBuff[i]);
			}
		close(device);
	}
//	if (ret > 3){
	if( fullBuff[0] == 1 && fullBuff[1] == 1 && fullBuff[2] == 19 && fullBuff[3] == 1){ // Gesture, working nicely
		mPendingEventsMask |= 1 << GESTURE;
		mPendingEvents[GESTURE].data[0] = 1.0f;
	}else if (fullBuff[0] == 1 && fullBuff[1] == 1 && fullBuff[2] == 3 && fullBuff[3] ==1){ // Step detect, not working
		ALOGE("SSPContext: Step Detected! all Steps: %llu", mStepsSinceReboot);
		mPendingEventsMask |= 1 << STEP_DETECT;
		mPendingEvents[STEP_DETECT].data[0] = 1;
		mPendingEvents[STEP_DETECT].data[1] = 0.f;
		mPendingEvents[STEP_DETECT].data[2] = 0.f;
		mStepsSinceReboot++;
		
	}else if (fullBuff[0] == 1 && fullBuff[1] == 1 && fullBuff[2] == 3 && fullBuff[3] ==2){ // Step counter, not working
		ALOGE("SSPContext: Step Counter Event! all Steps: %llu", mStepsSinceReboot);
		mPendingEventsMask |= 1 << STEP_COUNT;
		mStepsSinceReboot++;
		mPendingEvents[STEP_COUNT].u64.step_counter =  mStepsSinceReboot;
	}
	//}
	
	
	return ret;
}
int ContextSensor::read_large_context_data(int bufferSize)
{
	
	char fullBuff[256];
	int ret = 0;
	int device;
	int i=0;
	ALOGE("SSPContext: read_context_data called!");
	device = open(SSPCONTEXT_DEVICE, O_RDONLY);
	if (device < 0){
		ALOGE("SSPContext: Error opening device");
		return 0;
	}
	else {
		ret = ioctl(device, IOCTL_READ_LARGE_CONTEXT_DATA, fullBuff);//read(device, fullBuff, bufferSize);
		ALOGE("SSPContext: Buffersize: %i, data: %s, ret: %i", bufferSize, fullBuff, ret);
		for (i = 0 ; i< ret; i++){
			ALOGE("SSPContext: Report: %d", (signed char)fullBuff[i]);
			}
		close(device);
	}
	ALOGE("SSPContext: Read Large Context Data IS NOT implemented!");
	return ret;
}
int ContextSensor::flush(int handle)
{
    int id;

    switch (handle) {
    case ID_GES:
        id = GESTURE;
        break;
    case ID_STD:
        id = STEP_DETECT;
        break;
    case STEP_COUNT:
        /* One-shot sensors must return -EINVAL */
        return -EINVAL;
    default:
        ALOGE("SSPContext: unknown handle %d", handle);
        return -EINVAL;
    }

    if (!(mEnabled & indexToMask(id)))
        return -EINVAL;

    mPendingEventsFlushCount[id]++;

    return 0;
}
