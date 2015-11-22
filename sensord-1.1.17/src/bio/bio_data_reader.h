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
#ifndef _BIO_DATA_READER_H_
#define _BIO_DATA_READER_H_


#include <bio_data_reader.h>
#include <sys/time.h>
#include <sensor_common.h>

class bio_data_reader
{
public:
	bio_data_reader();
	virtual ~bio_data_reader();

	virtual bool open(void);
	virtual bool close(void);
	virtual bool start(void);
	virtual bool stop(void);
	virtual bool get_data(int handle, sensor_data_t &data) = 0;
	unsigned long long get_timestamp(timeval *t);
};
#endif
