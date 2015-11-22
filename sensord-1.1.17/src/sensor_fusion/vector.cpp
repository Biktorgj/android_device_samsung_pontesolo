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

#if defined (_VECTOR_H_) && defined (_MATRIX_H_)

template <typename TYPE>
vect<TYPE>::vect(void)
{
	m_vec = NULL;
}

template <typename TYPE>
vect<TYPE>::vect(const int size)
{
	m_size = size;
	m_vec = NULL;
	m_vec = new TYPE [m_size]();
}

template <typename TYPE>
vect<TYPE>::vect(const int size, TYPE *vec_data)
{
	m_size = size;
	m_vec = NULL;
	m_vec = new TYPE [m_size];

	for (int j = 0; j < m_size; j++)
		m_vec[j] = *vec_data++;
}

template <typename TYPE>
vect<TYPE>::vect(const vect<TYPE>& v)
{
	m_size = v.m_size;
	m_vec = NULL;
	m_vec = new TYPE [m_size];

	for (int q = 0; q < m_size; q++)
		m_vec[q] = v.m_vec[q];
}

template <typename TYPE>
vect<TYPE>::~vect()
{
	if (m_vec != NULL)
		delete[] m_vec;
}

template <typename TYPE>
vect<TYPE> vect<TYPE>::operator =(const vect<TYPE>& v)
{
	if (this == &v)
	{
		return *this;
	}

	if (m_vec == NULL)
	{
		m_size = v.m_size;
		m_vec = new TYPE [m_size];
	}
	else
	{
		if (m_size != v.m_size)
		{
			delete[] m_vec;

			m_size = v.m_size;
			m_vec = new TYPE [m_size];
		}
	}


	for (int q = 0; q < m_size; q++)
		m_vec[q] = v.m_vec[q];


	return *this;
}

template <typename TYPE>
ostream& operator <<(ostream& dout, vect<TYPE>& v)
{
	for (int j = 0; j < v.m_size; j++)
	{
		dout << v.m_vec[j] << "\t";
	}

	dout << endl;

	return dout;
}

template <typename T>
vect<T> operator +(const vect<T> v1, const vect<T> v2)
{
	assert(v1.m_size == v2.m_size);

	vect<T> v3(v1.m_size);

	for (int j = 0; j < v1.m_size; j++)
		v3.m_vec[j] = v1.m_vec[j] + v2.m_vec[j];

	return v3;
}

template <typename T>
vect<T> operator +(const vect<T> v, const T val)
{
	vect<T> v1(v.m_size);

	for (int j = 0; j < v.m_size; j++)
		v1.m_vec[j] = v.m_vec[j] + val;

	return v1;
}

template <typename T>
vect<T> operator -(const vect<T> v1, const vect<T> v2)
{
	assert(v1.m_size == v2.m_size);

	vect<T> v3(v1.m_size);

	for (int j = 0; j < v1.m_size; j++)
		v3.m_vec[j] = v1.m_vec[j] - v2.m_vec[j];

	return v3;
}

template <typename T>
vect<T> operator -(const vect<T> v, const T val)
{
	vect<T> v1(v.m_size);

	for (int j = 0; j < v.m_size; j++)
		v1.m_vec[j] = v.m_vec[j] - val;

	return v1;
}

template <typename T>
matrix<T> operator *(const matrix<T> m, const vect<T> v)
{
	assert(m.m_rows == v.m_size);
	assert(m.m_cols == 1);

	matrix<T> m1(m.m_rows, v.m_size);

	for (int i = 0; i < m1.m_rows; i++)
	{
		for (int j = 0; j < m1.m_cols; j++)
		{
			m1.m_mat[i][j] = m.m_mat[i][0] * v.m_vec[j];
		}
	}

	return m1;
}

template <typename T>
vect<T> operator *(const vect<T> v, const matrix<T> m)
{
	assert(m.m_rows == v.m_size);
	assert(m.m_cols != 1);

	vect<T> v1(m.m_cols);

	for (int j = 0; j < m.m_cols; j++)
	{
		v1.m_vec[j] = 0;
		for (int k = 0; k < m.m_rows; k++)
			v1.m_vec[j] += v.m_vec[k] * m.m_mat[k][j];
	}

	return v1;
}

template <typename T>
vect<T> operator *(const vect<T> v, const T val)
{
	vect<T> v1(v.m_size);

	for (int j = 0; j < v.m_size; j++)
		v1.m_vec[j] = v.m_vec[j] * val;

	return v1;
}

template <typename T>
vect<T> operator /(const vect<T> v, const T val)
{
	vect<T> v1(v.m_size);

	for (int j = 0; j < v.m_size; j++)
		v1.m_vec[j] = v.m_vec[j] / val;

	return v1;
}

template <typename T>
bool operator ==(const vect<T> v1, const vect<T> v2)
{
	if (v1.m_size == v2.m_size)
	{
		for (int i = 0; i < v1.m_size; i++)
			if (v1.m_vec[i] != v2.m_vec[i])
				return false;
	}
	else
		return false;

	return true;
}

template <typename T>
bool operator !=(const vect<T> v1, const vect<T> v2)
{
	return (!(v1 == v2));
}

template <typename T>
matrix<T> transpose(const vect<T> v)
{
	matrix<T> m(v.m_size, 1);

	for (int i = 0; i < v.m_size; i++)
		m.m_mat[i][0] = v.m_vec[i];

	return m;
}

template <typename T>
vect<T> transpose(const matrix<T> m)
{
	vect<T> v(m.m_rows);

	for (int i = 0; i < m.m_rows; i++)
		v.m_vec[i] = m.m_mat[i][0];

	return v;
}

template <typename T>
T mul(const vect<T> v, const matrix<T> m)
{
	assert(m.m_rows == v.m_size);
	assert(m.m_cols == 1);

	T result = (T) 0;

	for (int k = 0; k < v.m_size; k++)
		result += v.m_vec[k] * m.m_mat[k][0];

	return result;
}


template <typename T>
void insert_end(vect<T>& v, T val)
{
	for (int i = 0; i < (v.m_size - 1); i++)
		v.m_vec[i] = v.m_vec[i+1];

	v.m_vec[v.m_size-1] = val;
}

template <typename T>
vect<T> cross(const vect<T> v1, const vect<T> v2)
{
	vect<T> v3(v1.m_size);

	v3.m_vec[0] = ((v1.m_vec[1] * v2.m_vec[2]) - (v1.m_vec[2] * v2.m_vec[1]));
	v3.m_vec[1] = ((v1.m_vec[2] * v2.m_vec[0]) - (v1.m_vec[0] * v2.m_vec[2]));
	v3.m_vec[2] = ((v1.m_vec[0] * v2.m_vec[1]) - (v1.m_vec[1] * v2.m_vec[0]));

	return v3;
}

template <typename T>
bool is_initialized(const vect<T> v)
{
	vect<T> v1(v.m_size);
	bool retval;

	retval = (v == v1) ? false : true;

	return retval;
}

template <typename T>
T var(const vect<T> v)
{
	T val = 0;
	T mean, var, diff;

	for (int i = 0; i < v.m_size; i++)
		val += v.m_vec[i];

	mean = val / v.m_size;

	val = 0;
	for (int i = 0; i < v.m_size; i++)
	{
		diff = (v.m_vec[i] - mean);
		val += diff * diff;
	}

	var = val / (v.m_size - 1);

	return var;
}
#endif

