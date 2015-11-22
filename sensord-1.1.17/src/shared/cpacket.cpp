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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <new>
#include <common.h>
#include <cpacket.h>
#include <sf_common.h>

cpacket::cpacket()
{
	m_packet = NULL;
}

cpacket::cpacket(size_t size)
{
	m_packet = NULL;
	set_payload_size(size);
}

cpacket::~cpacket()
{
	delete[] (char*)m_packet;
}

void cpacket::set_cmd(int cmd)
{
	if (!m_packet)
		set_payload_size(0);

	m_packet->cmd = cmd;
}

int cpacket::cmd(void)
{
	if (!m_packet)
		return CMD_NONE;

	return m_packet->cmd;
}

void *cpacket::data(void)
{
	if (!m_packet)
		return NULL;

	return m_packet->data;
}

void *cpacket::packet(void)
{
	return (void*)m_packet;
}

size_t cpacket::size(void)
{
	if (!m_packet)
		return 0;

	return m_packet->size + sizeof(packet_header);
}

size_t cpacket::payload_size(void)
{
	if (!m_packet)
		return 0;

	return m_packet->size;
}

void cpacket::set_payload_size(size_t size)
{
	int prev_cmd = CMD_NONE;

	if (m_packet) {
		prev_cmd = m_packet->cmd;
		delete []m_packet;
	}

	m_packet = (packet_header*) new(std::nothrow) char[size + sizeof(packet_header)];
	retm_if (!m_packet, "Failed to allocate memory");
	m_packet->size = size;

	if (prev_cmd != CMD_NONE)
		m_packet->cmd = prev_cmd;
}
