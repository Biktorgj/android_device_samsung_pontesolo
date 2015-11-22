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

#ifndef _EULER_ANGLES_H_
#define _EULER_ANGLES_H_

#include "vector.h"
#include "quaternion.h"

template <typename TYPE>
class euler_angles {
public:
	vect<TYPE> m_ang;

	euler_angles();
	euler_angles(const TYPE roll, const TYPE pitch, const TYPE azimuth);
	euler_angles(const vect<TYPE> v);
	euler_angles(const euler_angles<TYPE>& e);
	~euler_angles();

	euler_angles<TYPE> operator =(const euler_angles<TYPE>& e);

	template<typename T> friend euler_angles<T> quat2euler(const quaternion<T> q);
	template<typename T> friend euler_angles<T> rad2deg(const euler_angles<T> e);
	template<typename T> friend euler_angles<T> deg2rad(const euler_angles<T> e);
};

#include "euler_angles.cpp"

#endif  //_EULER_ANGLES_H_
