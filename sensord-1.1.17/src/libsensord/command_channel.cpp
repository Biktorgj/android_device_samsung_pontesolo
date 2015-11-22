/*
 * libsensord
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

#include <command_channel.h>
#include <client_common.h>
#include <sf_common.h>
#include <sensor_info.h>
#include <sensor_info_manager.h>

command_channel::command_channel()
: m_client_id(CLIENT_ID_INVALID)
, m_sensor_id(UNKNOWN_SENSOR)
{

}
command_channel::~command_channel()
{
	if (m_command_socket.is_valid())
		m_command_socket.close();
}

bool command_channel::command_handler(cpacket *packet, void **return_payload)
{
	if (!m_command_socket.is_valid()) {
		ERR("Command socket(%d) is not valid for client %s", m_command_socket.get_socket_fd(), get_client_name());
		return false;
	}

	if (packet->size() == 0) {
		ERR("Packet is not valid for client %s", get_client_name());
		return false;
	}

	if (m_command_socket.send(packet->packet(), packet->size()) <= 0) {
		m_command_socket.close();
		ERR("Failed to send command in client %s", get_client_name());
		return false;
	}

	packet_header header;

	if (m_command_socket.recv(&header, sizeof(header)) <= 0) {
		m_command_socket.close();
		ERR("Failed to receive header for reply packet in client %s", get_client_name());
		return false;
	}

	char *buffer = new(std::nothrow) char[header.size];
	retvm_if(!buffer, false, "Failed to allocate memory");

	if (m_command_socket.recv(buffer, header.size) <= 0) {
		m_command_socket.close();
		ERR("Failed to receive reply packet in client %s", get_client_name());
		delete[] buffer;
		return false;
	}

	*return_payload = buffer;

	return true;
}

bool command_channel::create_channel(void)
{
	if (!m_command_socket.create(SOCK_STREAM))
		return false;

	if (!m_command_socket.connect(COMMAND_CHANNEL_PATH)) {
		ERR("Failed to connect command channel for client %s, command socket fd[%d]", get_client_name(), m_command_socket.get_socket_fd());
		return false;
	}

	m_command_socket.set_connection_mode();

	return true;
}

void command_channel::set_client_id(int client_id)
{
	m_client_id = client_id;
}

bool command_channel::cmd_get_id(int &client_id)
{
	cpacket *packet;
	cmd_get_id_t *cmd_get_id;
	cmd_get_id_done_t *cmd_get_id_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_get_id_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_GET_ID);

	cmd_get_id = (cmd_get_id_t *)packet->data();
	cmd_get_id->pid = getpid();

	INFO("%s send cmd_get_id()", get_client_name());

	if (!command_handler(packet, (void **)&cmd_get_id_done)) {
		ERR("Client %s failed to send/receive command", get_client_name());
		delete packet;
		return false;
	}

	if (cmd_get_id_done->client_id < 0) {
		ERR("Client %s failed to get client_id[%d] from server",
			get_client_name(), cmd_get_id_done->client_id);
		delete[] (char *)cmd_get_id_done;
		delete packet;
		return false;
	}

	client_id = cmd_get_id_done->client_id;

	delete[] (char *)cmd_get_id_done;
	delete packet;

	return true;
}

bool command_channel::cmd_get_sensor_list(void)
{
	cpacket packet;
	cmd_get_sensor_list_done_t *cmd_get_sensor_list_done;

	packet.set_payload_size(sizeof(cmd_get_sensor_list_t));
	packet.set_cmd(CMD_GET_SENSOR_LIST);

	INFO("%s send cmd_get_sensor_list", get_client_name());

	if (!command_handler(&packet, (void **)&cmd_get_sensor_list_done)) {
		ERR("Client %s failed to send/receive command", get_client_name());
		return false;
	}

	int sensor_cnt;
	const size_t *size_field;
	const char *raw_data_field;

	sensor_cnt = cmd_get_sensor_list_done->sensor_cnt;
	size_field = (const size_t *)cmd_get_sensor_list_done->data;
	raw_data_field = (const char *)size_field + (sizeof(size_t) * sensor_cnt);

	sensor_info *info;

	int idx = 0;
	for (int i = 0; i < sensor_cnt; ++i) {
		info = new(std::nothrow) sensor_info;

		if (!info) {
			ERR("Failed to allocate memory");
			delete[] (char *)cmd_get_sensor_list_done;
			return false;
		}

		info->set_raw_data(raw_data_field + idx, size_field[i]);
		sensor_info_manager::get_instance().add_info(info);
		idx += size_field[i];
	}

	delete[] (char *)cmd_get_sensor_list_done;
	return true;
}


bool command_channel::cmd_hello(sensor_id_t sensor)
{
	cpacket *packet;
	cmd_hello_t *cmd_hello;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_hello_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_HELLO);

	cmd_hello = (cmd_hello_t*)packet->data();
	cmd_hello->client_id = m_client_id;
	cmd_hello->sensor = sensor;

	INFO("%s send cmd_hello(client_id=%d, %s)",
		get_client_name(), m_client_id, get_sensor_name(sensor));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s]",
			get_client_name(), get_sensor_name(sensor));
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("client %s got error[%d] from server with sensor [%s]",
			get_client_name(), cmd_done->value, get_sensor_name(sensor));

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	m_sensor_id = sensor;

	return true;
}

bool command_channel::cmd_byebye(void)
{
	cpacket *packet;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_byebye_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_BYEBYE);

	INFO("%s send cmd_byebye(client_id=%d, %s)",
		get_client_name(), m_client_id, get_sensor_name(m_sensor_id));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d]",
			get_client_name(), get_sensor_name(m_sensor_id), m_client_id);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d]",
			get_client_name(), cmd_done->value, get_sensor_name(m_sensor_id), m_client_id);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	if (m_command_socket.is_valid())
		m_command_socket.close();


	m_client_id = CLIENT_ID_INVALID;
	m_sensor_id = UNKNOWN_SENSOR;

	return true;
}

bool command_channel::cmd_start(void)
{
	cpacket *packet;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_start_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_START);

	INFO("%s send cmd_start(client_id=%d, %s)",
		get_client_name(), m_client_id, get_sensor_name(m_sensor_id));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d]",
			get_client_name(), get_sensor_name(m_sensor_id), m_client_id);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d]",
			get_client_name(), cmd_done->value, get_sensor_name(m_sensor_id), m_client_id);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_stop(void)
{
	cpacket *packet;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_stop_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_STOP);

	INFO("%s send cmd_stop(client_id=%d, %s)",
		get_client_name(), m_client_id, get_sensor_name(m_sensor_id));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d]",
			get_client_name(), get_sensor_name(m_sensor_id), m_client_id);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d]",
			get_client_name(), cmd_done->value, get_sensor_name(m_sensor_id), m_client_id);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_set_option(int option)
{
	cpacket *packet;
	cmd_set_option_t *cmd_set_option;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_set_option_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_SET_OPTION);

	cmd_set_option = (cmd_set_option_t*)packet->data();
	cmd_set_option->option = option;

	INFO("%s send cmd_set_option(client_id=%d, %s, option=%d)",
		get_client_name(), m_client_id, get_sensor_name(m_sensor_id), option);

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d], option[%d]",
			get_client_name(), get_sensor_name(m_sensor_id), m_client_id, option);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d], option[%d]",
			get_client_name(), cmd_done->value, get_sensor_name(m_sensor_id), m_client_id, option);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_register_event(unsigned int event_type)
{
	cpacket *packet;
	cmd_reg_t *cmd_reg;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_reg_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_REG);

	cmd_reg = (cmd_reg_t*)packet->data();
	cmd_reg->event_type = event_type;

	INFO("%s send cmd_register_event(client_id=%d, %s)",
		get_client_name(), m_client_id, get_event_name(event_type));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command with client_id [%d], event_type[%s]",
			get_client_name(), m_client_id, get_event_name(event_type));
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server with client_id [%d], event_type[%s]",
			get_client_name(), cmd_done->value, m_client_id, get_event_name(event_type));

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}


bool command_channel::cmd_register_events(event_type_vector &event_vec)
{
	auto it_event = event_vec.begin();

	while (it_event != event_vec.end()) {
		if(!cmd_register_event(*it_event))
			return false;

		++it_event;
	}

	return true;
}

bool command_channel::cmd_unregister_event(unsigned int event_type)
{
	cpacket *packet;
	cmd_unreg_t *cmd_unreg;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_unreg_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_UNREG);

	cmd_unreg = (cmd_unreg_t*)packet->data();
	cmd_unreg->event_type = event_type;

	INFO("%s send cmd_unregister_event(client_id=%d, %s)",
		get_client_name(), m_client_id, get_event_name(event_type));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command with client_id [%d], event_type[%s]",
			get_client_name(), m_client_id, get_event_name(event_type));
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server with client_id [%d], event_type[%s]",
			get_client_name(), cmd_done->value, m_client_id, get_event_name(event_type));

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}


bool command_channel::cmd_unregister_events(event_type_vector &event_vec)
{
	auto it_event = event_vec.begin();

	while (it_event != event_vec.end()) {
		if(!cmd_unregister_event(*it_event))
			return false;

		++it_event;
	}

	return true;
}

bool command_channel::cmd_set_interval(unsigned int interval)
{
	cpacket *packet;
	cmd_set_interval_t *cmd_set_interval;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_set_interval_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_SET_INTERVAL);

	cmd_set_interval = (cmd_set_interval_t*)packet->data();
	cmd_set_interval->interval = interval;

	INFO("%s send cmd_set_interval(client_id=%d, %s, interval=%d)",
		get_client_name(), m_client_id, get_sensor_name(m_sensor_id), interval);

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("%s failed to send/receive command for sensor[%s] with client_id [%d], interval[%d]",
			get_client_name(), get_sensor_name(m_sensor_id), m_client_id, interval);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("%s got error[%d] from server for sensor[%s] with client_id [%d], interval[%d]",
			get_client_name(), cmd_done->value, get_sensor_name(m_sensor_id), m_client_id, interval);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_unset_interval(void)
{
	cpacket *packet;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_unset_interval_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_UNSET_INTERVAL);

	INFO("%s send cmd_unset_interval(client_id=%d, %s)",
		get_client_name(), m_client_id, get_sensor_name(m_sensor_id));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d]",
			get_client_name(), get_sensor_name(m_sensor_id), m_client_id);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d]",
			get_client_name(), cmd_done->value, get_sensor_name(m_sensor_id), m_client_id);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_set_command(unsigned int cmd, long value)
{
	cpacket *packet;
	cmd_set_command_t *cmd_set_command;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_set_command_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_SET_COMMAND);

	cmd_set_command = (cmd_set_command_t*)packet->data();
	cmd_set_command->cmd = cmd;
	cmd_set_command->value = value;


	INFO("%s send cmd_set_command(client_id=%d, %s, 0x%x, %ld)",
		get_client_name(), m_client_id, get_sensor_name(m_sensor_id), cmd, value);

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d], property[0x%x], value[%d]",
			get_client_name(), get_sensor_name(m_sensor_id), m_client_id, cmd, value);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with property[0x%x], value[%d]",
			get_client_name(), cmd_done->value, get_sensor_name(m_sensor_id), cmd, value);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_get_data(unsigned int type, sensor_data_t* sensor_data)
{
	cpacket *packet;
	cmd_get_data_t *cmd_get_data;
	cmd_get_data_done_t *cmd_get_data_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_get_data_done_t));
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_GET_DATA);

	cmd_get_data = (cmd_get_data_t*)packet->data();
	cmd_get_data->type = type;

	if (!command_handler(packet, (void **)&cmd_get_data_done)) {
		ERR("Client %s failed to send/receive command with client_id [%d], data_id[%s]",
			get_client_name(), m_client_id, get_data_name(type));
		delete packet;
		return false;
	}

	if (cmd_get_data_done->state < 0 ) {
		ERR("Client %s got error[%d] from server with client_id [%d], data_id[%s]",
			get_client_name(), cmd_get_data_done->state, m_client_id, get_data_name(type));
		sensor_data->accuracy = SENSOR_ACCURACY_UNDEFINED;
		sensor_data->timestamp = 0;
		sensor_data->value_count = 0;
		delete[] (char *)cmd_get_data_done;
		delete packet;
		return false;
	}

	sensor_data_t *base_data;
	base_data = &cmd_get_data_done->base_data;

	sensor_data->timestamp = base_data->timestamp;
	sensor_data->accuracy = base_data->accuracy;
	sensor_data->value_count = base_data->value_count;

	memcpy(sensor_data->values, base_data->values,
		sizeof(sensor_data->values[0]) * base_data->value_count);

	delete[] (char *)cmd_get_data_done;
	delete packet;

	return true;
}

bool command_channel::cmd_send_sensorhub_data(const char* buffer, int data_len)
{
	cpacket *packet;
	cmd_send_sensorhub_data_t *cmd_send_sensorhub_data;
	cmd_done_t *cmd_done;

	packet = new(std::nothrow) cpacket(sizeof(cmd_send_sensorhub_data_t) + data_len);
	retvm_if(!packet, false, "Failed to allocate memory");

	packet->set_cmd(CMD_SEND_SENSORHUB_DATA);

	cmd_send_sensorhub_data = (cmd_send_sensorhub_data_t*)packet->data();
	cmd_send_sensorhub_data->data_len = data_len;
	memcpy(cmd_send_sensorhub_data->data, buffer, data_len);


	INFO("%s send cmd_send_sensorhub_data(client_id=%d, data_len = %d, buffer = 0x%x)",
		get_client_name(), m_client_id, data_len, buffer);

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("%s failed to send/receive command with client_id [%d]",
			get_client_name(), m_client_id);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("%s got error[%d] from server with client_id [%d]",
			get_client_name(), cmd_done->value, m_client_id);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;


}
