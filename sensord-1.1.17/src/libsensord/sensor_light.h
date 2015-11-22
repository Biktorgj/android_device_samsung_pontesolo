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

#ifndef __SENSOR_LIGHT_H__
#define __SENSOR_LIGHT_H__

//! Pre-defined events for the light sensor
//! Sensor Plugin developer can add more event to their own headers

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup SENSOR_LIGHT Light Sensor
 * @ingroup SENSOR_FRAMEWORK
 * 
 * These APIs are used to control the light sensor.
 * @{
 */

enum light_data_id {
	LIGHT_LUX_DATA_SET	= (LIGHT_SENSOR << 16) | 0x0001,
	LIGHT_BASE_DATA_SET	= (LIGHT_SENSOR << 16) | 0x0002,
};

enum light_evet_type {			
	LIGHT_EVENT_LUX_DATA_REPORT_ON_TIME		= (LIGHT_SENSOR << 16) | 0x0001,
	LIGHT_EVENT_LEVEL_DATA_REPORT_ON_TIME 	= (LIGHT_SENSOR << 16) | 0x0002,
	LIGHT_EVENT_CHANGE_LEVEL				= (LIGHT_SENSOR << 16) | 0x0004,
};

enum light_property_id {
	LIGHT_PROPERTY_UNKNOWN = 0,
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
//! End of a file
