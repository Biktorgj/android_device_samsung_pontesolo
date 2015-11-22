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


#ifndef _MOTION_SENSOR_H_
#define _MOTION_SENSOR_H_

#include <sensor_internal.h>
#include <sensor/MREngine.h>
#include <vconf.h>
#include <string>

using std::string;

class motion_sensor : public virtual_sensor {
public:
	motion_sensor();
	virtual ~motion_sensor();

	bool init();
	sensor_type_t get_type(void);

	static bool working(void *inst);

	bool add_client(unsigned int event_type);
	bool delete_client(unsigned int event_type);
	void synthesize(const sensor_event_t& event, vector<sensor_event_t> &outs);
private:
	typedef struct {
		double a_x;
		double a_y;
		double a_z;
		double g_x;
		double g_y;
		double g_z;
		int proxi;
		unsigned int sensor_usage;
	} motion_data;

	typedef struct {
		bool always_on;
		bool enabled;
		int used_sensor;
	} motion_event_info;

	typedef unordered_map<unsigned int, motion_event_info> motion_info_map;

	static const int ACC_USED = 0x1;
	static const int GYRO_USED = 0x2;
	static const int PROXI_USED = 0x4;
	static const int SAMPLING_TIME = 20;

	typedef int (*ME_init_t)(void);
	typedef unsigned char (*ME_fini_t)(void);
	typedef XYZ_TYPE* (*ME_get_gyro_t)(double g_x, double g_y, double g_z, unsigned int time_delay);
	typedef MOTION_SCENARIO_RESULT* (*ME_get_result_t)(double a_x, double a_y, double a_z, double g_x, double g_y,
		double g_z, short prox, int light, unsigned char touch, unsigned int sensor_usage, unsigned int time_delay, int face_status);
	typedef int (*ME_get_panning_dx_t)(void);
	typedef int (*ME_get_panning_dy_t)(void);
	typedef int (*ME_get_panning_dz_t)(void);
	typedef void (*ME_clear_result_t)(unsigned int id);
	typedef bool (*ME_reset_angle_t)(int);

	virtual bool on_start(void);
	virtual bool on_stop(void);

	void get_motion_setting(void);
	static void motion_enable_cb(keynode_t *node, void *data);
	static void sensitivity_change_cb(keynode_t *node, void *data);
	bool register_sensor_event(sensor_type_t sensor_type, bool is_register);
	bool operate_sensor(void);
	bool turn_off_sensor(void);
	static float get_scale_factor(int sensitivity);
	bool load_lib(void);
	void reset(void);
	void put_motion_event(unsigned int event_type, int value, vector<sensor_event_t> &outs);
	void put_tilt_event(unsigned int event_type, float scale_factor, vector<sensor_event_t> &outs);
	void get_motion(float a_x, float a_y, float a_z, float g_x, float g_y, float g_z, int proxi, vector<sensor_event_t> &outs);

	virtual bool get_properties(sensor_properties_t &properties);

	bool is_active_event(unsigned int type);
	bool is_sensor_used(int sensor);
	void set_motion_enable_state(unsigned int type, bool enable);

	sensor_base *m_accel_sensor;
	sensor_base *m_gyro_sensor;
	sensor_base *m_proxi_sensor;

	bool m_enabled;

	int m_g_x;
	int m_g_y;
	int m_g_z;
	int m_proxi;

	bool m_accel_sensor_started;
	bool m_gyro_sensor_started;
	bool m_proxi_sensor_started;

	bool m_motion_activation_state;

	float m_tilt_scale_factor;
	float m_panning_scale_factor;
	float m_panning_browse_scale_factor;

	unsigned long long m_prev_acc_time;
	unsigned long long m_prev_gyro_time;

	motion_info_map m_motion_info;

	bool m_smart_relay_running;
	cmutex m_smart_relay_mutex;

	void *m_handle;

	ME_init_t ME_init;
	ME_fini_t ME_fini;
	ME_get_gyro_t ME_get_gyro;
	ME_get_result_t ME_get_result;
	ME_get_panning_dx_t ME_get_panning_dx;
	ME_get_panning_dy_t ME_get_panning_dy;
	ME_get_panning_dz_t ME_get_panning_dz;
	ME_clear_result_t ME_clear_result;
	ME_reset_angle_t ME_reset_angle;
};

#endif
