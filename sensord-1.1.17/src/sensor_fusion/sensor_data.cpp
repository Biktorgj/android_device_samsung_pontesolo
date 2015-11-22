/*
 * sensord
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

#if defined (_SENSOR_DATA_H_) && defined (_VECTOR_H_)

#include "math.h"

#define SENSOR_DATA_SIZE 3

template <typename TYPE>
sensor_data<TYPE>::sensor_data() : m_data(SENSOR_DATA_SIZE), m_time_stamp(0)
{
}

template <typename TYPE>
sensor_data<TYPE>::sensor_data(const TYPE x, const TYPE y,
		const TYPE z, const unsigned long long time_stamp)
{
	TYPE vec_data[SENSOR_DATA_SIZE] = {x, y, z};

	vect<TYPE> v(SENSOR_DATA_SIZE, vec_data);
	m_data = v;
	m_time_stamp = time_stamp;
}

template <typename TYPE>
sensor_data<TYPE>::sensor_data(const vect<TYPE> v,
		const unsigned long long time_stamp)
{
	m_data = v;
	m_time_stamp = time_stamp;
}

template <typename TYPE>
sensor_data<TYPE>::sensor_data(const sensor_data<TYPE>& s)
{
	m_data = s.m_data;
	m_time_stamp = s.m_time_stamp;
}

template <typename TYPE>
sensor_data<TYPE>::~sensor_data()
{
}

template <typename TYPE>
sensor_data<TYPE> sensor_data<TYPE>::operator =(const sensor_data<TYPE>& s)
{
	m_data = s.m_data;
	m_time_stamp = s.m_time_stamp;

	return *this;
}

template <typename T>
sensor_data<T> operator +(sensor_data<T> data1, sensor_data<T> data2)
{
	return (data1.m_data + data2.m_data);
}

template <typename T>
void normalize(sensor_data<T>& data)
{
	T x, y, z;

	x = data.m_data.m_vec[0];
	y = data.m_data.m_vec[1];
	z = data.m_data.m_vec[2];

	T val = sqrt(x*x + y*y + z*z);

	data.m_data.m_vec[0] = x / val;
	data.m_data.m_vec[1] = y / val;
	data.m_data.m_vec[2] = z / val;
}

template <typename T>
sensor_data<T> scale_data(sensor_data<T> data, T scaling_factor)
{
	T x, y, z;

	x = data.m_data.m_vec[0] / scaling_factor;
	y = data.m_data.m_vec[1] / scaling_factor;
	z = data.m_data.m_vec[2] / scaling_factor;

	sensor_data<T> s(x, y, z, data.m_time_stamp);

	return s;
}

#endif /* _SENSOR_DATA_H_ */

