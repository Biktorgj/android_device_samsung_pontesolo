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


#ifdef _ORIENTATION_FILTER_H_

#include "orientation_filter.h"

//Windowing is used for buffering of previous samples for statistical analysis
#define MOVING_AVERAGE_WINDOW_LENGTH	20
//Earth's Gravity
#define GRAVITY		9.80665
#define PI		3.141593
//Needed for non-zero initialization for statistical analysis
#define NON_ZERO_VAL	0.1
//microseconds to seconds
#define US2S	(1.0 / 1000000.0)
//Initialize sampling interval to 100000microseconds
#define SAMPLE_INTV		100000

// constants for computation of covariance and transition matrices
#define ZIGMA_W		(0.05 * DEG2RAD)
#define TAU_W		1000
#define QWB_CONST	((2 * (ZIGMA_W * ZIGMA_W)) / TAU_W)
#define F_CONST		(-1 / TAU_W)

#define NEGLIGIBLE_VAL 0.0000001

#define ABS(val) (((val) < 0) ? -(val) : (val))

// M-matrix, V-vector, MxN=> matrix dimension, R-RowCount, C-Column count
#define M3X3R	3
#define M3X3C	3
#define M6X6R	6
#define M6X6C	6
#define V1x3S	3
#define V1x4S	4
#define V1x6S	6

template <typename TYPE>
orientation_filter<TYPE>::orientation_filter()
{
	TYPE arr[MOVING_AVERAGE_WINDOW_LENGTH];

	std::fill_n(arr, MOVING_AVERAGE_WINDOW_LENGTH, NON_ZERO_VAL);

	vect<TYPE> vec(MOVING_AVERAGE_WINDOW_LENGTH, arr);
	vect<TYPE> vec1x3(V1x3S);
	vect<TYPE> vec1x6(V1x6S);
	matrix<TYPE> mat6x6(M6X6R, M6X6C);

	m_var_gyr_x = vec;
	m_var_gyr_y = vec;
	m_var_gyr_z = vec;
	m_var_roll = vec;
	m_var_pitch = vec;
	m_var_azimuth = vec;

	m_tran_mat = mat6x6;
	m_measure_mat = mat6x6;
	m_pred_cov = mat6x6;
	m_driv_cov = mat6x6;
	m_aid_cov = mat6x6;

	m_bias_correction = vec1x3;
	m_state_new = vec1x6;
	m_state_old = vec1x6;
	m_state_error = vec1x6;

	m_pitch_phase_compensation = 1;
	m_roll_phase_compensation = 1;
	m_azimuth_phase_compensation = 1;
	m_magnetic_alignment_factor = 1;

	m_gyro.m_time_stamp = 0;
}

template <typename TYPE>
orientation_filter<TYPE>::~orientation_filter()
{
}

template <typename TYPE>
inline void orientation_filter<TYPE>::initialize_sensor_data(const sensor_data<TYPE> accel,
		const sensor_data<TYPE> gyro, const sensor_data<TYPE> magnetic)
{
	unsigned long long sample_interval_gyro = SAMPLE_INTV;

	m_accel.m_data = accel.m_data;
	m_gyro.m_data = gyro.m_data;
	m_magnetic.m_data = magnetic.m_data;

	if (m_gyro.m_time_stamp != 0 && gyro.m_time_stamp != 0)
		sample_interval_gyro = 	gyro.m_time_stamp - m_gyro.m_time_stamp;

	m_gyro_dt = sample_interval_gyro * US2S;

	m_accel.m_time_stamp = accel.m_time_stamp;
	m_gyro.m_time_stamp = gyro.m_time_stamp;
	m_magnetic.m_time_stamp = magnetic.m_time_stamp;

	m_gyro.m_data = m_gyro.m_data - m_bias_correction;
}

