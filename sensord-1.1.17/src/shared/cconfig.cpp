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

#include <cconfig.h>
#include <fstream>

using std::ifstream;

cconfig::cconfig(void)
{

}

cconfig::~cconfig(void)
{

}

bool cconfig::get_device_id(void)
{
	const string INFO_INI_PATH = "/etc/info.ini";
	const string START_DELIMETER = "Model=";
	const string END_DELIMETER = ";";
	string line;
	ifstream in_file;
	std::size_t start_pos, end_pos;
	bool ret = false;

	in_file.open(INFO_INI_PATH);

	if (!in_file.is_open())
		return false;

	while (!in_file.eof()) {
		getline(in_file, line);
		start_pos = line.find(START_DELIMETER);

		if (start_pos != std::string::npos) {
			start_pos = start_pos + START_DELIMETER.size();
			end_pos = line.find(END_DELIMETER, start_pos);

			if (end_pos != std::string::npos) {
				m_device_id = line.substr(start_pos, end_pos - start_pos);
				ret = true;
				break;
			}
		}
	}

	in_file.close();

	return ret;
}
