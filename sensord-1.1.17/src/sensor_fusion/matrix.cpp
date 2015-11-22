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

#ifdef _MATRIX_H_

template <typename TYPE>
matrix<TYPE>::matrix(void)
{
	m_mat = NULL;
}

template <typename TYPE>
matrix<TYPE>::matrix(const int rows, const int cols)
{
	m_rows = rows;
	m_cols = cols;
	m_mat = NULL;
	m_mat = new TYPE *[m_rows];

	for (int i = 0; i < m_rows; i++)
		m_mat[i] = new TYPE [m_cols]();
}

template <typename TYPE>
matrix<TYPE>::matrix(const matrix<TYPE>& m)
{
	m_rows = m.m_rows;
	m_cols = m.m_cols;
	m_mat = NULL;
	m_mat = new TYPE *[m_rows];

	for (int i = 0; i < m_rows; i++)
		m_mat[i] = new TYPE [m_cols];

	for (int p = 0; p < m_rows; p++)
		for (int q = 0; q < m_cols; q++)
			m_mat[p][q] = m.m_mat[p][q];
}

template <typename TYPE>
matrix<TYPE>::matrix(const int rows, const int cols, TYPE *mat_data)
{
	m_rows = rows;
	m_cols = cols;
	m_mat = NULL;
	m_mat = new TYPE *[m_rows];

	for (int i = 0; i < m_rows; i++)
		m_mat[i] = new TYPE [m_cols];

	for (int i = 0; i < m_rows; i++)
		for (int j = 0; j < m_cols; j++)
			m_mat[i][j] = *mat_data++;
}

template <typename TYPE>
matrix<TYPE>::~matrix()
{
	if (m_mat != NULL)
	{
		for (int i = 0; i < m_rows; i++)
			delete[] m_mat[i];
		delete[] m_mat;
	}
}

template <typename TYPE>
matrix<TYPE> matrix<TYPE>::operator =(const matrix<TYPE>& m)
{
	if (this == &m)
	{
		return *this;
	}

	if (m_mat == NULL)
	{
		m_rows = m.m_rows;
		m_cols = m.m_cols;
		m_mat = new TYPE *[m_rows];

		for (int i = 0; i < m_rows; i++)
			m_mat[i] = new TYPE [m_cols];
	}
	else
	{
		if ((m_rows != m.m_rows) || (m_cols != m.m_cols))
		{
			for (int i = 0; i < m_rows; i++)
				delete[] m_mat[i];
			delete[] m_mat;

			m_rows = m.m_rows;
			m_cols = m.m_cols;
			m_mat = new TYPE *[m_rows];

			for (int i = 0; i < m_rows; i++)
				m_mat[i] = new TYPE [m_cols];
		}
	}

	for (int p = 0; p < m_rows; p++)
		for (int q = 0; q < m_cols; q++)
			m_mat[p][q] = m.m_mat[p][q];

	return *this;
}

template <typename T>
ostream& operator <<(ostream& dout, matrix<T>& m)
{
	for (int i = 0; i < m.m_rows; i++)
	{
		for (int j = 0; j < m.m_cols; j++)
		{
			dout << m.m_mat[i][j] << "\t";
		}
		dout << endl;
	}
	return dout;
}

template <typename T>
matrix<T> operator +(const matrix<T> m1, const matrix<T> m2)
{
	assert(m1.m_rows == m2.m_rows);
	assert(m1.m_cols == m2.m_cols);

	matrix<T> m3(m1.m_rows, m1.m_cols);

	for (int i = 0; i < m1.m_rows; i++)
		for (int j = 0; j < m1.m_cols; j++)
			m3.m_mat[i][j] = m1.m_mat[i][j] + m2.m_mat[i][j];

	return m3;
}

