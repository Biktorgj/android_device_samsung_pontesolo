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

#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <assert.h>
#include <iostream>
using namespace std;

template <typename TYPE>
class matrix {
public:
	int m_rows;
	int m_cols;
	TYPE **m_mat;

	matrix(void);
	matrix(const int rows, const int cols);
	matrix(const int rows, const int cols, TYPE *mat_data);
	matrix(const matrix<TYPE>& m);
	~matrix();

	matrix<TYPE> operator =(const matrix<TYPE>& m);

	template<typename T> friend ostream& operator << (ostream& dout,
			matrix<T>& m);
	template<typename T> friend matrix<T> operator +(const matrix<T> m1,
			const matrix<T> m2);
	template<typename T> friend matrix<T> operator +(const matrix<T> m,
			const T val);
	template<typename T> friend matrix<T> operator -(const matrix<T> m1,
			const matrix<T> m2);
	template<typename T> friend matrix<T> operator -(const matrix<T> m,
			const T val);
	template<typename T> friend matrix<T> operator *(const matrix<T> m1,
			const matrix<T> m2);
	template<typename T> friend matrix<T> operator *(const matrix<T> m,
			const T val);
	template<typename T> friend matrix<T> operator /(const matrix<T> m1,
			 const T val);
	template<typename T> friend bool operator ==(const matrix<T> m1,
			const matrix<T> m2);
	template<typename T> friend bool operator !=(const matrix<T> m1,
			const matrix<T> m2);

	template<typename T> friend matrix<T> tran(const matrix<T> m);
	template <typename T> friend matrix<T> mul(const matrix<T> m1,
			const matrix<T> m2);
};

#include "matrix.cpp"

#endif  //_MATRIX_H_
