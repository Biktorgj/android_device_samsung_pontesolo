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

#include <permission_checker.h>
#include <sf_common.h>
#include <common.h>
#include <sensor_plugin_loader.h>
#include <sensor_base.h>
#include <dlfcn.h>

#define SECURITY_LIB "/usr/lib/libsecurity-server-client.so.1"

permission_checker::permission_checker()
: m_permission_set(0)
, security_server_check_privilege_by_sockfd(NULL)
, m_security_handle(NULL)

{
	init();
}

permission_checker::~permission_checker()
{
	if (m_security_handle)
		dlclose(m_security_handle);
}

permission_checker& permission_checker::get_instance()
{
	static permission_checker inst;
	return inst;
}

bool permission_checker::init_security_lib(void)
{
	m_security_handle = dlopen(SECURITY_LIB, RTLD_LAZY);

	if (!m_security_handle) {
		ERR("dlopen(%s) error, cause: %s", SECURITY_LIB, dlerror());
		return false;
	}

	security_server_check_privilege_by_sockfd =
		(security_server_check_privilege_by_sockfd_t) dlsym(m_security_handle, "security_server_check_privilege_by_sockfd");

	if (!security_server_check_privilege_by_sockfd) {
		ERR("Failed to load symbol");
		dlclose(m_security_handle);
		m_security_handle = NULL;
		return false;
	}

	return true;

}

void permission_checker::init()
{
	m_permission_infos.push_back(std::make_shared<permission_info> (SENSOR_PERMISSION_STANDARD, false, "", ""));
	m_permission_infos.push_back(std::make_shared<permission_info> (SENSOR_PERMISSION_BIO, true, "sensord::bio", "rw"));

	vector<sensor_base *> sensors;
	sensors = sensor_plugin_loader::get_instance().get_sensors(ALL_SENSOR);

	for (int i = 0; i < sensors.size(); ++i)
		m_permission_set |= sensors[i]->get_permission();

	INFO("Permission Set = %d", m_permission_set);

	if (!init_security_lib())
		ERR("Failed to init security lib: %s", SECURITY_LIB);
}

int permission_checker::get_permission(int sock_fd)
{
	int permission = SENSOR_PERMISSION_NONE;

	for (int i = 0; i < m_permission_infos.size(); ++i) {
		if (!m_permission_infos[i]->need_to_check) {
			permission |= m_permission_infos[i]->permission;
		} else if ((m_permission_set & m_permission_infos[i]->permission) && security_server_check_privilege_by_sockfd) {
			if (security_server_check_privilege_by_sockfd(sock_fd, m_permission_infos[i]->name.c_str(), m_permission_infos[i]->access_right.c_str()) == 1) {
				permission |= m_permission_infos[i]->permission;
			}
		}
	}

	return permission;
}