template <typename TYPE>
inline void orientation_filter<TYPE>::orientation_triad_algorithm()
{
	TYPE arr_acc_e[V1x3S] = {0.0, 0.0, 1.0};
	TYPE arr_mag_e[V1x3S] = {0.0, (TYPE) m_magnetic_alignment_factor, 0.0};

	vect<TYPE> acc_e(V1x3S, arr_acc_e);
	vect<TYPE> mag_e(V1x3S, arr_mag_e);

	vect<TYPE> acc_b_x_mag_b = cross(m_accel.m_data, m_magnetic.m_data);
	vect<TYPE> acc_e_x_mag_e = cross(acc_e, mag_e);

	vect<TYPE> cross1 = cross(acc_b_x_mag_b, m_accel.m_data);
	vect<TYPE> cross2 = cross(acc_e_x_mag_e, acc_e);

	matrix<TYPE> mat_b(M3X3R, M3X3C);
	matrix<TYPE> mat_e(M3X3R, M3X3C);

	for(int i = 0; i < M3X3R; i++)
	{
		mat_b.m_mat[i][0] = m_accel.m_data.m_vec[i];
		mat_b.m_mat[i][1] = acc_b_x_mag_b.m_vec[i];
		mat_b.m_mat[i][2] = cross1.m_vec[i];
		mat_e.m_mat[i][0] = acc_e.m_vec[i];
		mat_e.m_mat[i][1] = acc_e_x_mag_e.m_vec[i];
		mat_e.m_mat[i][2] = cross2.m_vec[i];
	}

	matrix<TYPE> mat_b_t = tran(mat_b);
	rotation_matrix<TYPE> rot_mat(mat_e * mat_b_t);

	m_quat_aid = rot_mat2quat(rot_mat);
}

template <typename TYPE>
inline void orientation_filter<TYPE>::compute_covariance()
{
	TYPE var_gyr_x, var_gyr_y, var_gyr_z;
	TYPE var_roll, var_pitch, var_azimuth;

	insert_end(m_var_gyr_x, m_gyro.m_data.m_vec[0]);
	insert_end(m_var_gyr_y, m_gyro.m_data.m_vec[1]);
	insert_end(m_var_gyr_z, m_gyro.m_data.m_vec[2]);
	insert_end(m_var_roll, m_orientation.m_ang.m_vec[0]);
	insert_end(m_var_pitch, m_orientation.m_ang.m_vec[1]);
	insert_end(m_var_azimuth, m_orientation.m_ang.m_vec[2]);

	var_gyr_x = var(m_var_gyr_x);
	var_gyr_y = var(m_var_gyr_y);
	var_gyr_z = var(m_var_gyr_z);
	var_roll = var(m_var_roll);
	var_pitch = var(m_var_pitch);
	var_azimuth = var(m_var_azimuth);

	m_driv_cov.m_mat[0][0] = var_gyr_x;
	m_driv_cov.m_mat[1][1] = var_gyr_y;
	m_driv_cov.m_mat[2][2] = var_gyr_z;
	m_driv_cov.m_mat[3][3] = (TYPE) QWB_CONST;
	m_driv_cov.m_mat[4][4] = (TYPE) QWB_CONST;
	m_driv_cov.m_mat[5][5] = (TYPE) QWB_CONST;

	m_aid_cov.m_mat[0][0] = var_roll;
	m_aid_cov.m_mat[1][1] = var_pitch;
	m_aid_cov.m_mat[2][2] = var_azimuth;
}

