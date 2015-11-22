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

#include <systemd/sd-daemon.h>
#include <server.h>
#include <sensor_plugin_loader.h>
#include <command_worker.h>
#include <thread>

using std::thread;

server::server()
: m_mainloop(NULL)
{

}

server::~server()
{
	stop();
}

int server::get_systemd_socket(const char *name)
{
	int type = SOCK_STREAM;
	const int listening = 1;
	size_t length = 0;
	int fd = -1;

	if (sd_listen_fds(1) != 1)
		return -1;

	fd = SD_LISTEN_FDS_START + 0;

	if (sd_is_socket_unix(fd, type, listening, name, length) > 0)
		return fd;

	return -1;
}

void server::accept_client(void)
{
	command_worker *cmd_worker;

	INFO("Client acceptor is started");

	while (true) {
		csocket client_command_socket;

		if (!m_client_accep_socket.accept(client_command_socket)) {
			ERR("Failed to accept connection request from a client");
			continue;
		}

		DBG("New client (socket_fd : %d) connected", client_command_socket.get_socket_fd());

		cmd_worker = new(std::nothrow) command_worker(client_command_socket);

		if (!cmd_worker) {
			ERR("Failed to allocate memory");
			continue;
		}

		if(!cmd_worker->start())
			delete cmd_worker;
	}
}

void server::run(void)
{
	int sock_fd = -1;
	const int MAX_PENDING_CONNECTION = 5;

	m_mainloop = g_main_loop_new(NULL, false);

	sock_fd = get_systemd_socket(COMMAND_CHANNEL_PATH);

	if (sock_fd >= 0) {
		INFO("Succeeded to get systemd socket(%d)", sock_fd);
		m_client_accep_socket = csocket(sock_fd);
	} else {
		ERR("Failed to get systemd socket, create it by myself!");
		if (!m_client_accep_socket.create(SOCK_STREAM)) {
			ERR("Failed to create command channel");
			return;
		}

		if(!m_client_accep_socket.bind(COMMAND_CHANNEL_PATH)) {
			ERR("Failed to bind command channel");
			m_client_accep_socket.close();
			return;
		}

		if(!m_client_accep_socket.listen(MAX_PENDING_CONNECTION)) {
			ERR("Failed to listen command channel");
			return;
		}
	}

	csensor_event_dispatcher::get_instance().run();

	thread client_accepter(&server::accept_client, this);
	client_accepter.detach();

	sd_notify(0, "READY=1");

	g_main_loop_run(m_mainloop);
	g_main_loop_unref(m_mainloop);

	return;
}

void server::stop(void)
{
	if(m_mainloop)
		g_main_loop_quit(m_mainloop);

	m_client_accep_socket.close();
}

server& server::get_instance()
{
	static server inst;
	return inst;
}
