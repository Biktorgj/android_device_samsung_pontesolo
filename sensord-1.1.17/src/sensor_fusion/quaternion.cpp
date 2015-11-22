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

#if defined (_QUATERNION_H_) && defined (_VECTOR_H_)

#include <math.h>

#define QUAT_SIZE 4

template <typename T> int sgn(T val) {
	if (val >= 0)
		return 1;
	else
		return -1;
}

template <typename T> T mag(T val) {
	if (val < 0)
		return val * (T)-1;
	else
		return val;
}

template <typename TYPE>
quaternion<TYPE>::quaternion() : m_quat(QUAT_SIZE)
{
}

template <typename TYPE>
quaternion<TYPE>::quaternion(const TYPE w, const TYPE x, const TYPE y, const TYPE z)
{
	TYPE vec_data[QUAT_SIZE] = {w, x, y, z};

	vect<TYPE> v(QUAT_SIZE, vec_data);
	m_quat = v;
}

template <typename TYPE>
quaternion<TYPE>::quaternion(const vect<TYPE> v)
{
	m_quat = v;
}

template <typename TYPE>
quaternion<TYPE>::quaternion(const quaternion<TYPE>& q)
{
	m_quat = q.m_quat;
}

template <typename TYPE>
quaternion<TYPE>::~quaternion()
{
}

template <typename TYPE>
quaternion<TYPE> quaternion<TYPE>::operator =(const quaternion<TYPE>& q)
{
	m_quat = q.m_quat;

	return *this;
}

template <typename TYPE>
void quaternion<TYPE>::quat_normalize()
{
	TYPE w, x, y, z;
	TYPE val;

	w = m_quat.m_vec[0] * m_quat.m_vec[0];
	x = m_quat.m_vec[1] * m_quat.m_vec[1];
	y = m_quat.m_vec[2] * m_quat.m_vec[2];
	z = m_quat.m_vec[3] * m_quat.m_vec[3];

	val = sqrt(w + x + y + z);

	m_quat = m_quat / val;
}

template <typename T>
quaternion<T> operator *(const quaternion<T> q, const T val)
{
	return (q.m_quat * val);
}

template <typename T>
quaternion<T> operator *(const quaternion<T> q1, const quaternion<T> q2)
{
	T w, x, y, z;
	T w1, x1, y1, z1;
	T w2, x2, y2, z2;

	w1 = q1.m_quat.m_vec[0];
	x1 = q1.m_quat.m_vec[1];
	y1 = q1.m_quat.m_vec[2];
	z1 = q1.m_quat.m_vec[3];

	w2 = q2.m_quat.m_vec[0];
	x2 = q2.m_quat.m_vec[1];
	y2 = q2.m_quat.m_vec[2];
	z2 = q2.m_quat.m_vec[3];

	x = x1*w2 + y1*z2 - z1*y2 + w1*x2;
	y = -x1*z2 + y1*w2 + z1*x2 + w1*y2;
	z = x1*y2 - y1*x2 + z1*w2 + w1*z2;
	w = -x1*x2 - y1*y2 - z1*z2 + w1*w2;

	quaternion<T> q(w, x, y, z);

	return q;
}

template <typename T>
quaternion<T> operator +(const quaternion<T> q1, const quaternion<T> q2)
{
	return (q1.m_quat + q2.m_quat);
}


template<typename T>
quaternion<T> phase_correction(const quaternion<T> q1, const quaternion<T> q2)
{
	T w, x, y, z;
	w = mag(q1.m_quat.m_vec[0]) * sgn(q2.m_quat.m_vec[0]);
	x = mag(q1.m_quat.m_vec[1]) * sgn(q2.m_quat.m_vec[1]);
	y = mag(q1.m_quat.m_vec[2]) * sgn(q2.m_quat.m_vec[2]);
	z = mag(q1.m_quat.m_vec[3]) * sgn(q2.m_quat.m_vec[3]);

	quaternion<T> q(w, x, y, z);

	return q;
}
#endif  //_QUATERNION_H_
