/*
 * libsensord-share
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
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

#include <sensor_plugin_loader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <sensor_hal.h>
#include <sensor_base.h>
#include <sensor_fusion.h>

#include <dlfcn.h>
#include <dirent.h>
#include <common.h>
#include <unordered_set>
#include <algorithm>

using std::make_pair;
using std::equal;
using std::unordered_set;

#define ROOT_ELEMENT "PLUGIN"
#define TEXT_ELEMENT "text"
#define PATH_ATTR "path"

#define HAL_ELEMENT "HAL"
#define SENSOR_ELEMENT "SENSOR"

#define PLUGINS_CONFIG_PATH "/usr/etc/sensor_plugins.xml"
#define PLUGINS_DIR_PATH "/usr/lib/sensord"

#define SENSOR_INDEX_SHIFT 16

sensor_plugin_loader::sensor_plugin_loader()
{
}

sensor_plugin_loader& sensor_plugin_loader::get_instance()
{
	static sensor_plugin_loader inst;
	return inst;
}

bool sensor_plugin_loader::load_module(const string &path, vector<void*> &sensors, void* &handle)
{
	void *_handle = dlopen(path.c_str(), RTLD_NOW);

	if (!_handle) {
		ERR("Failed to dlopen(%s), dlerror : %s", path.c_str(), dlerror());
		return false;
	}

	dlerror();

	create_t create_module = (create_t) dlsym(_handle, "create");

	if (!create_module) {
		ERR("Failed to find symbols in %s", path.c_str());
		dlclose(_handle);
		return false;
	}

	sensor_module *module = create_module();

	if (!module) {
		ERR("Failed to create module, path is %s\n", path.c_str());
		dlclose(_handle);
		return false;
	}

	sensors.clear();
	sensors.swap(module->sensors);

	delete module;
	handle = _handle;

	return true;
}

bool sensor_plugin_loader::insert_module(plugin_type type, const string &path)
{
	if (type == PLUGIN_TYPE_HAL) {
		DBG("Insert HAL plugin [%s]", path.c_str());

		std::vector<void *>hals;
		void *handle;

		if (!load_module(path, hals, handle))
			return false;

		sensor_hal* hal;

		for (int i = 0; i < hals.size(); ++i) {
			hal = static_cast<sensor_hal*> (hals[i]);
			sensor_type_t sensor_type = hal->get_type();
			m_sensor_hals.insert(make_pair(sensor_type, hal));
		}
	} else if (type == PLUGIN_TYPE_SENSOR) {
		DBG("Insert Sensor plugin [%s]", path.c_str());

		std::vector<void *> sensors;
		void *handle;

		if (!load_module(path, sensors, handle))
			return false;

		sensor_base* sensor;

		for (int i = 0; i < sensors.size(); ++i) {
			sensor = static_cast<sensor_base*> (sensors[i]);

			if (!sensor->init()) {
				ERR("Failed to init [%s] module\n", sensor->get_name());
				delete sensor;
				continue;
			}

			DBG("init [%s] module", sensor->get_name());

			sensor_type_t sensor_type = sensor->get_type();

			if (sensor->is_fusion())
				m_fusions.push_back((sensor_fusion*) sensor);
			else {
				int idx;
				idx = m_sensors.count(sensor_type);
				sensor->set_id(idx << SENSOR_INDEX_SHIFT | sensor_type);
				m_sensors.insert(make_pair(sensor_type, sensor));
			}
		}
	}else {
		ERR("Not supported type: %d", type);
		return false;
	}

	return true;
}

bool sensor_plugin_loader::load_plugins(void)
{
	vector<string> hal_paths, sensor_paths;
	vector<string> unique_hal_paths, unique_sensor_paths;

	get_paths_from_config(string(PLUGINS_CONFIG_PATH), hal_paths, sensor_paths);
	get_paths_from_dir(string(PLUGINS_DIR_PATH), hal_paths, sensor_paths);

	//remove duplicates while keeping the original ordering => unique_*_paths
	unordered_set<string> s;
	auto unique = [&s](vector<string> &paths, const string &path) {
		if (s.insert(path).second)
			paths.push_back(path);
	};

	for_each(hal_paths.begin(), hal_paths.end(),
		[&](const string &path) {
			unique(unique_hal_paths, path);
		}
	);

	for_each(sensor_paths.begin(), sensor_paths.end(),
		[&](const string &path) {
			unique(unique_sensor_paths, path);
		}
	);

	//load plugins specified by unique_*_paths
	auto insert = [&](plugin_type type, const string &path) {
			insert_module(type, path);
	};

	for_each(unique_hal_paths.begin(), unique_hal_paths.end(),
		[&](const string &path) {
			insert(PLUGIN_TYPE_HAL, path);
		}
	);

	for_each(unique_sensor_paths.begin(), unique_sensor_paths.end(),
		[&](const string &path) {
			insert(PLUGIN_TYPE_SENSOR, path);
		}
	);

	show_sensor_info();
	return true;
}

void sensor_plugin_loader::show_sensor_info(void)
{
	INFO("========== Loaded sensor information ==========\n");

	int index = 0;

	auto it = m_sensors.begin();

	while (it != m_sensors.end()) {
		sensor_base* sensor = it->second;

		sensor_info info;
		sensor->get_sensor_info(info);
		INFO("No:%d [%s]\n", ++index, sensor->get_name());
		info.show();
		it++;
	}

	INFO("===============================================\n");
}


bool sensor_plugin_loader::get_paths_from_dir(const string &dir_path, vector<string> &hal_paths, vector<string> &sensor_paths)
{
	const string PLUGIN_POSTFIX = ".so";
	const string HAL_POSTFIX = "_hal.so";

	DIR *dir = NULL;
	struct dirent *dir_entry = NULL;

	dir = opendir(dir_path.c_str());

	if (!dir) {
		ERR("Failed to open dir: %s", dir_path.c_str());
		return false;
	}

	string name;

	while (dir_entry = readdir(dir)) {
		name = string(dir_entry->d_name);

		if (equal(PLUGIN_POSTFIX.rbegin(), PLUGIN_POSTFIX.rend(), name.rbegin())) {
			if (equal(HAL_POSTFIX.rbegin(), HAL_POSTFIX.rend(), name.rbegin()))
				hal_paths.push_back(dir_path + "/" + name);
			else
				sensor_paths.push_back(dir_path + "/" + name);
		}
	}

	closedir(dir);
	return true;
}

bool sensor_plugin_loader::get_paths_from_config(const string &config_path, vector<string> &hal_paths, vector<string> &sensor_paths)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(config_path.c_str());

	if (doc == NULL) {
		ERR("There is no %s\n", config_path.c_str());
		return false;
	}

	cur = xmlDocGetRootElement(doc);

	if (cur == NULL) {
		ERR("There is no root element in %s\n", config_path.c_str());
		xmlFreeDoc(doc);
		return false;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)ROOT_ELEMENT)) {
		ERR("Wrong type document: there is no [%s] root element in %s\n", ROOT_ELEMENT, config_path.c_str());
		xmlFreeDoc(doc);
		return false;
	}

	xmlNodePtr plugin_list_node_ptr;
	xmlNodePtr module_node_ptr;
	char* prop = NULL;
	string path, category;

	plugin_list_node_ptr = cur->xmlChildrenNode;

	while (plugin_list_node_ptr != NULL) {
		//skip garbage element, [text]
		if (!xmlStrcmp(plugin_list_node_ptr->name, (const xmlChar *)TEXT_ELEMENT)) {
			plugin_list_node_ptr = plugin_list_node_ptr->next;
			continue;
		}

		DBG("<%s>\n", (const char*)plugin_list_node_ptr->name);

		module_node_ptr = plugin_list_node_ptr->xmlChildrenNode;
		while (module_node_ptr != NULL) {
			if (!xmlStrcmp(module_node_ptr->name, (const xmlChar *)TEXT_ELEMENT)) {
				module_node_ptr = module_node_ptr->next;
				continue;
			}

			prop = (char*)xmlGetProp(module_node_ptr, (const xmlChar*)PATH_ATTR);
			path = prop;
			free(prop);

			DBG("<%s path=\"%s\">\n", (const char*) module_node_ptr->name, path.c_str());

			category = (const char*) plugin_list_node_ptr->name;

			if (category == string(HAL_ELEMENT))
				hal_paths.push_back(path);
			else if (category == string(SENSOR_ELEMENT))
				sensor_paths.push_back(path);

			DBG("\n");
			module_node_ptr = module_node_ptr->next;
		}

		DBG("\n");
		plugin_list_node_ptr = plugin_list_node_ptr->next;
	}

	xmlFreeDoc(doc);

	return true;

}

sensor_hal* sensor_plugin_loader::get_sensor_hal(sensor_type_t type)
{
	auto it_plugins = m_sensor_hals.find(type);

	if (it_plugins == m_sensor_hals.end())
		return NULL;

	return it_plugins->second;
}

vector<sensor_hal *> sensor_plugin_loader::get_sensor_hals(sensor_type_t type)
{
	vector<sensor_hal *> sensor_hal_list;
	pair<sensor_hal_plugins::iterator, sensor_hal_plugins::iterator> ret;
	ret = m_sensor_hals.equal_range(type);

	for (auto it = ret.first; it != ret.second; ++it)
		sensor_hal_list.push_back(it->second);

	return sensor_hal_list;
}

sensor_base* sensor_plugin_loader::get_sensor(sensor_type_t type)
{
	auto it_plugins = m_sensors.find(type);

	if (it_plugins == m_sensors.end())
		return NULL;

	return it_plugins->second;
}

vector<sensor_base *> sensor_plugin_loader::get_sensors(sensor_type_t type)
{
	vector<sensor_base *> sensor_list;
	pair<sensor_plugins::iterator, sensor_plugins::iterator> ret;

	if (type == ALL_SENSOR)
		ret = std::make_pair(m_sensors.begin(), m_sensors.end());
	else
		ret = m_sensors.equal_range(type);

	for (auto it = ret.first; it != ret.second; ++it)
		sensor_list.push_back(it->second);

	return sensor_list;
}


sensor_base* sensor_plugin_loader::get_sensor(sensor_id_t id)
{
	const int SENSOR_TYPE_MASK = 0x0000FFFF;
	vector<sensor_base *> sensors;

	sensor_type_t type = (sensor_type_t) (id & SENSOR_TYPE_MASK);
	int index = id >> SENSOR_INDEX_SHIFT;

	sensors = get_sensors(type);

	if (sensors.size() <= index)
		return NULL;

	return sensors[index];
}


vector<sensor_base *> sensor_plugin_loader::get_virtual_sensors(void)
{
	vector<sensor_base *> virtual_list;
	sensor_base* module;

	for (auto sensor_it = m_sensors.begin(); sensor_it != m_sensors.end(); ++sensor_it) {
		module = sensor_it->second;

		if (module && module->is_virtual() == true) {
			virtual_list.push_back(module);
		}
	}

	return virtual_list;
}

sensor_fusion* sensor_plugin_loader::get_fusion(void)
{
	if (m_fusions.empty())
		return NULL;

	return m_fusions.front();
}

vector<sensor_fusion *> sensor_plugin_loader::get_fusions(void)
{
	return m_fusions;
}

bool sensor_plugin_loader::destroy()
{
	sensor_base* sensor;

	for (auto sensor_it = m_sensors.begin(); sensor_it != m_sensors.end(); ++sensor_it) {
		sensor = sensor_it->second;

		//need to dlclose
		//unregister_module(module);

		delete sensor;
	}

	sensor_hal* sensor_hal;

	for (auto sensor_hal_it = m_sensor_hals.begin(); sensor_hal_it != m_sensor_hals.end(); ++sensor_hal_it) {
		sensor_hal = sensor_hal_it->second;

		// need to dlclose
		//unregister_module(module);

		delete sensor_hal;
	}

	m_sensors.clear();
	m_sensor_hals.clear();

	return true;
}

