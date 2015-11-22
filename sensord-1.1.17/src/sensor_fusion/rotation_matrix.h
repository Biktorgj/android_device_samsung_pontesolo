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

#ifndef _ROTATION_MATRIX_H_
#define _ROTATION_MATRIX_H_

#include "matrix.h"
#include "quaternion.h"

template <typename TYPE>
class rotation_matrix {
public:
	matrix<TYPE> m_rot_mat;

	rotation_matrix();
	rotation_matrix(const matrix<TYPE> m);
	rotation_matrix(const int rows, const int cols, TYPE *mat_data);
	rotation_matrix(const rotation_matrix<TYPE>& rm);
	~rotation_matrix();

	rotation_matrix<TYPE> operator =(const rotation_matrix<TYPE>& rm);

	template<typename T> friend rotation_matrix<T> quat2rot_mat(quaternion<T> q);
	template<typename T> friend quaternion<T> rot_mat2quat(rotation_matrix<T> rm);
};

#include "rotation_matrix.cpp"

#endif /* _ROTATION_MATRIX_H_ */
