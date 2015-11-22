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

#ifndef CSOCKET_H_
#define CSOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "common.h"
#include <string>
using std::string;

class csocket {
public:
	csocket();
	virtual ~csocket();
	csocket(int sock_fd);
	csocket(const csocket &sock);

	//Server
	bool create(int sock_type);
	bool bind (const char *sock_path);
	bool listen(const int max_connections);
	bool accept(csocket& client_socket) const;

	//Client
	bool connect(const char *sock_path);

	//Data Transfer
	ssize_t send(void const* buffer, size_t size) const;
	ssize_t recv(void* buffer, size_t size) const;

	bool set_connection_mode(void);
	bool set_transfer_mode(void);
	bool is_blocking_mode(void);

	//check if socket is created
	bool is_valid(void) const;
	int get_socket_fd(void) const;

	bool close(void);

	bool is_block_mode(void);

private:
	bool set_blocking_mode(bool blocking);
	bool set_sock_type(void);

	ssize_t send_for_seqpacket(void const* buffer, size_t size) const;
	ssize_t send_for_stream(void const* buffer, size_t size) const;
	ssize_t recv_for_seqpacket(void* buffer, size_t size) const;
	ssize_t recv_for_stream(void* buffer, size_t size) const;

	int m_sock_fd;
	int m_sock_type;
	sockaddr_un m_addr;
	int m_send_flags;
	int m_recv_flags;
};

#endif /* CSOCKET_H_ */
