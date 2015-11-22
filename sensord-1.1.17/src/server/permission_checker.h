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

#ifndef _PERMISSION_CHECKER_H_
#define _PERMISSION_CHECKER_H_

#include <string>
#include <vector>
#include <memory>

class permission_checker {
private:
	class permission_info {
		public:
		permission_info(int _permission, bool _need_to_check, std::string _name, std::string _access_right)
		: permission(_permission)
		, need_to_check(_need_to_check)
		, name(_name)
		, access_right(_access_right)
		{
		}
		int permission;
		bool need_to_check;
		std::string name;
		std::string access_right;
	};

	typedef std::vector<std::shared_ptr<permission_info>> permission_info_vector;

	typedef int (*security_server_check_privilege_by_sockfd_t)(int sockfd,
			  const char *object,
			  const char *access_rights);

	permission_checker();
	~permission_checker();
	permission_checker(permission_checker const&) {};
	permission_checker& operator=(permission_checker const&);

	bool init_security_lib(void);
	void init();

	security_server_check_privilege_by_sockfd_t security_server_check_privilege_by_sockfd;
	void *m_security_handle;

	permission_info_vector m_permission_infos;
	int m_permission_set;
public:
	static permission_checker& get_instance();

	int get_permission(int sock_fd);
};

#endif /* COMMAND_WORKER_H_ */
