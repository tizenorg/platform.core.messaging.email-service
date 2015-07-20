/*
*  email-service
*
* Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Kyuho Jo <kyuho.jo@samsung.com>, Sunghyun Kwon <sh0701.kwon@samsung.com>
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


#include "email-ipc-socket.h"
#include "email-ipc-build.h"

#include "email-debug-log.h"
#include "email-types.h"

#include "email-core-container.h"

#include <glib.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>

#include <errno.h>
#include <unistd.h>

#include <sys/smack.h>
#include <systemd/sd-daemon.h>

EXPORT_API bool emipc_init_email_socket(int *fd)
{
	bool ret = true;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	*fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (*fd < 0) {
		EM_DEBUG_EXCEPTION("socket failed: %s", EM_STRERROR(errno_buf));
		ret = false;
	}

	EM_DEBUG_LOG("Socket fd = %d", *fd);

	return ret;
}

/*  Close */
EXPORT_API void emipc_close_email_socket(int* fd)
{
	EM_DEBUG_LOG("fd %d removal done", *fd);
	EM_SAFE_CLOSE (*fd);
}

/* returns positive write length,
 * 0, when connection is closed
 * -1, when send error */
static int emipc_writen(int fd, const char *buf, int len)
{
	int length = len;
	int passed_len = 0;

	while (length > 0) {
		passed_len = send(fd, (const void *)buf, length, MSG_NOSIGNAL);
		if (passed_len == -1) {
			EM_DEBUG_EXCEPTION ("send error [%d]", errno);
			if (errno == EINTR) continue;
			else if (errno == EPIPE) return 0; /* connection closed */
			else return passed_len; /* -1 */
		} else if (passed_len == 0)
			break;
		length -= passed_len;
		buf += passed_len;
	}
	return (len - length);
}

/* returns positive value, when write success,
 * 0, when socket connection is broken,
 * EMAIL_ERROR_IPC_SOCKET_FAILURE, when write failure,
 * EMAIL_ERROR_INVALID_PARAM, when wrong parameter */
EXPORT_API int emipc_send_email_socket(int fd, unsigned char *buf, int len)
{
	EM_DEBUG_FUNC_BEGIN("fd [%d], buffer [%p], buf_len [%d]", fd, buf, len);

	if (!buf || len <= 0) {
		EM_DEBUG_EXCEPTION("No data to send %p, %d", buf, len);
		return EMAIL_ERROR_INVALID_PARAM;
	}

	EM_DEBUG_LOG("Sending %dB data to [fd = %d]", len, fd);

	int write_len = emipc_writen(fd, (char*) buf, len);
	if (write_len == 0) /* connection closed */
		return 0; 
	if (write_len != len) {
		EM_DEBUG_LOG("WARNING: buf_size [%d] != write_len[%d]", len, write_len);
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}
	EM_DEBUG_FUNC_END("write_len [%d]", write_len);
	return write_len;
}

static int emipc_readn(int fd, char *buf, int len)
{
	int length = len;
	int read_len = 0;

	while (length > 0) {
		read_len = read(fd, (void *)buf, length);
		if (read_len < 0) {
			EM_DEBUG_EXCEPTION("read err %d", errno);
			if (errno == EINTR) continue;
			else if (errno == EPIPE) return 0; /* connection closed */
			return read_len;
		} else if (read_len == 0)
			break;

		length -= read_len;
		buf += read_len;
	}
	return (len-length);
}

/* returns positive value when read success,
 * 0, when socket is closed
 * EMAIL_ERROR_IPC_SOCKET_FAILURE, when read failed
 * EMAIL_ERROR_INVALID_PARAM when wrong parameter */
