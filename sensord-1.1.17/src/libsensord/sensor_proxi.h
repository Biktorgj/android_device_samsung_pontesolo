/*
 * libsensord
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
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

#ifndef __SENSOR_PROXI_H__
#define __SENSOR_PROXI_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup SENSOR_PROXY Proximity Sensor
 * @ingroup SENSOR_FRAMEWORK
 * 
 * These APIs are used to control the Proxymaty sensor.
 * @{
 */

enum proxi_data_id {
	PROXIMITY_BASE_DATA_SET 	= (PROXIMITY_SENSOR << 16) | 0x0001,
	PROXIMITY_DISTANCE_DATA_SET = (PROXIMITY_SENSOR << 16) | 0x0002,
};

enum proxi_evet_type {			
	PROXIMITY_EVENT_CHANGE_STATE					= (PROXIMITY_SENSOR << 16) | 0x0001,
	PROXIMITY_EVENT_STATE_REPORT_ON_TIME			= (PROXIMITY_SENSOR << 16) | 0x0002,
	PROXIMITY_EVENT_DISTANCE_DATA_REPORT_ON_TIME	= (PROXIMITY_SENSOR << 16) | 0x0004,
};

enum proxi_change_state {
	PROXIMITY_STATE_FAR		= 0,
	PROXIMITY_STATE_NEAR	= 1,
};

enum proxi_property_id {
	PROXIMITY_PROPERTY_UNKNOWN = 0,
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
//! End of a file
