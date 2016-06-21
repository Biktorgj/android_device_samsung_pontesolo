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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <hardware/sensors.h>
#include <linux/input.h>
#include <math.h>

__BEGIN_DECLS

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/*
 * The screen orientation sensor is not supported by Android.
 * The following value is the same as the one Motorola uses.
 */
#define SENSOR_TYPE_SCREEN_ORIENTATION    (65536)
/*
 * WRIST_TILT_GESTURE is required by Android Wear for WakeUp 
 * gesture, but doesn't exist in Cyanogenmod 12.1
 * public static final String STRING_TYPE_WRIST_TILT_GESTURE = "android.sensor.wrist_tilt_gesture";
 */
#define SENSOR_TYPE_WRIST_TILT_GESTURE                         (26)
#define SENSOR_STRING_TYPE_WRIST_TILT_GESTURE                  "android.sensor.wrist_tilt_gesture"

/*****************************************************************************/

#define ID_A    (0)
#define ID_M    (1)
#define ID_O    (2)
#define ID_L    (3)
#define ID_P    (4)
#define ID_SO   (5)
#define ID_SM   (6)
#define ID_GY   (7)
#define ID_HRM	(8)
#define ID_GES	(9)
#define ID_STC	(10)
#define ID_STD	(11)
#define ID_ROT	(12)
#define ID_PRS (14)
#define ID_TMP (15)
/*****************************************************************************/
/* Seamless Sensor Platform Enable bits. This is sent to the DEVICE_ENABLE node at
sysFS, and the kernel SSP module sends the instruction to enable the sensor to the MCU */
// Check drivers/sensorhub/stm/ssp.h for details on the SENSOR_TYPE enum
#define SSP_ACCEL  (1) 
#define SSP_GYRO   (2) 
#define SSP_MAG    (16)
#define SSP_PRESS  (32)
#define SSP_GESTURE (64)
#define SSP_PROXIMITY (128)
#define SSP_TEMPERATURE (256)
#define SSP_LIGHT  (512)
#define SSP_PROXI_RAW (1024)
#define SSP_ORIENTATION (2048)
#define SSP_STEP_DETECT (4096)
#define SSP_ROTV (65536)
#define SSP_STEP_COUNTER (131072)
#define SSP_HRM (1048576)
#define SSP_TILT (2097152)
#define SSP_UV (4194304)

// SysFS hook
#define SSP_DEVICE_ENABLE   "/sys/class/sensors/ssp_sensor/enable"


/*****************************************************************************/

/*
 * The SENSORS Module
 */

/*****************************************************************************/

#define ACC_MAX_DELAY_NS            200000000 /* nanoseconds, as expected by SSP */

#define EVENT_TYPE_ACCEL_X          REL_X
#define EVENT_TYPE_ACCEL_Y          REL_Y
#define EVENT_TYPE_ACCEL_Z          REL_Z

#define EVENT_TYPE_SO               MSC_RAW

#define EVENT_TYPE_SM               MSC_GESTURE

#define EVENT_TYPE_MAGV_X           ABS_RX
#define EVENT_TYPE_MAGV_Y           ABS_RY
#define EVENT_TYPE_MAGV_Z           ABS_RZ
#define EVENT_TYPE_MAGV_STATUS      ABS_RUDDER

#define EVENT_TYPE_YAW              ABS_HAT0X
#define EVENT_TYPE_PITCH            ABS_HAT0Y
#define EVENT_TYPE_ROLL             ABS_HAT1X
#define EVENT_TYPE_ORIENT_STATUS    ABS_HAT1Y

#define EVENT_TYPE_PROXIMITY        MSC_RAW
#define EVENT_TYPE_LIGHT            ABS_MISC

#define ADJUST_PROX                 0.1f

#define EVENT_TYPE_GYRO_X           REL_RX
#define EVENT_TYPE_GYRO_Y           REL_RY
#define EVENT_TYPE_GYRO_Z           REL_RZ

#define LSG                         (1000.0f)
#define RANGE_A                     (2*GRAVITY_EARTH)
#define RESOLUTION_A                (GRAVITY_EARTH / LSG)

/* conversion of acceleration data to SI units (m/s^2) */
#define CONVERT_A                   (GRAVITY_EARTH / LSG)
#define CONVERT_A_X                 (CONVERT_A)
#define CONVERT_A_Y                 (CONVERT_A)
#define CONVERT_A_Z                 (CONVERT_A)

/* conversion of acceleration data for AKM accel interface */
#define CONVERT_ACC                 (72.0f)
#define CONVERT_ACC_X               (CONVERT_ACC)
#define CONVERT_ACC_Y               (CONVERT_ACC)
#define CONVERT_ACC_Z               (CONVERT_ACC)

/* conversion of orientation data to degree units */
#define CONVERT_O                   (1.0f/64.0f)
#define CONVERT_O_A                 (CONVERT_O)
#define CONVERT_O_P                 (CONVERT_O)
#define CONVERT_O_R                 (CONVERT_O)

/* conversion of magnetic data to uT units */
#define CONVERT_M                   (0.06f)
#define CONVERT_M_X                 (CONVERT_M)
#define CONVERT_M_Y                 (CONVERT_M)
#define CONVERT_M_Z                 (CONVERT_M)

/* conversion of gyro data to SI units (radian/s) */
#define CONVERT_G                   ((70.0f / -1000.0f) * ((float)M_PI / 180.0f))
#define CONVERT_G_X                 (CONVERT_G)
#define CONVERT_G_Y                 (CONVERT_G)
#define CONVERT_G_Z                 (CONVERT_G)

/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