EXPORT_API int emipc_recv_email_socket(int fd, char **buf)
{
	EM_DEBUG_FUNC_BEGIN();
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (!buf) {
		EM_DEBUG_LOG("Buffer must not null");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int read_len = 0;
	/* read the size of message. note that ioctl is non-blocking */
	if (ioctl(fd, FIONREAD, &read_len)) {
		EM_DEBUG_EXCEPTION ("ioctl failed: %s", EM_STRERROR(errno_buf));
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}
	/* when server or client closed socket */
	if (read_len == 0) {
		EM_DEBUG_LOG("[IPC Socket] connection is closed");
		return 0;
	}

	*buf = (char *) malloc(read_len);
	if (*buf == NULL) {
		EM_DEBUG_EXCEPTION("Malloc failed");
		return EMAIL_ERROR_OUT_OF_MEMORY;
	}
	memset(*buf, 0x00, read_len);

	EM_DEBUG_LOG_DEV("[IPC Socket] Receiving [%d] bytes", read_len);
	int len = emipc_readn(fd, *buf, read_len);
	if (len == 0) /* connection closed */
		return 0; 
	if (read_len != len) {
		EM_SAFE_FREE(*buf);
		EM_DEBUG_LOG("WARNING: buf_size [%d] != read_len[%d]", read_len, len);
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

	EM_DEBUG_LOG_DEV("[IPC Socket] Receiving [%d] bytes Completed", len);

	return len;
}

EXPORT_API int emipc_accept_email_socket(int fd)
{
	EM_DEBUG_FUNC_BEGIN();
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (fd == -1) {
		EM_DEBUG_EXCEPTION ("Server_socket is not yet initialized");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	struct sockaddr_un remote;
	int remote_len = sizeof(remote);
	int client_fd = accept(fd, (struct sockaddr *)&remote, (socklen_t*) &remote_len);
	if (client_fd == -1) {
		EM_DEBUG_EXCEPTION ("accept failed [%s][%d]", EM_STRERROR(errno_buf), errno);
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

	EM_DEBUG_LOG("%d is added", client_fd);

	EM_DEBUG_FUNC_END();
	return client_fd;
}

EXPORT_API int emipc_open_email_socket(int fd, const char *path)
{
	EM_DEBUG_FUNC_BEGIN("path [%s]", path);
	int sock_fd = 0;
	char errno_buf[ERRNO_BUF_SIZE] = {0};
	char *ipc_socket_path = NULL;
	char uid_string[12] = {0,};

	SNPRINTF(uid_string, sizeof(uid_string), "%d", getuid());
	ipc_socket_path = g_strconcat(EM_SOCKET_USER_PATH, "/", uid_string, "/", EM_SOCKET_PATH_NAME, NULL);

	if (strcmp(path, ipc_socket_path) == 0 &&
		sd_listen_fds(1) == 1 &&
		sd_is_socket_unix(SD_LISTEN_FDS_START, SOCK_SEQPACKET, -1, ipc_socket_path, 0) > 0) {
		EM_SAFE_CLOSE (fd);
		sock_fd = SD_LISTEN_FDS_START + 0;
		EM_SAFE_FREE(ipc_socket_path);
		return sock_fd;
	}

	EM_SAFE_FREE(ipc_socket_path);

	if (!path || EM_SAFE_STRLEN(path) > 108) {
		EM_DEBUG_EXCEPTION ("Path is null");
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

	if (fd <= 0) {
		EM_DEBUG_EXCEPTION ("Socket not created %d", fd);
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

	struct sockaddr_un local;
	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, path);
	unlink(local.sun_path);

	int len = EM_SAFE_STRLEN(local.sun_path) + sizeof(local.sun_family);

	if (bind(fd, (struct sockaddr *)&local, len) == -1) {
		EM_DEBUG_LOG("bind: %s", EM_STRERROR(errno_buf));
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

    emcore_set_declare_link(path);

	/**
	 * determine permission of socket file
	 *
	 *  - S_IRWXU : for user, allow read and write and execute
	 *  - S_IRWXG : for group, allow read and write and execute
	 *  - S_IRWXO : for other, allow read and write and execute
	 *
	 *  - S_IRUSR, S_IWUSR, S_IXUSR : for user, allow only read, write, execute respectively
	 *  - S_IRGRP, S_IWGRP, S_IXGRP : for group, allow only read, write, execute respectively
	 *  - S_IROTH, S_IWOTH, S_IXOTH : for other, allow only read, write, execute respectively
	 */
	mode_t sock_mode = (S_IRWXU | S_IRWXG | S_IRWXO); /*  has 777 permission */

	if (chmod(path, sock_mode) == -1) {
		EM_DEBUG_LOG("chmod: %s", EM_STRERROR(errno_buf));
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

	if (listen(fd, 10) == -1) {
		EM_DEBUG_LOG("listen: %s", EM_STRERROR(errno_buf));
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

	EM_DEBUG_FUNC_END();
	return fd;
}

EXPORT_API int emipc_connect_email_socket(int fd)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int p_errno = 0;
	struct sockaddr_un server;
	char errno_buf[ERRNO_BUF_SIZE] = {0};
	char *ipc_socket_path = NULL;
	char uid_string[12] = {0,};

	SNPRINTF(uid_string, sizeof(uid_string), "%d", getuid());
	ipc_socket_path = g_strconcat(EM_SOCKET_USER_PATH, "/", uid_string, "/", EM_SOCKET_PATH_NAME, NULL);

	memset(&server, 0, sizeof(server));
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, ipc_socket_path);
	
	EM_SAFE_FREE(ipc_socket_path);

	if (connect(fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		EM_DEBUG_EXCEPTION ("connect failed: [%s][errno=%d][fd=%d]", EM_STRERROR(errno_buf), errno, fd);

		p_errno = errno;
		if (p_errno == EACCES || p_errno == EPERM) 
			err = EMAIL_ERROR_PERMISSION_DENIED;
		else 
			err = EMAIL_ERROR_SYSTEM_FAILURE;
		
		return err;
	}

	EM_DEBUG_FUNC_END();
	return err;
}

