/*
 * libsensord-share
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

#if !defined(_SF_COMMON_H_)
#define _SF_COMMON_H_
#include <unistd.h>
#include <cpacket.h>
#include <vector>
#include <sensor_common.h>
#include <string>
#include <vector>

#define COMMAND_CHANNEL_PATH			"/tmp/sf_command_socket"
#define EVENT_CHANNEL_PATH				"/tmp/sf_event_socket"

#define MAX_HANDLE			64
#define MAX_HANDLE_REACHED	-2

#define CLIENT_ID_INVALID   -1

enum packet_type_t {
	CMD_NONE = 0,
	CMD_GET_ID,
	CMD_GET_SENSOR_LIST,
	CMD_HELLO,
	CMD_BYEBYE,
	CMD_DONE,
	CMD_START,
	CMD_STOP,
	CMD_REG,
	CMD_UNREG,
	CMD_SET_OPTION,
	CMD_SET_INTERVAL,
	CMD_UNSET_INTERVAL,
	CMD_SET_COMMAND,
	CMD_GET_DATA,
	CMD_SEND_SENSORHUB_DATA,
	CMD_CNT,
};

enum sensor_state_t {
	SENSOR_STATE_UNKNOWN = -1,
	SENSOR_STATE_STOPPED = 0,
	SENSOR_STATE_STARTED = 1,
	SENSOR_STATE_PAUSED = 2
};

enum poll_interval_t {
	POLL_100HZ_MS	= 10,
	POLL_50HZ_MS	= 20,
	POLL_25HZ_MS	= 40,
	POLL_20HZ_MS	= 50,
	POLL_10HZ_MS	= 100,
	POLL_5HZ_MS		= 200,
	POLL_1HZ_MS		= 1000,
	POLL_MAX_HZ_MS  = POLL_1HZ_MS,
};

typedef struct {
	pid_t pid;
} cmd_get_id_t;

typedef struct {
} cmd_get_sensor_list_t;

typedef struct {
	int client_id;
	int sensor;
} cmd_hello_t;

typedef struct {
} cmd_byebye_t;


typedef struct {
	unsigned int type;
} cmd_get_data_t;

typedef struct {
	long value;
} cmd_done_t;


typedef struct {
	int client_id;
} cmd_get_id_done_t;

typedef struct {
	int sensor_cnt;
	char data[0];
} cmd_get_sensor_list_done_t;

typedef struct {
	int state;
	sensor_data_t base_data;
} cmd_get_data_done_t;

typedef struct {
} cmd_start_t;

typedef struct {
} cmd_stop_t;

typedef struct {
	unsigned int event_type;
} cmd_reg_t;

typedef struct {
	unsigned int event_type;
} cmd_unreg_t;

typedef struct {
	unsigned int interval;
} cmd_set_interval_t;

typedef struct {
} cmd_unset_interval_t;

typedef struct {
	int option;
} cmd_set_option_t;

typedef struct  {
	unsigned int cmd;
	long value;
} cmd_set_command_t;

typedef struct  {
	int data_len;
	char data[0];
} cmd_send_sensorhub_data_t;

#define EVENT_CHANNEL_MAGIC 0xCAFECAFE

typedef struct {
	unsigned int magic;
	int client_id;
} event_channel_ready_t;


typedef struct {
	std::string name;
	std::string vendor;
	float min_range;
	float max_range;
	float resolution;
	int min_interval;
	int fifo_count;
	int max_batch_count;
} sensor_properties_t;


/*
 * When modifying it, check copy_sensor*_data()
 */
typedef struct sensor_event_t {
	unsigned int event_type;
	sensor_id_t sensor_id;
	sensor_data_t data;
} sensor_event_t;


typedef struct sensorhub_event_t {
	unsigned int event_type;
	sensor_id_t sensor_id;
	sensorhub_data_t data;
} sensorhub_event_t;

typedef struct sensor_module{
	std::vector<void*> sensors;
} sensor_module;

typedef sensor_module* (*create_t)(void);

typedef void *(*cmd_func_t)(void *data, void *cb_data);

typedef std::vector<unsigned int> event_type_vector;

enum sensorhub_enable_bit {
	SENSORHUB_ACCELEROMETER_ENABLE_BIT = 0,
	SENSORHUB_GYROSCOPE_ENABLE_BIT,
	SENSORHUB_GEOMAGNETIC_UNCALIB_ENABLE_BIT,
	SENSORHUB_GEOMAGNETIC_RAW_ENABLE_BIT,
	SENSORHUB_GEOMAGNETIC_ENABLE_BIT,
	SENSORHUB_PRESSURE_ENABLE_BIT,
	SENSORHUB_GESTURE_ENABLE_BIT,
	SENSORHUB_PROXIMITY_ENABLE_BIT,
	SENSORHUB_TEMPERATURE_HUMIDITY_ENABLE_BIT,
	SENSORHUB_LIGHT_ENABLE_BIT,
	SENSORHUB_PROXIMITY_RAW_ENABLE_BIT,
	SENSORHUB_ORIENTATION_ENABLE_BIT,
	SENSORHUB_STEP_DETECTOR_ENABLE_BIT = 12,
	SENSORHUB_SIG_MOTION_ENABLE_BIT,
	SENSORHUB_GYRO_UNCALIB_ENABLE_BIT,
	SENSORHUB_GAME_ROTATION_VECTOR_ENABLE_BIT = 15,
	SENSORHUB_ROTATION_VECTOR_ENABLE_BIT,
	SENSORHUB_STEP_COUNTER_ENABLE_BIT,
	SENSORHUB_BIO_HRM_RAW_ENABLE_BIT,
	SENSORHUB_BIO_HRM_RAW_FAC_ENABLE_BIT,
	SENSORHUB_BIO_HRM_LIB_ENABLE_BIT,
	SENSORHUB_TILT_MOTION,
	SENSORHUB_UV_SENSOR,
	SENSORHUB_PIR_ENABLE_BIT,
	SENSORHUB_ENABLE_BIT_MAX,
};

enum sensor_permission_t {
	SENSOR_PERMISSION_NONE	= 0,
	SENSOR_PERMISSION_STANDARD = (1 << 0),
	SENSOR_PERMISSION_BIO	=  (1 << 1),
};

#define BIO_SENSOR_PRIVELEGE_NAME "sensord::bio"
#define BIO_SENSOR_ACCESS_RIGHT "rw"

#endif
