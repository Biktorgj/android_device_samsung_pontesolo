/*
 * bio_data_reader_standard
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

#include <bio_data_reader_standard.h>
#include <common.h>
#include <sensor_common.h>
#include <linux/input.h>

#define INPUT_MAX_BEFORE_SYN	20
#define INPUT_EVENT_BIAS	1
#define INPUT_VALUE_COUNT	10

bio_data_reader_standard::bio_data_reader_standard()
{

}

bio_data_reader_standard::~bio_data_reader_standard()
{

}


bool bio_data_reader_standard::get_data(int handle, sensor_data_t &data)
{
	bool syn = false;
	int read_input_cnt = 0;
	int index = 0;
	struct input_event bio_input;

	while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
		int len = read(handle, &bio_input, sizeof(bio_input));
		if (len != sizeof(bio_input)) {
			ERR("bio_file read fail, read_len = %d", len);
			return false;
		}

		++read_input_cnt;

		if (bio_input.type == EV_REL) {
			index = bio_input.code - REL_X;

			/* Check an avaiable value REL_X(0x00) ~ REL_MISC(0x09) */
			if (index >= INPUT_VALUE_COUNT) {
				ERR("bio_input event[type = %d, code = %d] is unknown.", bio_input.type, index);
				return false;
			}
			data.values[index] = (unsigned int)bio_input.value - INPUT_EVENT_BIAS;

		} else if (bio_input.type == EV_SYN) {
			syn = true;
			data.timestamp = get_timestamp(&bio_input.time);
			data.value_count = INPUT_VALUE_COUNT;
		} else {
			ERR("bio_input event[type = %d, code = %d] is unknown.", bio_input.type, bio_input.code);
			return false;
		}
	}

	if (!syn) {
		ERR("EV_SYN didn't come until %d inputs had come", read_input_cnt);
		return false;
	}
	return true;
}
