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

#ifndef __SENSOR_INTERNAL_H__
#define __SENSOR_INTERNAL_H__

#ifndef DEPRECATED
#define DEPRECATED __attribute__((deprecated))
#endif

#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>

#include <sensor_internal_deprecated.h>

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
#include <sensor_dust.h>

typedef void (*sensor_cb_t)(sensor_t sensor, unsigned int event_type, sensor_data_t *data, void *user_data);
typedef void (*sensorhub_cb_t)(sensor_t sensor, unsigned int event_type, sensorhub_data_t *data, void *user_data);
typedef void (*sensor_accuracy_changed_cb_t) (sensor_t sensor, unsigned long long timestamp, int accuracy, void *user_data);

/**
 * @brief Get the list of available sensors of a certain type,  use ALL_SENSOR to get all the sensors.
 *
 * @param[in] type the type of sensors requested.
 * @param[out] list the list of sensors matching the asked type,  the caller should explicitly free this list.
 * @param[out] sensor count the count of sensors contained in the list.
 * @return true on success, otherwise false.
 */
bool sensord_get_sensor_list(sensor_type_t type, sensor_t **list, int *sensor_count);

/**
 * @brief Get the default sensor for a given type.
 *
 * @param[in] type the type of a sensor requested.
 * @return the default sensor matching the asked type on success, otherwise NULL.
 */
sensor_t sensord_get_sensor(sensor_type_t type);

/**
 * @brief Get the type of this sensor.
 *
 * @param[in] sensor a sensor to get type.
 * @param[out] type the type of this sensor.
 * @return return true on success, otherwise false.
 */
bool sensord_get_type(sensor_t sensor, sensor_type_t *type);

/**
 * @brief Get the name string of this sensor.
 *
 * @param[in] sensor a sensor to get name.
 * @return the name string of this sensor on success, otherwise NULL.
 */
const char* sensord_get_name(sensor_t sensor);

/**
 * @brief Get the vendor string of this sensor.
 *
 * @param[in] sensor a sensor to get vendor.
 * @return the vendor string of this sensor on success, otherwise NULL.
 */
const char* sensord_get_vendor(sensor_t sensor);

/**
 * @brief Get the privilege of this sensor.
 *
 * @param[in] sensor a sensor to get privilege.
 * @param[out] privilege the privilege of this sensor.
 * @return true on success, otherwise false.
 */
bool sensord_get_privilege(sensor_t sensor, sensor_privilege_t *privilege);

/**
 * @brief Get the minimum range of this sensor in the sensor's unit.
 *
 * @param[in] sensor a sensor to get minimum range.
 * @param[out] min_range the minimum range of this sensor in the sensor's unit.
 * @return true on success, otherwise false.
 */
bool sensord_get_min_range(sensor_t sensor, float *min_range);

/**
 * @brief Get the maximum range of this sensor in the sensor's unit.
 *
 * @param[in] sensor a sensor to get maximum range.
 * @param[out] max_range the maximum range of this sensor in the sensor's unit.
 * @return true on success, otherwise false.
 */
bool sensord_get_max_range(sensor_t sensor, float *max_range);

/**
 * @brief Get the resolution of this sensor in the sensor's unit.
 *
 * @param[in] sensor a sensor to get resolution.
 * @param[out] resolution the resolution of this sensor in the sensor's unit.
 * @return true on success, otherwise false.
 */
bool sensord_get_resolution(sensor_t sensor, float *resolution);

/**
 * @brief Get the minimum interval allowed between two events in microsecond or zero if this sensor only returns a value when the data it's measuring changes.
 *
 * @param[in] sensor a sensor to get minimum interval.
 * @param[out] min_interval the minimum interval of this sensor.
 * @return true on success, otherwise false.
 */
bool sensord_get_min_interval(sensor_t sensor, int *min_interval);

/**
 * @brief Get the number of events reserved for this sensor in the batch mode FIFO.
 *
 * @param[in] sensor a sensor to get the number of fifo count
 * @param[out] fifo_count the number of events reserved for this sensor in the batch mode FIFO
 * @return true on success, otherwise false
 */