template <typename TYPE>
inline void orientation_filter<TYPE>::time_update()
{
	quaternion<TYPE> quat_diff, quat_error, quat_output;
	euler_angles<TYPE> euler_error;
	euler_angles<TYPE> orientation;

	m_tran_mat.m_mat[0][1] = m_gyro.m_data.m_vec[2];
	m_tran_mat.m_mat[0][2] = -m_gyro.m_data.m_vec[1];
	m_tran_mat.m_mat[1][0] = -m_gyro.m_data.m_vec[2];
	m_tran_mat.m_mat[1][2] = m_gyro.m_data.m_vec[0];
	m_tran_mat.m_mat[2][0] = m_gyro.m_data.m_vec[1];
	m_tran_mat.m_mat[2][1] = -m_gyro.m_data.m_vec[0];
	m_tran_mat.m_mat[3][3] = (TYPE) F_CONST;
	m_tran_mat.m_mat[4][4] = (TYPE) F_CONST;
	m_tran_mat.m_mat[5][5] = (TYPE) F_CONST;

	m_measure_mat.m_mat[0][0] = 1;
	m_measure_mat.m_mat[1][1] = 1;
	m_measure_mat.m_mat[2][2] = 1;

	if (is_initialized(m_state_old))
		m_state_new = transpose(mul(m_tran_mat, transpose(m_state_old)));

	m_pred_cov = (m_tran_mat * m_pred_cov * tran(m_tran_mat)) + m_driv_cov;

	if(!is_initialized(m_quat_driv.m_quat))
		m_quat_driv = m_quat_aid;

	quaternion<TYPE> quat_rot_inc(0, m_gyro.m_data.m_vec[0], m_gyro.m_data.m_vec[1],
			m_gyro.m_data.m_vec[2]);

	quat_diff = (m_quat_driv * quat_rot_inc) * (TYPE) 0.5;

	m_quat_driv = m_quat_driv + (quat_diff * (TYPE) m_gyro_dt * (TYPE) PI);
	m_quat_driv.quat_normalize();
	quat_output = phase_correction(m_quat_driv, m_quat_aid);

	m_quaternion = quat_output;

	orientation = quat2euler(quat_output);

	m_orientation.m_ang.m_vec[0] = orientation.m_ang.m_vec[0] * m_pitch_phase_compensation;
	m_orientation.m_ang.m_vec[1] = orientation.m_ang.m_vec[1] * m_roll_phase_compensation;
	m_orientation.m_ang.m_vec[2] = orientation.m_ang.m_vec[2] * m_azimuth_phase_compensation;

	m_rot_matrix = quat2rot_mat(m_quat_driv);

	quat_error = m_quat_aid * m_quat_driv;

	euler_error = (quat2euler(quat_error)).m_ang / (TYPE) PI;

	quaternion<TYPE> quat_eu_er(1, euler_error.m_ang.m_vec[0], euler_error.m_ang.m_vec[1],
			euler_error.m_ang.m_vec[2]);

	m_quat_driv = (m_quat_driv * quat_eu_er) * (TYPE) PI;
	m_quat_driv.quat_normalize();

	if (is_initialized(m_state_new))
	{
		m_state_error.m_vec[0] = euler_error.m_ang.m_vec[0];
		m_state_error.m_vec[1] = euler_error.m_ang.m_vec[1];
		m_state_error.m_vec[2] = euler_error.m_ang.m_vec[2];
		m_state_error.m_vec[3] = m_state_new.m_vec[3];
		m_state_error.m_vec[4] = m_state_new.m_vec[4];
		m_state_error.m_vec[5] = m_state_new.m_vec[5];
	}
}

template <typename TYPE>
inline void orientation_filter<TYPE>::measurement_update()
{
	matrix<TYPE> gain(M6X6R, M6X6C);
	TYPE iden = 0;

	for (int j=0; j<M6X6C; ++j) {
		for (int i=0; i<M6X6R; ++i)	{
			gain.m_mat[i][j] = m_pred_cov.m_mat[j][i] / (m_pred_cov.m_mat[j][j] + m_aid_cov.m_mat[j][j]);

			m_state_new.m_vec[i] = m_state_new.m_vec[i] + gain.m_mat[i][j] * m_state_error.m_vec[j];

			if (i == j)
				iden = 1;
			else
				iden = 0;

			m_pred_cov.m_mat[j][i] = (iden - (gain.m_mat[i][j] * m_measure_mat.m_mat[j][i])) *
					m_pred_cov.m_mat[j][i];

			if (ABS(m_pred_cov.m_mat[j][i]) < NEGLIGIBLE_VAL)
				m_pred_cov.m_mat[j][i] = NEGLIGIBLE_VAL;
		}
	}

	m_state_old = m_state_new;

	TYPE arr_bias[V1x3S] = {m_state_new.m_vec[3], m_state_new.m_vec[4], m_state_new.m_vec[5]};
	vect<TYPE> vec(V1x3S, arr_bias);

	m_bias_correction = vec;
}

template <typename TYPE>
euler_angles<TYPE> orientation_filter<TYPE>::get_orientation(const sensor_data<TYPE> accel,
		const sensor_data<TYPE> gyro, const sensor_data<TYPE> magnetic)
{
	euler_angles<TYPE> cor_euler_ang;

	initialize_sensor_data(accel, gyro, magnetic);

	normalize(m_accel);
	m_gyro.m_data = m_gyro.m_data * (TYPE) PI;
	normalize(m_magnetic);

	orientation_triad_algorithm();

	compute_covariance();

	time_update();

	measurement_update();

	return m_orientation;
}

template <typename TYPE>
rotation_matrix<TYPE> orientation_filter<TYPE>::get_rotation_matrix(const sensor_data<TYPE> accel,
		const sensor_data<TYPE> gyro, const sensor_data<TYPE> magnetic)
{
	get_orientation(accel, gyro, magnetic);

	return m_rot_matrix;
}

template <typename TYPE>
quaternion<TYPE> orientation_filter<TYPE>::get_quaternion(const sensor_data<TYPE> accel,
		const sensor_data<TYPE> gyro, const sensor_data<TYPE> magnetic)
{
	get_orientation(accel, gyro, magnetic);

	return m_quaternion;
}

#endif  //_ORIENTATION_FILTER_H_
