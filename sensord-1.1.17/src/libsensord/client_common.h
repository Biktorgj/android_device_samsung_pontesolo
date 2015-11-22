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

#ifndef CLIENT_COMMON_H_
#define CLIENT_COMMON_H_

/*header for each sensor type*/
#include <sensor_internal.h>
#include <csensor_handle_info.h>
#include <creg_event_info.h>
#include <common.h>

#define BASE_GATHERING_INTERVAL	100

#define CLIENT_NAME_SIZE NAME_MAX+10

enum log_id {
	LOG_ID_START = 0,
	LOG_ID_SENSOR_TYPE = 0,
	LOG_ID_EVENT,
	LOG_ID_DATA,
	LOG_ID_PROPERTY,
	LOG_ID_END,
};

struct log_attr {
	const char *name;
	unsigned long cnt;
	const unsigned int print_per_cnt;
};

struct log_element {
	log_id id;
	unsigned int type;
	struct log_attr log_attr;
};


typedef struct {
	int handle;
	unsigned int event_type;
	sensor_event_data_t ev_data;
	int sensor_state;
	int sensor_option;
	sensor_type_t sensor;
	creg_event_info event_info;
} log_info;

bool is_one_shot_event(unsigned int event_type);
bool is_ontime_event(unsigned int event_type);
bool is_panning_event(unsigned int event_type);
bool is_single_state_event(unsigned int event_type);
unsigned int get_calibration_event_type(unsigned int event_type);
unsigned long long get_timestamp(void);

const char* get_log_element_name(log_id id, unsigned int type);
const char* get_sensor_name(sensor_id_t sensor_id);
const char* get_event_name(unsigned int event_type);
const char* get_data_name(unsigned int data_id);
void print_event_occurrence_log(csensor_handle_info &sensor_handle_info, const creg_event_info *event_info);

class sensor_info;
sensor_info *sensor_to_sensor_info(sensor_t sensor);
sensor_t sensor_info_to_sensor(const sensor_info *info);

#endif /* CLIENT_COMMON_H_ */