bool sensord_get_fifo_count(sensor_t sensor, int *fifo_count);

/**
 * @brief Get the maximum number of events of this sensor that could be batched. If this value is zero it indicates that batch mode is not supported for this sensor.
 *
 * @param[in] sensor a sensor to the maximum number of events that could be batched.
 * @param[out] max_batch_count the maximum number of events of this sensor that could be batched.
 * @return true on success, otherwise false.
 */
bool sensord_get_max_batch_count(sensor_t sensor, int *max_batch_count);


/**
 * @brief Get the supported event types of this sensor.
 *
 * @param[in] sensor a sensor to get the supported event types.
 * @param[out] event_types the array containing supported event types of this sensor, the caller should explicitly free this array.
 * @param[out] count the count of the supported event types of this sensor.
 * @return true on success, otherwise false.
 */
bool sensord_get_supported_event_types(sensor_t sensor, unsigned int **event_types, int *count);


/**
 * @brief Check a given event type is supporeted by this sensor.
 *
 * @param[in] sensor a sensor to check a given event type is supporeted.
 * @param[out] event_type an event type to be checked whether supported or not.
 * @param[out] supported whether a given event is supported or not in this sensor.
 * @return true on success, otherwise false.
 */
bool sensord_is_supported_event_type(sensor_t sensor, unsigned int event_type, bool *supported);

/**
 * @brief Connect a given sensor and get a handle of a given sensor.
 *
 * @param[in] sensor a sensor to connect
 * @return a handle of a given sensor on success, otherwise negative value
 */
int sensord_connect(sensor_t sensor);

/**
 * @brief Disconnect a given sensor.
 *
 * @param[in] handle a handle to disconnect.
 * @return true on success, otherwise false.
 */
bool sensord_disconnect(int handle);

/**
 * @brief Register a callback with a connected sensor for a given event_type. This callback will be called when a given event occurs in a connected sensor.
 *
 * @param[in] handle a handle represensting a connected sensor.
 * @param[in] event_type an event type  to register
 * @param[in] interval The desired interval between two consecutive events in microseconds. This is only a hint to the system so events may be received faster or slower than the specified interval.
 * 				     It can be one of SENSOR_INTERVAL_NORMAL,  SENSOR_INTERVAL_FASTEST or the interval in microseconds.
 * @param[in] max_batch_latency An event in the batch can be delayed by at most max_batch_latency microseconds. If this is set to zero, batch mode is disabled.
 * @param[in] cb a callback which is called when a given event occurs
 * @param[in] user_data the callback is called with user_data
 * @return true on success, otherwise false.
 */
bool sensord_register_event(int handle, unsigned int event_type, unsigned int interval, unsigned int max_batch_latency, sensor_cb_t cb, void *user_data);

/**
 * @brief Register a callback with a connected context sensor for a given event_type. This callback will be called when a given event occurs in a connected context sensor.
 *
 * @param[in] handle a handle represensting a connected context sensor.
 * @param[in] event_type an event type to register
 * @param[in] interval The desired interval between two consecutive events in microseconds. This is only a hint to the system so events may be received faster or slower than the specified interval.
 * 				      It can be one of SENSOR_INTERVAL_NORMAL,  SENSOR_INTERVAL_FASTEST or the interval in microseconds.
 * @param[in] max_batch_latency An event in the batch can be delayed by at most max_batch_latency microseconds. If this is set to zero, batch mode is disabled.
 * @param[in] cb a callback which is called when a given event occurs
 * @param[in] user_data the callback is called with user_data
 * @return true on success, otherwise false.
 */
bool sensord_register_hub_event(int handle, unsigned int event_type, unsigned int interval, unsigned int max_batch_latency, sensorhub_cb_t cb, void *user_data);

/**
 * @brief Unregister a event with a connected sensor.  After unregistering, that event will not be sent.
 *
 * @param[in] handle a handle represensting a connected sensor.
 * @param[in] event_type an event type to unregister.
 * @return true on success, otherwise false.
 */
