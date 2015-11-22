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

#include <csensor_config.h>
#include "common.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <sstream>
#include <iostream>
#include <fstream>

using std::ifstream;

#define ROOT_ELEMENT	"SENSOR"
#define TEXT_ELEMENT 	"text"
#define MODEL_ID_ATTR 	"id"
#define DEFAULT_ATTR	"value"

csensor_config::csensor_config()
{
}

csensor_config& csensor_config::get_instance(void)
{
	static bool load_done = false;
	static csensor_config inst;

	if (!load_done) {
		inst.load_config(SENSOR_CONFIG_FILE_PATH);
		inst.get_device_id();
		if (!inst.m_device_id.empty())
			INFO("Device ID = %s", inst.m_device_id.c_str());
		else
			ERR("Failed to get Device ID");
		load_done = true;
	}

	return inst;
}

bool csensor_config::load_config(const string& config_path)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	DBG("csensor_config::load_config(\"%s\") is called!\n",config_path.c_str());

	doc = xmlParseFile(config_path.c_str());

	if (doc == NULL) {
		ERR("There is no %s\n",config_path.c_str());
		return false;
	}

	cur = xmlDocGetRootElement(doc);
	if(cur == NULL) {
		ERR("There is no root element in %s\n",config_path.c_str());
		xmlFreeDoc(doc);
		return false;
	}

	if(xmlStrcmp(cur->name, (const xmlChar *)ROOT_ELEMENT)) {
		ERR("Wrong type document: there is no [%s] root element in %s\n",ROOT_ELEMENT,config_path.c_str());
		xmlFreeDoc(doc);
		return false;
	}

	xmlNodePtr model_list_node_ptr;
	xmlNodePtr model_node_ptr;
	xmlNodePtr element_node_ptr;
	xmlAttrPtr attr_ptr;
	char* prop = NULL;

	model_list_node_ptr = cur->xmlChildrenNode;

	while (model_list_node_ptr != NULL) {
		//skip garbage element, [text]
		if (!xmlStrcmp(model_list_node_ptr->name,(const xmlChar *)TEXT_ELEMENT)) {
			model_list_node_ptr = model_list_node_ptr->next;
			continue;
		}

		//insert Model_list to config map
		m_sensor_config[(const char*)model_list_node_ptr->name];
		DBG("<%s>\n",(const char*)model_list_node_ptr->name);

		model_node_ptr = model_list_node_ptr->xmlChildrenNode;
		while (model_node_ptr != NULL){
			//skip garbage element, [text]
			if (!xmlStrcmp(model_node_ptr->name,(const xmlChar *)TEXT_ELEMENT)) {
				model_node_ptr = model_node_ptr->next;
				continue;
			}


			string model_id;
			prop = (char*)xmlGetProp(model_node_ptr,(const xmlChar*)MODEL_ID_ATTR);
			model_id = prop;
			free(prop);

			//insert Model to Model_list
			m_sensor_config[(const char*)model_list_node_ptr->name][model_id];
			DBG("<%s id=\"%s\">\n",(const char*)model_list_node_ptr->name,model_id.c_str());

			element_node_ptr = model_node_ptr->xmlChildrenNode;
			while (element_node_ptr != NULL) {
				//skip garbage element, [text]
				if (!xmlStrcmp(element_node_ptr->name,(const xmlChar *)TEXT_ELEMENT)) {
					element_node_ptr = element_node_ptr->next;
					continue;
				}

				//insert Element to Model
				m_sensor_config[(const char*)model_list_node_ptr->name][model_id][(const char*)element_node_ptr->name];
				DBG("<%s id=\"%s\"><%s>\n",(const char*)model_list_node_ptr->name,model_id.c_str(),(const char*)element_node_ptr->name);

				attr_ptr = element_node_ptr->properties;
				while (attr_ptr != NULL) {

					string key,value;
					key = (char*)attr_ptr->name;
					prop = (char*)xmlGetProp(element_node_ptr,attr_ptr->name);
					value = prop;
					free(prop);

					//insert attribute to Element
					m_sensor_config[(const char*)model_list_node_ptr->name][model_id][(const char*)element_node_ptr->name][key]=value;
					DBG("<%s id=\"%s\"><%s \"%s\"=\"%s\">\n",(const char*)model_list_node_ptr->name,model_id.c_str(),(const char*)element_node_ptr->name,key.c_str(),value.c_str());
					attr_ptr = attr_ptr->next;
				}


				element_node_ptr = element_node_ptr->next;
			}

			DBG("\n");
			model_node_ptr = model_node_ptr->next;
		}

		DBG("\n");
		model_list_node_ptr = model_list_node_ptr->next;
	}

	xmlFreeDoc(doc);
	return true;
}


bool csensor_config::get(const string& sensor_type,const string& model_id, const string& element, const string& attr, string& value)
{
	auto it_model_list = m_sensor_config.find(sensor_type);

	if (it_model_list == m_sensor_config.end())	{
		ERR("There is no <%s> element\n",sensor_type.c_str());
		return false;
	}

	auto it_model = it_model_list->second.find(model_id);

	if (it_model == it_model_list->second.end()) {
		ERR("There is no <%s id=\"%s\"> element\n",sensor_type.c_str(),model_id.c_str());
		return false;
	}

	auto it_element = it_model->second.find(element);

	if (it_element == it_model->second.end()) {
		DBG("There is no <%s id=\"%s\"><%s> element\n",sensor_type.c_str(),model_id.c_str(),element.c_str());
		return false;
	}

	auto it_attr = it_element->second.find(attr);

	if (it_attr == it_element->second.end()) {
		DBG("There is no <%s id=\"%s\"><%s \"%s\">\n",sensor_type.c_str(),model_id.c_str(),element.c_str(),attr.c_str());
		return false;
	}

	value = it_attr->second;

	return true;
}

bool csensor_config::get(const string& sensor_type, const string& model_id, const string& element, const string& attr, double& value)
{
	string str_value;

	if (get(sensor_type,model_id,element,attr,str_value) == false)
		return false;

	istringstream convert(str_value);

	if ( !(convert >> value))
		value = 0;

	return true;
}

bool csensor_config::get(const string& sensor_type, const string& model_id, const string& element, const string& attr, long& value)
{
	string str_value;

	if (get(sensor_type,model_id,element,attr,str_value) == false)
		return false;

	istringstream convert(str_value);

	if ( !(convert >> value))
		value = 0;

	return true;
}

bool csensor_config::get(const string& sensor_type, const string& model_id, const string& element, string& value)
{
	if (get(sensor_type, model_id, element, m_device_id, value))
		return true;

	if (get(sensor_type, model_id, element, DEFAULT_ATTR, value))
		return true;

	return false;
}

bool csensor_config::get(const string& sensor_type, const string& model_id, const string& element, double& value)
{
	if (get(sensor_type, model_id, element, m_device_id, value))
		return true;

	if (get(sensor_type, model_id, element, DEFAULT_ATTR, value))
		return true;

	return false;
}

bool csensor_config::get(const string& sensor_type, const string& model_id, const string& element, long& value)
{
	if (get(sensor_type, model_id, element, m_device_id, value))
		return true;

	if (get(sensor_type, model_id, element, DEFAULT_ATTR, value))
		return true;

	return false;
}

bool csensor_config::is_supported(const string& sensor_type,const string& model_id)
{
	auto it_model_list = m_sensor_config.find(sensor_type);

	if (it_model_list == m_sensor_config.end())
		return false;

	auto it_model = it_model_list->second.find(model_id);

	if (it_model == it_model_list->second.end())
		return false;

	return true;
}
