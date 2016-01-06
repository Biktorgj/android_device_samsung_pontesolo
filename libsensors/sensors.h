/*
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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

__BEGIN_DECLS

/*****************************************************************************/

/*
	 SENSOR_TYPES (look at drivers/sensorhub/stm/ssp.h in kernel source)
	Values are X << 16 | 0x001, value must always be >0, so sensor 0 is 1
1			ACCELEROMETER_SENSOR = 0,
2			GYROSCOPE_SENSOR,
4			GEOMAGNETIC_UNCALIB_SENSOR,
8			GEOMAGNETIC_RAW,
16			GEOMAGNETIC_SENSOR,
32			PRESSURE_SENSOR,
64			GESTURE_SENSOR,
128			PROXIMITY_SENSOR,
256			TEMPERATURE_HUMIDITY_SENSOR,
512			LIGHT_SENSOR,
1024		PROXIMITY_RAW,
2048		ORIENTATION_SENSOR,
4096		STEP_DETECTOR = 12,
8192		SIG_MOTION_SENSOR,
16384		GYRO_UNCALIB_SENSOR,
32768		GAME_ROTATION_VECTOR = 15,
65536		ROTATION_VECTOR,
131072		STEP_COUNTER,
262144		BIO_HRM_RAW,
524288		BIO_HRM_RAW_FAC,
1048576		BIO_HRM_LIB,
2097152		TILT_MOTION,
4194304		UV_SENSOR,
 */


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ID_A  (0) // accel
#define ID_M  (1) // magnetic
#define ID_O  (2) // orientation
#define ID_L  (3) // light
#define ID_GY (5) // gyro
#define ID_PR (6) // pressure
/* .... */
#define ID_T  (7) // Temperature from pressure sensor
#define ID_UV (8) // UV
#define ID_HRM (9) // Heart rate monitor

#define SSP_ACCEL  (1)
#define SSP_GYRO   (2)
#define SSP_MAG    (16)
#define SSP_PRESS  (32)
#define SSP_LIGHT  (512)
/* ....*/
#define SSP_UV (4194304)
#define SSP_HRM (1048576)
 
#define SSP_DEVICE_ENABLE   "/sys/class/sensors/ssp_sensor/enable"

const int ssp_sensors[] = {
  SSP_ACCEL,
  SSP_GYRO,
  SSP_MAG,
  SSP_PRESS,
  SSP_LIGHT,
  SSP_UV,
  SSP_HRM
};

/*****************************************************************************/

/*
 * The SENSORS Module
 */

 /*** ALL THIS HAS TO GO */

/* the CM3663 is a binary proximity sensor that triggers around 6 cm on
 * this hardware */
#define PROXIMITY_THRESHOLD_CM  8.0f

/*****************************************************************************/

#define EVENT_TYPE_ACCEL_X          REL_X  //1
#define EVENT_TYPE_ACCEL_Y          REL_Y  //0
#define EVENT_TYPE_ACCEL_Z          REL_Z  //2

#define EVENT_TYPE_YAW              ABS_RX  //3
#define EVENT_TYPE_PITCH            ABS_RY  //4
#define EVENT_TYPE_ROLL             ABS_RZ  //5
#define EVENT_TYPE_ORIENT_STATUS    ABS_WHEEL //8

#define EVENT_TYPE_MAGV_X           REL_RX  // 3
#define EVENT_TYPE_MAGV_Y           REL_RY  // 4
#define EVENT_TYPE_MAGV_Z           REL_RZ  // 5

#define EVENT_TYPE_TEMPERATURE      ABS_THROTTLE
#define EVENT_TYPE_STEP_COUNT       ABS_GAS
#define EVENT_TYPE_PROXIMITY        0x0019
#define EVENT_TYPE_LIGHT            ABS_MISC //REL_MISC

#define EVENT_TYPE_GYRO_X           REL_RX
#define EVENT_TYPE_GYRO_Y           REL_RY
#define EVENT_TYPE_GYRO_Z           REL_RZ

#define EVENT_TYPE_PRESSURE         REL_HWHEEL

#define LSG                         (1000.0f)

// Proximity values
#define PROXIMITY_NODE_STATE_NEAR (0)
#define PROXIMITY_NODE_STATE_FAR (1)
#define PROXIMITY_NODE_STATE_UNKNOWN (2)

// conversion of acceleration data to SI units (m/s^2)
#define RANGE_A                     (2*GRAVITY_EARTH)
#define RESOLUTION_A                (GRAVITY_EARTH / LSG)
#define CONVERT_A                   (GRAVITY_EARTH / LSG)
#define CONVERT_A_X                 (CONVERT_A)
#define CONVERT_A_Y                 (CONVERT_A)
#define CONVERT_A_Z                 (CONVERT_A)

// conversion of magnetic data to uT units
#define CONVERT_M                   (1.0f/16.0f)
#define CONVERT_M_X                 (CONVERT_M)
#define CONVERT_M_Y                 (CONVERT_M)
#define CONVERT_M_Z                 (CONVERT_M)

/* conversion of orientation data to degree units */
#define CONVERT_O                   (1.0f/1000.0f)
#define CONVERT_O_A                 (CONVERT_O)
#define CONVERT_O_P                 (CONVERT_O)
#define CONVERT_O_R                 (CONVERT_O)

// conversion of gyro data to SI units (radian/sec)
#define RANGE_GYRO                  (2000.0f*(float)M_PI/180.0f)
#define CONVERT_GYRO                ((70.0f / 4000.0f) * ((float)M_PI / 180.0f))
#define CONVERT_GYRO_X              (CONVERT_GYRO)
#define CONVERT_GYRO_Y              (CONVERT_GYRO)
#define CONVERT_GYRO_Z              (CONVERT_GYRO)

#define SENSOR_STATE_MASK           (0x7FFF)

/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
