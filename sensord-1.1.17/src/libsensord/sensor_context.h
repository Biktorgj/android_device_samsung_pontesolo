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

#ifndef __SENSOR_CONTEXT_H__
#define __SENSOR_CONTEXT_H__

//! Pre-defined events for the context sensor
//! Sensor Plugin developer can add more event to their own headers

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup SENSOR_CONTEXT Context Sensor
 * @ingroup SENSOR_FRAMEWORK
 *
 * These APIs are used to control the Context sensor.
 * @{
 */

enum context_data_id {
	CONTEXT_BASE_DATA_SET	= (CONTEXT_SENSOR << 16) | 0x0001,
};

enum context_event_type {
	CONTEXT_EVENT_REPORT	= (CONTEXT_SENSOR << 16) | 0x0001,
};

enum context_property_id {
	CONTEXT_PROPERTY_UNKNOWN	= 0,
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
//! End of a file
