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

#ifndef ANDROID_BIOHRM_SENSOR_H
#define ANDROID_BIOHRM_SENSOR_H

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

class BioHRMSensor : public SensorBase {
    char input_sysfs_path[PATH_MAX];
    int input_sysfs_path_len;
	float mLastBPM;
private:
    uint32_t mEnabled;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvents;
    uint32_t mPendingEventsMask;
    int mPendingEventsFlushCount;

public:
            BioHRMSensor();
    virtual ~BioHRMSensor();
    virtual int readEvents(sensors_event_t* data, int count);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int flush(int32_t handle);
};

#endif  // ANDROID_BIOHRM_SENSOR_H
