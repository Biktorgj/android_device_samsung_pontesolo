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

#ifndef _VECTOR_H_
#define _VECTOR_H_

#include "matrix.h"

template <typename TYPE>
class vect {
public:
	int m_size;
	TYPE *m_vec;

	vect(void);
	vect(const int size);
	vect(const int size, TYPE *vec_data);
	vect(const vect<TYPE>& v);
	~vect();

	vect<TYPE> operator =(const vect<TYPE>& v);

	template<typename T> friend ostream& operator << (ostream& dout,
			vect<T>& v);
	template<typename T> friend vect<T> operator +(const vect<T> v1,
			const vect<T> v2);
	template<typename T> friend vect<T> operator +(const vect<T> v,
			const T val);
	template<typename T> friend vect<T> operator -(const vect<T> v1,
			const vect<T> v2);
	template<typename T> friend vect<T> operator -(const vect<T> v,
			const T val);
	template<typename T> friend matrix<T> operator *(const matrix<T> v1,
			const vect<T> v2);
	template<typename T> friend vect<T> operator *(const vect<T> v,
			const matrix<T> m);
	template<typename T> friend vect<T> operator *(const vect<T> v,
			const T val);
	template<typename T> friend vect<T> operator /(const vect<T> v1,
			const T val);
	template<typename T> friend bool operator ==(const vect<T> v1,
			const vect<T> v2);
	template<typename T> friend bool operator !=(const vect<T> v1,
			const vect<T> v2);

	template<typename T> friend T mul(const vect<T> v, const matrix<T> m);
	template<typename T> friend void insert_end(vect<T>& v, T val);
	template<typename T> friend matrix<T> transpose(const vect<T> v);
	template <typename T> friend vect<T> transpose(const matrix<T> m);
	template<typename T> friend vect<T> cross(const vect<T> v1,
			const vect<T> v2);
	template <typename T> friend T var(const vect<T> v);
	template <typename T> friend bool is_initialized(const vect<T> v);
};

#include "vector.cpp"

#endif /* _VECTOR_H_ */