template <typename T>
matrix<T> operator +(const matrix<T> m, const T val)
{
	matrix<T> m1(m.m_rows, m.m_cols);

	for (int i = 0; i < m.m_rows; i++)
		for (int j = 0; j < m.m_cols; j++)
			m1.m_mat[i][j] = m.m_mat[i][j] + val;

	return m1;
}

template <typename T>
matrix<T> operator -(const matrix<T> m1, const matrix<T> m2)
{
	assert(m1.m_rows == m2.m_rows);
	assert(m1.m_cols == m2.m_cols);

	matrix<T> m3(m1.m_rows, m1.m_cols);

	for (int i = 0; i < m1.m_rows; i++)
		for (int j = 0; j < m1.m_cols; j++)
			m3.m_mat[i][j] = m1.m_mat[i][j] - m2.m_mat[i][j];

	return m3;
}

template <typename T>
matrix<T> operator -(const matrix<T> m, const T val)
{
	matrix<T> m1(m.m_rows, m.m_cols);

	for (int i = 0; i < m.m_rows; i++)
		for (int j = 0; j < m.m_cols; j++)
			m1.m_mat[i][j] = m.m_mat[i][j] - val;

	return m1;
}

template <typename T>
matrix<T> operator *(const matrix<T> m1, const matrix<T> m2)
{
	assert(m1.m_rows == m2.m_cols);
	assert(m1.m_cols == m2.m_rows);

	matrix<T> m3(m1.m_rows, m2.m_cols);

	for (int i = 0; i < m1.m_rows; i++)
	{
		for (int j = 0; j < m2.m_cols; j++)
		{
			m3.m_mat[i][j] = 0;
			for (int k = 0; k < m2.m_rows; k++)
				m3.m_mat[i][j] += m1.m_mat[i][k] * m2.m_mat[k][j];
		}
	}

	return m3;
}

template <typename T>
matrix<T> operator *(const matrix<T> m, const T val)
{
	matrix<T> m1(m.m_rows, m.m_cols);

	for (int i = 0; i < m.m_rows; i++)
		for (int j = 0; j < m.m_cols; j++)
			m1.m_mat[i][j] = m.m_mat[i][j] * val;

	return m1;
}

template <typename T>
matrix<T> operator /(const matrix<T> m1, const T val)
{
	matrix<T> m3(m1.m_rows, m1.m_cols);

	for (int i = 0; i < m1.m_rows; i++)
		for (int j = 0; j < m1.m_cols; j++)
			m3.m_mat[i][j] = m1.m_mat[i][j] / val;

	return m3;
}

template <typename T>
bool operator ==(const matrix<T> m1, const matrix<T> m2)
{
	if ((m1.m_rows == m2.m_rows) && (m1.m_cols == m2.m_cols))
	{
		for (int i = 0; i < m1.m_rows; i++)
			for (int j = 0; j < m2.m_cols; j++)
				if (m1.m_mat[i][j] != m2.m_mat[i][j])
					return false;
	}
	else
		return false;

	return true;
}

template <typename T>
bool operator !=(const matrix<T> m1, const matrix<T> m2)
{
	return (!(m1 == m2));
}

template <typename T>
matrix<T> tran(const matrix<T> m)
{
	matrix<T> m1(m.m_cols, m.m_rows);

	for (int i = 0; i < m.m_rows; i++)
		for (int j = 0; j < m.m_cols; j++)
			m1.m_mat[j][i] = m.m_mat[i][j];

	return m1;
}


template <typename T>
matrix<T> mul(const matrix<T> m1, const matrix<T> m2)
{
	assert(m2.m_cols == 1);
	assert(m1.m_cols == m2.m_rows);

	matrix<T> m3(m1.m_rows, 1);

	for (int i = 0; i < m1.m_rows; i++)
	{
			m3.m_mat[i][0] = 0;
			for (int k = 0; k < m2.m_rows; k++)
				m3.m_mat[i][0] += m1.m_mat[i][k] * m2.m_mat[k][0];
	}

	return m3;
}

#endif //_MATRIX_H_
