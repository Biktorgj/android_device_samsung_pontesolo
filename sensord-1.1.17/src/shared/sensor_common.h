/*
 * libsensord-share
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

#ifndef __SENSOR_COMMON_H__
#define __SENSOR_COMMON_H__

#ifndef DEPRECATED
#define DEPRECATED __attribute__((deprecated))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup SENSOR_FRAMEWORK SensorFW
 * To support the unified API for the various sensors
 */

/**
 * @defgroup SENSOR_FRAMEWORK_COMMON Sensor Framework Common API
 * @ingroup SENSOR_FRAMEWORK
 *
 * These APIs are used to control the sensors.
 * @{
 */

typedef enum {
	ALL_SENSOR = -1,
	UNKNOWN_SENSOR = 0,
	ACCELEROMETER_SENSOR,
	GEOMAGNETIC_SENSOR,
	LIGHT_SENSOR,
	PROXIMITY_SENSOR,
	THERMOMETER_SENSOR,
	GYROSCOPE_SENSOR,
	PRESSURE_SENSOR,
	MOTION_SENSOR,
	FUSION_SENSOR,
	PEDOMETER_SENSOR,
	CONTEXT_SENSOR,
	FLAT_SENSOR,
	BIO_SENSOR,
	BIO_HRM_SENSOR,
	AUTO_ROTATION_SENSOR,
	GRAVITY_SENSOR,
	LINEAR_ACCEL_SENSOR,
	ROTATION_VECTOR_SENSOR,
	ORIENTATION_SENSOR,
	PIR_SENSOR,
	PIR_LONG_SENSOR,
	TEMPERATURE_SENSOR,
	HUMIDITY_SENSOR,
	ULTRAVIOLET_SENSOR,
	DUST_SENSOR,
	RV_RAW_SENSOR,
	UNCAL_GYROSCOPE_SENSOR,
	UNCAL_GEOMAGNETIC_SENSOR
} sensor_type_t;

typedef unsigned int sensor_id_t;

typedef void *sensor_t;

typedef enum {
	SENSOR_PRIVILEGE_PUBLIC,
	SENSOR_PRIVILEGE_INTERNAL,
} sensor_privilege_t;


#define SENSOR_DATA_VALUE_SIZE 16

/*
 *	When modifying it, check copy_sensor_data()
 */
typedef struct sensor_data_t {
/*
 * 	Use "accuracy" instead of "data_accuracy"
 * 	which is going to be removed soon
 */
	union {
		int accuracy;
		int data_accuracy; //deprecated
	};

	unsigned long long timestamp;

/*
 * 	Use "value_count" instead of "values_num"
 * 	which is going to be removed soon
 */
	union {
		int value_count;
		int values_num; //deprecated
	};

	float values[SENSOR_DATA_VALUE_SIZE];
} sensor_data_t;

#define SENSOR_HUB_DATA_SIZE	4096

typedef struct sensorhub_data_t {
    int version;
    int sensorhub;
    int type;
    int hub_data_size;
    unsigned long long timestamp;
    char hub_data[SENSOR_HUB_DATA_SIZE];
    float data[16];
} sensorhub_data_t;

enum sensor_accuracy_t {
	SENSOR_ACCURACY_UNDEFINED = -1,
	SENSOR_ACCURACY_BAD = 0,
	SENSOR_ACCURACY_NORMAL =1,
	SENSOR_ACCURACY_GOOD = 2,
	SENSOR_ACCURACY_VERYGOOD = 3
};

/*
 *	To prevent naming confliction as using same enums as sensor CAPI use
 */
#ifndef __SENSORS_H__
enum sensor_option_t {
	SENSOR_OPTION_DEFAULT = 0,
	SENSOR_OPTION_ON_IN_SCREEN_OFF = 1,
	SENSOR_OPTION_ON_IN_POWERSAVE_MODE = 2,
	SENSOR_OPTION_ALWAYS_ON = SENSOR_OPTION_ON_IN_SCREEN_OFF | SENSOR_OPTION_ON_IN_POWERSAVE_MODE,
	SENSOR_OPTION_END
};

typedef enum sensor_option_t sensor_option_e;
#endif

enum sensor_interval_t {
	SENSOR_INTERVAL_FASTEST = 0,
	SENSOR_INTERVAL_NORMAL = 200,
};


typedef enum {
	CONDITION_NO_OP,
	CONDITION_EQUAL,
	CONDITION_GREAT_THAN,
	CONDITION_LESS_THAN,
} condition_op_t;

#ifdef __cplusplus
}
#endif


#endif
//! End of a file
