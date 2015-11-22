/*
 * libsensord
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _SENSOR_AUTO_ROTATION_H_
#define _SENSOR_AUTO_ROTATION_H_

//! Pre-defined events for the auto_rotation sensor
//! Sensor Plugin developer can add more event to their own headers

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup SENSOR_AUTO_ROTATION Auto Rotation Sensor
 * @ingroup SENSOR_FRAMEWORK
 *
 * These APIs are used to control the Auto Rotation sensor.
 * @{
 */

enum auto_rotation_data_id {
	AUTO_ROTATION_BASE_DATA_SET	= (AUTO_ROTATION_SENSOR << 16) | 0x0001,
};

enum auto_rotation_event_type {
	AUTO_ROTATION_EVENT_CHANGE_STATE = (AUTO_ROTATION_SENSOR << 16) | 0x0001,
};

enum auto_rotation_state {
	AUTO_ROTATION_DEGREE_UNKNOWN = 0,
	AUTO_ROTATION_DEGREE_0,
	AUTO_ROTATION_DEGREE_90,
	AUTO_ROTATION_DEGREE_180,
	AUTO_ROTATION_DEGREE_270,
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
//! End of a file
