/*
 * bio_data_reader
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

#include <common.h>
#include <bio_data_reader.h>


bio_data_reader::bio_data_reader()
{
}
bio_data_reader::~bio_data_reader()
{
}

bool bio_data_reader::open(void)
{
	return true;
}

bool bio_data_reader::close(void)
{
	return true;
}


bool bio_data_reader::start(void)
{
	return true;
}

bool bio_data_reader::stop(void)
{
	return true;
}

unsigned long long bio_data_reader::get_timestamp(timeval *t)
{
	if (!t) {
		ERR("t is NULL");
		return 0;
	}

	return ((unsigned long long)(t->tv_sec)*1000000LL +t->tv_usec);
}

