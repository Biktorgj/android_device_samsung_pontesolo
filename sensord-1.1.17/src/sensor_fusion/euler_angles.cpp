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
#if defined (_EULER_ANGLES_H_) && defined (_VECTOR_H_)

#include <math.h>

#define EULER_SIZE 3
#define RAD2DEG 57.2957795
#define DEG2RAD 0.0174532925

template <typename TYPE>
euler_angles<TYPE>::euler_angles() : m_ang(EULER_SIZE)
{
}

template <typename TYPE>
euler_angles<TYPE>::euler_angles(const TYPE roll, const TYPE pitch, const TYPE azimuth)
{
	TYPE euler_data[EULER_SIZE] = {roll, pitch, azimuth};

	vect<TYPE> v(EULER_SIZE, euler_data);
	m_ang = v;
}

template <typename TYPE>
euler_angles<TYPE>::euler_angles(const vect<TYPE> v)
{
	m_ang = v;
}

template <typename TYPE>
euler_angles<TYPE>::euler_angles(const euler_angles<TYPE>& e)
{
	m_ang = e.m_ang;
}

template <typename TYPE>
euler_angles<TYPE>::~euler_angles()
{

}

template <typename TYPE>
euler_angles<TYPE> euler_angles<TYPE>::operator =(const euler_angles<TYPE>& e)
{
	m_ang = e.m_ang;

	return *this;
}

template <typename T>
euler_angles<T> quat2euler(const quaternion<T> q)
{
	T w, x, y, z;
	T R11, R21, R31, R32, R33;
	T phi, theta, psi;

	w = q.m_quat.m_vec[0];
	x = q.m_quat.m_vec[1];
	y = q.m_quat.m_vec[2];
	z = q.m_quat.m_vec[3];

	R11 = 2 * (w * w) - 1 + 2 * (x * x);
	R21 = 2 * ((x * y) - (w * z));
	R31 = 2 * ((x * z) + (w * y));
	R32 = 2 * ((y * z) - (w * x));
	R33 = 2 * (w * w) - 1 + 2 * (z * z);

	phi = atan2(R32, R33);
	theta = atan2(-R31 , sqrt((R32 * R32) + (R33 * R33)));
	psi = atan2(R21, R11);

	euler_angles<T> e(phi, theta, psi);

	return e;
}

template <typename T>
euler_angles<T> rad2deg(const euler_angles<T> e)
{
	return (e.m_ang * (T) RAD2DEG);
}

template <typename T>
euler_angles<T> deg2rad(const euler_angles<T> e)
{
	return (e.m_ang * (T) DEG2RAD);
}

#endif  //_EULER_ANGLES_H_
