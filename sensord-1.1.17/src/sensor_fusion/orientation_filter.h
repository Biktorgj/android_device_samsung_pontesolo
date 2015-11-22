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

#ifndef _ORIENTATION_FILTER_H_
#define _ORIENTATION_FILTER_H_

#include "matrix.h"
#include "vector.h"
#include "sensor_data.h"
#include "quaternion.h"
#include "euler_angles.h"
#include "rotation_matrix.h"

template <typename TYPE>
class orientation_filter {
public:
	sensor_data<TYPE> m_accel;
	sensor_data<TYPE> m_gyro;
	sensor_data<TYPE> m_magnetic;
	vect<TYPE> m_var_gyr_x;
	vect<TYPE> m_var_gyr_y;
	vect<TYPE> m_var_gyr_z;
	vect<TYPE> m_var_roll;
	vect<TYPE> m_var_pitch;
	vect<TYPE> m_var_azimuth;
	matrix<TYPE> m_driv_cov;
	matrix<TYPE> m_aid_cov;
	matrix<TYPE> m_tran_mat;
	matrix<TYPE> m_measure_mat;
	matrix<TYPE> m_pred_cov;
	vect<TYPE> m_state_new;
	vect<TYPE> m_state_old;
	vect<TYPE> m_state_error;
	vect<TYPE> m_bias_correction;
	quaternion<TYPE> m_quat_aid;
	quaternion<TYPE> m_quat_driv;
	rotation_matrix<TYPE> m_rot_matrix;
	euler_angles<TYPE> m_orientation;
	quaternion<TYPE> m_quaternion;
	TYPE m_gyro_dt;

	int m_pitch_phase_compensation;
	int m_roll_phase_compensation;
	int m_azimuth_phase_compensation;
	int m_magnetic_alignment_factor;

	orientation_filter();
	~orientation_filter();

	inline void initialize_sensor_data(const sensor_data<TYPE> accel,
			const sensor_data<TYPE> gyro, const sensor_data<TYPE> magnetic);
	inline void orientation_triad_algorithm();
	inline void compute_covariance();
	inline void time_update();
	inline void measurement_update();

	euler_angles<TYPE> get_orientation(const sensor_data<TYPE> accel,
			const sensor_data<TYPE> gyro, const sensor_data<TYPE> magnetic);
	rotation_matrix<TYPE> get_rotation_matrix(const sensor_data<TYPE> accel,
			const sensor_data<TYPE> gyro, const sensor_data<TYPE> magnetic);
	quaternion<TYPE> get_quaternion(const sensor_data<TYPE> accel,
			const sensor_data<TYPE> gyro, const sensor_data<TYPE> magnetic);
};

#include "orientation_filter.cpp"

#endif /* _ORIENTATION_FILTER_H_ */
