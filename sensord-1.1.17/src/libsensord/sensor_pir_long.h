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

#ifndef __SENSOR_PIR_LONG_H__
#define __SENSOR_PIR_LONG_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup SENSOR_PIR_LONG PIR Long Distance Sensor
 * @ingroup SENSOR_FRAMEWORK
 *
 * These APIs are used to control the PIR Long Distance sensor.
 * @{
 */

enum pir_long_data_id {
	PIR_LONG_BASE_DATA_SET 	= (PIR_LONG_SENSOR << 16) | 0x0001,
};

enum pir_long_evet_type {
	PIR_LONG_EVENT_CHANGE_STATE					= (PIR_LONG_SENSOR << 16) | 0x0001,
};

enum pir_long_change_state {
	PIR_LONG_STATE_ABSENT	= 0,
	PIR_LONG_STATE_PRESENT	= 1,
};

enum pir_long_property_id {
	PIR_LONG_PROPERTY_UNKNOWN = 0,
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
//! End of a file
