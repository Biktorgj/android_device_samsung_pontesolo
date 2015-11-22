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

#ifndef __SENSOR_INTERNAL_DEPRECATED__
#define __SENSOR_INTERNAL_DEPRECATED__

#ifndef DEPRECATED
#define DEPRECATED __attribute__((deprecated))
#endif

#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>

/*header for common sensor type*/
#include <sensor_common.h>

/*header for each sensor type*/
#include <sensor_accel.h>
#include <sensor_geomag.h>
#include <sensor_uncal_geomag.h>
#include <sensor_light.h>
#include <sensor_proxi.h>
#include <sensor_motion.h>
#include <sensor_gyro.h>
#include <sensor_uncal_gyro.h>
#include <sensor_pressure.h>
#include <sensor_pedo.h>
#include <sensor_context.h>
#include <sensor_flat.h>
#include <sensor_bio.h>
#include <sensor_bio_hrm.h>
#include <sensor_auto_rotation.h>
#include <sensor_gravity.h>
#include <sensor_linear_accel.h>
#include <sensor_orientation.h>
#include <sensor_rv.h>
#include <sensor_rv_raw.h>
#include <sensor_pir.h>
#include <sensor_pir_long.h>
#include <sensor_temperature.h>
#include <sensor_humidity.h>
#include <sensor_ultraviolet.h>

typedef struct {
	condition_op_t cond_op;
	float cond_value1;
} event_condition_t;

typedef struct {
	size_t event_data_size;
	void *event_data;
} sensor_event_data_t;

typedef void (*sensor_callback_func_t)(unsigned int, sensor_event_data_t *, void *);
typedef sensor_callback_func_t sensor_legacy_cb_t;

typedef struct {
	int x;
	int y;
	int z;
} sensor_panning_data_t;

/**
 * @fn int sf_connect(sensor_type_t sensor)
 * @brief  This API connects a sensor type to respective sensor. The application calls with the type of the sensor (ex. ACCELEROMETER_SENSOR) and on basis of that server takes decision of which plug-in to be connected. Once sensor connected application can proceed for data processing. This API returns a positive handle which should be used by application to communicate on sensor type.
 * @param[in] sensor_type your desired sensor type
 * @return if it succeed, it return handle value( >=0 ) , otherwise negative value return
 */
int sf_connect(sensor_type_t sensor_type);

/**
 * @fn int sf_disconnect(int handle)
 * @brief This API disconnects an attached sensor from an application. Application must use the handle retuned after attaching the sensor. After detaching, the corresponding handle will be released.
 * @param[in] handle received handle value by sf_connect()
 * @return if it succeed, it return zero value , otherwise negative value return
 */
int sf_disconnect(int handle);

/**
 * @fn int sf_start(int handle , int option)
 * @brief This API sends a start command to sensor server. This intimates server that the client side is ready to handle data and start processing. The parameter option should be '0' for current usages.
 * @param[in] handle received handle value by sf_connect()
 * @param[in] option With SENSOR_OPTION_DEFAULT, it stops to sense when LCD is off, and with SENSOR_OPTION_ALWAYS_ON, it continues to sense even when LCD is off
 * @return if it succeed, it return zero value , otherwise negative value return
 */
int sf_start(int handle , int option);

/**
 * @fn int sf_stop(int handle)
 * @brief This API sends a stop command to the Sensor server indicating that the data processing is stopped from application side for this time.
 * @param[in] handle received handle value by sf_connect()
 * @return if it succeed, it return zero value , otherwise negative value return
 */
int sf_stop(int handle);

/**
 * @fn int sf_register_event(int handle , unsigned int event_type , event_conditon_t *event_condition , sensor_callback_func_t cb , void *user_data )
 * @brief This API registers a user defined callback function with a connected sensor for a particular event. This callback function will be called when there is a change in the state of respective sensor. user_data will be the parameter used during the callback call. Callback interval can be adjusted using even_contion_t argument.
 * @param[in] handle received handle value by sf_connect()
 * @param[in] event_type your desired event_type to register it
 * @param[in] event_condition input event_condition for special event. if you want to register without event_condition, just use a NULL value
 * @param[in] cb your define callback function
 * @param[in] user_data	your option data that will be send when your define callback function called. if you don't have any option data, just use a NULL value
 * @return if it succeed, it return zero value , otherwise negative value return
 */
int sf_register_event(int handle , unsigned int event_type , event_condition_t *event_condition , sensor_callback_func_t cb , void *user_data );

/**
 * @fn int sf_unregister_event(int handle, unsigned int event_type)
 * @brief This API de-registers a user defined callback function with a sensor registered with the specified handle. After unsubscribe, no event will be sent to the application.
 * @param[in] handle received handle value by sf_connect()
 * @param[in] event_type your desired event_type that you want to unregister event
 * @return if it succeed, it return zero value , otherwise negative value return
 */
int sf_unregister_event(int handle, unsigned int event_type);

/**
 * @fn int sf_get_data(int handle , unsigned int data_id , sensor_data_t* values)
 * @brief This API gets raw data from a sensor with connecting the sensor-server. The type of sensor is supplied and return data is stored in the output parameter values [].
 * @param[in] handle received handle value by sf_connect()
 * @param[in] data_id predefined data_ID as every sensor in own header - sensor_xxx.h , enum xxx_data_id {}
 * @param[out] values return values
 * @return if it succeed, it return zero value , otherwise negative value return
 */
int sf_get_data(int handle , unsigned int data_id , sensor_data_t* values);

/**
 * @fn int sf_change_event_condition(int handle, unsigned int event_type, event_condition_t *event_condition)
 * @brief This API change a user defined callback function condition with a sensor registered with the specified handle.
 * @param[in] handle received handle value by sf_connect()
 * @param[in] event_type your desired event_type that you want to unregister event
 * @param[in] event_condition your desired event condition that you want to change event
 * @return if it succeed, it return zero value , otherwise negative value return
 */
int sf_change_event_condition(int handle, unsigned int event_type, event_condition_t *event_condition);

/**
 * @fn int sf_change_sensor_option(int handle, int option)
 * @brief This API change sensor option .
 * @param[in] handle received handle value by sf_connect()
 * @param[in] option your desired option that you want to turn on sensor during LCD OFF
 * @return if it succeed, it return zero value , otherwise negative value return
 */

int sf_change_sensor_option(int handle, int option);

/**
 * @fn int sf_send_sensorhub_data(int handle, const char* buffer, int data_len)
 * @brief This API sends data to sensorhub.
 * @param[in] handle received handle by sf_connect()
 * @param[in] data it holds data to send to sensorhub
 * @param[in] data_len the length of data
 * @return if it succeed, it returns zero, otherwise negative value
 */
int sf_send_sensorhub_data(int handle, const char* data, int data_len);


#ifdef __cplusplus
}
#endif


#endif
