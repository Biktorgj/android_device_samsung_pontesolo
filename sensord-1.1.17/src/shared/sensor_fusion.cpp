/*
 * libsensord-share
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

#include <sensor_fusion.h>

sensor_fusion::sensor_fusion()
{

}

sensor_fusion::~sensor_fusion()
{

}

bool sensor_fusion::is_fusion(void)
{
	return true;
}

bool sensor_fusion::is_virtual(void)
{
	return false;
}

bool sensor_fusion::is_data_ready(void)
{
	return true;
}

void sensor_fusion::clear_data(void)
{
	return;
}

bool sensor_fusion::get_rotation_matrix(arr33_t &rot, int &accuracy)
{
	return false;
}

bool sensor_fusion::get_attitude(float &x, float &y, float &z, float &w)
{
	return false;
}

bool sensor_fusion::get_gyro_bias(float &x, float &y, float &z)
{
	return false;
}

bool sensor_fusion::get_rotation_vector(float &x, float &y, float &z, float &w, float &heading_accuracy, int &accuracy)
{
	return false;
}

bool sensor_fusion::get_linear_acceleration(float &x, float &y, float &z, int &accuracy)
{
	return false;
}

bool sensor_fusion::get_gravity(float &x, float &y, float &z, int &accuracy)
{
	return false;
}

bool sensor_fusion::get_rotation_vector_6axis(float &x, float &y, float &z, float &w, float &heading_accuracy, int &accuracy)
{
	return false;
}

bool sensor_fusion::get_geomagnetic_rotation_vector(float &x, float &y, float &z, float &w, int &accuracy)
{
	return false;
}

bool sensor_fusion::get_orientation(float &azimuth, float &pitch, float &roll, int &accuracy)
{
	return false;
}

bool sensor_fusion::set_interval(unsigned int event_type, int client_id, unsigned int interval)
{
	return sensor_base::add_interval(client_id, interval, true);
}

bool sensor_fusion::unset_interval(unsigned int event_type, int client_id)
{
	return sensor_base::delete_interval(client_id, true);
}

