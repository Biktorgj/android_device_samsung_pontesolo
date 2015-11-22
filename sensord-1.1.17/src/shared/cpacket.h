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

#if !defined(_CPACKET_CLASS_H_)
#define _CPACKET_CLASS_H_

typedef struct packet_header {
	int cmd;
	size_t size;
	char data[];
} packet_header;

class cpacket
{
public:
	cpacket();
	explicit cpacket(size_t size);
	~cpacket();

	void set_cmd(int cmd);
	int cmd(void);

	void *data(void);
	void *packet(void);

	size_t size(void);
	size_t payload_size(void);

	void set_payload_size(size_t size);
private:
	packet_header *m_packet;
};

#endif
// End of a file
