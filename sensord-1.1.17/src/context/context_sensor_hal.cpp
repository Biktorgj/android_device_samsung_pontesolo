/*
 * context_sensor_hal
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
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

#include <linux/input.h>

#include <context_sensor_hal.h>
#include <sys/ioctl.h>
#include <fstream>

using std::ifstream;

#define SENSORHUB_ID_SENSOR 			(0)
#define SENSORHUB_ID_GESTURE			(1)

#define SENSORHUB_TYPE_CONTEXT	1
#define SENSORHUB_TYPE_GESTURE	2

#define EVENT_TYPE_CONTEXT_DATA		 REL_RX
#define EVENT_TYPE_LARGE_CONTEXT_DATA   REL_RY
#define EVENT_TYPE_CONTEXT_NOTI		 REL_RZ

#define SSPCONTEXT_DEVICE		"/dev/ssp_sensorhub"
#define SSP_INPUT_NODE_NAME			"ssp_context"
#define SENSORHUB_IOCTL_MAGIC		'S'
#define IOCTL_READ_LARGE_CONTEXT_DATA	_IOR(SENSORHUB_IOCTL_MAGIC, 3, char *)

#define INPUT_NAME	"context_sensor"

context_sensor_hal::context_sensor_hal()
: m_enabled(false)
{
	m_pending_data.version = sizeof(sensorhub_data_t);
	m_pending_data.sensorhub = SENSORHUB_ID_SENSOR;
	m_pending_data.type = SENSORHUB_TYPE_CONTEXT;
	memset(m_pending_data.hub_data, 0, sizeof(m_pending_data.hub_data));
	m_pending_data.hub_data_size = 0;

	m_node_handle = open_input_node(SSP_INPUT_NODE_NAME);

	if (m_node_handle < 0)
		throw ENXIO;

	if ((m_context_fd = open(SSPCONTEXT_DEVICE, O_RDWR)) < 0) {
		ERR("Open sensorhub device failed(%d)", m_context_fd);
		throw ENXIO;
	}

	INFO("m_context_fd = %s", SSPCONTEXT_DEVICE);

	INFO("context_sensor is created!\n");
}

context_sensor_hal::~context_sensor_hal()
{
	if (m_node_handle >= 0)
		close(m_node_handle);

	if (m_context_fd >= 0)
		close(m_context_fd);

	INFO("context_sensor is destroyed!\n");
}

int context_sensor_hal::open_input_node(const char* input_node)
{
	int fd = -1;
	const char *dirname = "/dev/input";
	char devname[PATH_MAX];
	char *filename;
	DIR *dir;
	struct dirent *de;

	INFO("======================start open_input_node=============================\n");

	dir = opendir(dirname);

	if (dir == NULL)
		return -1;

	strcpy(devname, dirname);

	filename = devname + strlen(devname);
	*filename++ = '/';

	while ((de = readdir(dir))) {
		if (de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
			(de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;

		strcpy(filename, de->d_name);
		fd = open(devname, O_RDONLY);

		if (fd >= 0) {
			char name[80];
			if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
				name[0] = '\0';
			}

			if (!strcmp(name, input_node)) {
				INFO("m_node_handle = %s", devname);
				break;
			} else {
				close(fd);
				fd = -1;
			}
		}
	}

	closedir(dir);

	if (fd < 0)
		ERR("couldn't find '%s' input device", input_node);

	return fd;
}

string context_sensor_hal::get_model_id(void)
{
	return SSP_INPUT_NODE_NAME;
}

sensor_type_t context_sensor_hal::get_type(void)
{
	return CONTEXT_SENSOR;
}

bool context_sensor_hal::enable(void)
{
	AUTOLOCK(m_mutex);

	m_enabled = true;

	INFO("Context sensor real starting");
	return true;
}

bool context_sensor_hal::disable(void)
{
	AUTOLOCK(m_mutex);

	m_enabled = false;

	INFO("Context sensor real stopping");
	return true;
}

bool context_sensor_hal::update_value(bool wait)
{
	const int INPUT_MAX_BEFORE_SYN = 10;
	struct input_event context_input;
	int read_input_cnt = 0;
	bool syn = false;
	bool event_updated = false;

	fd_set readfds,exceptfds;

	FD_ZERO(&readfds);
	FD_ZERO(&exceptfds);

	FD_SET(m_node_handle, &readfds);
	FD_SET(m_node_handle, &exceptfds);

	int ret;
	ret = select(m_node_handle+1, &readfds, NULL, &exceptfds, NULL);

	if (ret == -1) {
		ERR("select error:%s m_node_handle:d", strerror(errno), m_node_handle);
		return false;
	} else if (!ret) {
		DBG("select timeout");
		return false;
	}

	if (FD_ISSET(m_node_handle, &exceptfds)) {
		ERR("select exception occurred!");
		return false;
	}

	if (FD_ISSET(m_node_handle, &readfds)) {
		DBG("context event detection!");

		while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
			int input_len = read(m_node_handle, &context_input, sizeof(context_input));
			if (input_len != sizeof(context_input)) {
				ERR("context node read fail, read_len = %d", input_len);
				return false;
			}

			++read_input_cnt;

			int context_len = 0;
			if (context_input.type == EV_REL) {
				float value = context_input.value;

				if (context_input.code == EVENT_TYPE_CONTEXT_DATA) {
					DBG("EVENT_TYPE_CONTEXT_DATA, hub_data_size=%d", value);
					m_pending_data.hub_data_size = value;
					context_len = read_context_data();

					if (context_len == 0)
						DBG("No library data");
					else if (context_len < 0)
						ERR("read_context_data() err(%d)", context_len);

				} else if (context_input.code == EVENT_TYPE_LARGE_CONTEXT_DATA) {
					DBG("EVENT_TYPE_LARGE_CONTEXT_DATA, hub_data_size=%d", value);
					m_pending_data.hub_data_size = value;
					context_len = read_large_context_data();

					if (context_len == 0)
						DBG("No large library data");
					else if (context_len < 0)
						ERR("read_large_context_data() err(%d)", context_len);

				} else if (context_input.code == EVENT_TYPE_CONTEXT_NOTI) {
					DBG("EVENT_TYPE_CONTEXT_NOTI, value=%d", value);
					context_len = 3;
					m_pending_data.hub_data_size = context_len;
					m_pending_data.hub_data[0] = 0x02;
					m_pending_data.hub_data[1] = 0x01;
					m_pending_data.hub_data[2] = value;
					print_context_data(__FUNCTION__, m_pending_data.hub_data, m_pending_data.hub_data_size);
				}
			} else if (context_input.type == EV_SYN) {
				syn = true;
				AUTOLOCK(m_mutex);
				if (m_enabled && (context_len > 0)) {
					AUTOLOCK(m_data_mutex);
					m_data = m_pending_data;
					event_updated = true;
					DBG("context event is received!");
				}
			} else {
				ERR("Unknown event (type=%d, code=%d)", context_input.type, context_input.code);
			}
		}

	} else {
		ERR("select nothing to read!!!");
		return false;
	}

	if (syn == false) {
		ERR("EV_SYN didn't come until %d inputs had come", read_input_cnt);
		return false;
	}

	return event_updated;
}

int context_sensor_hal::print_context_data(const char* name, const char *data, int length)
{
	const int LOG_SIZE_LIMIT = 100;

	char buf[6];
	char *log_str;

	if ((length > LOG_SIZE_LIMIT) || (length <= 0)) {
		WARN("log size(%d) is exceptional!", length);
		return -1;
	}

	int log_size = strlen(name) + 2 + sizeof(buf) * length + 1;

	log_str = new(std::nothrow) char[log_size];
	retvm_if (!log_str, -1, "Failed to allocate memory");

	memset(log_str, 0, log_size);

	for (int i = 0; i < length; i++ ) {
		if (i == 0) {
			strcat(log_str, name);
			strcat(log_str, ": ");
	} else {
			strcat(log_str, ", ");
	}
		sprintf(buf, "%d", (signed char)data[i]);
		strcat(log_str, buf);
	}

	DBG("%s", log_str);
	delete[] log_str;

	return length;
}

bool context_sensor_hal::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

int context_sensor_hal::get_sensor_data(sensorhub_data_t &data)
{
	AUTOLOCK(m_data_mutex);
	copy_sensorhub_data(&data, &m_data);
	return 0;

}

bool context_sensor_hal::get_properties(sensor_properties_t &properties)
{
	properties.name = "CONTEXT_SENSOR";
	properties.vendor = "Samsung Electronics";
	properties.min_range = -100;
	properties.max_range = 100;
	properties.min_interval = 1;
	properties.resolution = 1;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	return true;
}

int context_sensor_hal::send_sensorhub_data(const char* data, int data_len)
{
	return send_context_data(data, data_len);
}

int context_sensor_hal::send_context_data(const char *data, int data_len)
{
	int ret;

	if (data_len <= 0) {
		ERR("Invalid data_len(%d)", data_len);
		return -EINVAL;
	}

	if (m_context_fd < 0) {
		ERR("Invalid context fd(%d)", m_context_fd);
		return -ENODEV;
	}

	print_context_data(__FUNCTION__, data, data_len);

write:
	ret = write(m_context_fd, data, data_len);
	if (ret < 0) {
		if (errno == EINTR) {
			INFO("EINTR! retry to write");
			goto write;
		}
		ERR("errno : %d , errstr : %s", errno, strerror(errno));
	}

	return ret < 0 ? -errno : ret;
}


int context_sensor_hal::read_context_data(void)
{
	int ret = 0;

	if (m_context_fd < 0) {
		ERR("Invalid context fd(%d)", m_context_fd);
		return -ENODEV;
	}

read:
	ret = read(m_context_fd, m_pending_data.hub_data, m_pending_data.hub_data_size);

	if (ret > 0) {
		m_pending_data.hub_data_size = ret;
		print_context_data(__FUNCTION__, m_pending_data.hub_data, m_pending_data.hub_data_size);
	} else if (ret < 0) {
		if (errno == EINTR) {
			DBG("EINTR! retry read");
			goto read;
		}

		ERR("errno : %d , errstr : %s", errno, strerror(errno));
		return -errno;
	}

	return ret;
}

int context_sensor_hal::read_large_context_data(void)
{
	int ret = 0;

	if (m_context_fd < 0) {
		ERR("Invalid context fd(%d)", m_context_fd);
		return -ENODEV;
	}

ioctl:
	ret = ioctl(m_context_fd, IOCTL_READ_LARGE_CONTEXT_DATA, m_pending_data.hub_data);

	if (ret > 0) {
		m_pending_data.hub_data_size = ret;
		print_context_data(__FUNCTION__, m_pending_data.hub_data, m_pending_data.hub_data_size);
	} else if (ret < 0) {
		if (errno == EINTR) {
			INFO("EINTR! retry ioctl");
			goto ioctl;
		}

		ERR("errno : %d , errstr : %s", errno, strerror(errno));
		return -errno;
	}

	return ret;
}

extern "C" sensor_module* create(void)
{
	context_sensor_hal *sensor;

	try {
		sensor = new(std::nothrow) context_sensor_hal;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