bool sensord_unregister_event(int handle, unsigned int event_type);

/**
 * @brief Register a callback with a connected sensor. This callback will be called when the accuracy of a sensor has changed.
 *
 * @param[in] handle a handle represensting a connected sensor.
 * @param[in] cb a callback which is called when he accuracy of a sensor has changed.
 * @param[in] user_data the callback is called with user_data
 * @return true on success, otherwise false.
 */
bool sensord_register_accuracy_cb(int handle, sensor_accuracy_changed_cb_t cb, void *user_data);

/**
 * @brief Unregister a callback with a connected sensor.  After unregistering,  sensor_accuray_change_cb will not be called.
 *
 * @param[in] handle a handle represensting a connected sensor.
 * @return true on success, otherwise false.
 */
bool sensord_unregister_accuracy_cb(int handle);

/**
 * @brief Start listening events with a connected sensor.
 *
 * @param[in] handle a handle represensting a connected sensor.
 * @param[in] option either one of SENSOR_OPTION_DEFAULT and  SENSOR_OPTION_ALWAYS_ON.
 *				   with SENSOR_OPTION_DEFAULT, it stops to listening events when LCD is off or in power save mode.
 *				   with SENSOR_OPTION_ALWAYS_ON, it continues to listening events even when LCD is off or in power save mode.
 * @return true on success, otherwise false.
 */

bool sensord_start(int handle, int option);

/**
 * @brief Stop listening events with a connected sensor.
 *
 * @param[in] handle a handle represensting a connected sensor.
 * @return true on success, otherwise false.
 */
bool sensord_stop(int handle);

/**
 * @brief Change the interval of a specifed event type in a connected sensor.
 *
 * @param[in] handle a handle represensting a connected sensor.
 * @param[in] event_type an event type to change interval.
 * @param[in] interval The desired interval between two consecutive events in microseconds. This is only a hint to the system so events may be received faster or slower than the specified interval.
 * 				      It can be one of SENSOR_INTERVAL_NORMAL,  SENSOR_INTERVAL_FASTEST or the interval in microseconds.
 * @return true on success, otherwise false.
 */
bool sensord_change_event_interval(int handle, unsigned int event_type, unsigned int interval);

/**
 * @brief Change the max batch latency of a specifed event type in a connected sensor.
 *
 * @param[in] handle a handle represensting a connected sensor.
 * @param[in] max_batch_latency an event in the batch can be delayed by at most max_batch_latency microseconds. If this is set to zero, batch mode is disabled.
 * @return true on success, otherwise false.
 */
bool sensord_change_event_max_batch_latency(int handle, unsigned int max_batch_latency);

/**
 * @brief Change the option of a connected sensor.
 *
 * @param[in] handle a handle represensting a connected sensor.
 * @param[in] option either one of SENSOR_OPTION_DEFAULT and  SENSOR_OPTION_ALWAYS_ON.
 *				   with SENSOR_OPTION_DEFAULT, it stops to listening events when LCD is off or in power save mode.
 * 				   with SENSOR_OPTION_ALWAYS_ON, it continues to listening events even when LCD is off or in power save mode.
 * @return true on success, otherwise false.
 */
bool sensord_set_option(int handle, int option);

/**
 * @brief Send data to sensorhub
 *
 * @param[in] handle a handle represensting a connected context sensor.
 * @param[in] data it holds data to send to sensorhub
 * @param[in] data_len the length of data
 * @return true on success, otherwise false.
 */
bool sensord_send_sensorhub_data(int handle, const char *data, int data_len);

/**
 * @brief get sensor data from a connected sensor
 *
 * @param[in] handle a handle represensting a connected context sensor.
 * @param[in] data_id it specifies data to get
 * @param[out] sensor_data data from connected sensor
 * @return true on success, otherwise false.
 */
bool sensord_get_data(int handle, unsigned int data_id, sensor_data_t* sensor_data);

/**
  * @}
 */

#ifdef __cplusplus
}
#endif


#endif
