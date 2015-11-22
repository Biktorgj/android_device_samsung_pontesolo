/*
 * bio_data_reader_adi
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

#include <bio_data_reader_adi.h>
#include <common.h>
#include <sensor_common.h>
#include <linux/input.h>

bio_data_reader_adi::bio_data_reader_adi()
{

}

bio_data_reader_adi::~bio_data_reader_adi()
{

}

bool bio_data_reader_adi::get_data(int handle, sensor_data_t &data)
{
	const int SLOT_A_RED = 0;
	const int SLOT_AB_BOTH = 1;
	const int SLOT_B_IR = 2;

	const int INPUT_MAX_BEFORE_SYN = 20;
	bool syn = false;
	int read_input_cnt = 0;
	struct input_event bio_input;

	static int ir_sum = 0, red_sum = 0;
	static int ppg_ch[8] = { 0, };
	static int ppg_ch_idx = 0;
	static int sub_mode = 0;

	while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
		int len = read(handle, &bio_input, sizeof(bio_input));
		if (len != sizeof(bio_input)) {
			ERR("bio_file read fail, read_len = %d", len);
			return false;
		}

		++read_input_cnt;

		if (bio_input.type == EV_REL) {
			switch (bio_input.code) {
			case REL_X:
				red_sum = bio_input.value - 1;
				break;
			case REL_Y:
				ir_sum = bio_input.value - 1;
				break;
			case REL_Z:
				sub_mode = bio_input.value - 1;
				break;
			default:
				ERR("bio_input event[type = %d, code = %d] is unknown", bio_input.type, bio_input.code);
				return false;
				break;
			}
		} else if (bio_input.type == EV_MSC) {
			if (bio_input.code == MSC_RAW) {
				ppg_ch[ppg_ch_idx++] = (bio_input.value - 1);
			} else {
				ERR("bio_input event[type = %d, code = %d] is unknown", bio_input.type, bio_input.code);
				return false;
			}
		} else if (bio_input.type == EV_SYN) {
			syn = true;
			ppg_ch_idx = 0;

			memset(data.values, 0, sizeof(data.values));

			if (sub_mode == SLOT_A_RED) {
				for (int i = 6; i < 10; i++)
					data.values[i] = (float)(ppg_ch[i - 6]);

			} else if(sub_mode == SLOT_AB_BOTH) {
				data.values[0] = (float)(ir_sum);
				data.values[1] = (float)(red_sum);

				for (int i = 2; i < 6; i++)
					data.values[i] = (float)(ppg_ch[i + 2]);

				for (int i = 6; i < 10; i++)
					data.values[i] = (float)(ppg_ch[i - 6]);

			} else if (sub_mode == SLOT_B_IR) {
				for (int i = 2; i < 6; i++)
					data.values[i] = (float)(ppg_ch[i - 2]);
			}

			data.values[10] = (float)(sub_mode);
			data.value_count = 11;
			data.timestamp = get_timestamp(&bio_input.time);

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
