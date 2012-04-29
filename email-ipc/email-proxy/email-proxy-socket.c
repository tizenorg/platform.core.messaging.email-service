/*
*  email-service
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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



#include <unistd.h>
#include <sys/select.h>

#include "email-ipc-build.h"
#include "email-proxy-socket.h"
#include "email-ipc-socket.h"

#include "email-debug-log.h"
#include <errno.h>

static int proxy_socket_fd = 0;

EXPORT_API bool emipc_start_proxy_socket()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true;

	ret = emipc_init_email_socket(&proxy_socket_fd);
	if (!ret) {
		return false;
	}

	ret = emipc_connect_email_socket(proxy_socket_fd);

	return ret;	
}

EXPORT_API bool emipc_end_proxy_socket()
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_LOG("[IPCLib] emipc_end_proxy_socket_fd");
	
	if (proxy_socket_fd) {
		emipc_close_email_socket(proxy_socket_fd);
		proxy_socket_fd = 0;
	}
	
	return true;
}

EXPORT_API int emipc_send_proxy_socket(unsigned char *data, int len)
{
	EM_DEBUG_FUNC_BEGIN();
	if (!proxy_socket_fd) {
		EM_DEBUG_EXCEPTION("[IPCLib] emipc_send_proxy_socket_fd not connect");
		return false;
	}
	return emipc_send_email_socket(proxy_socket_fd, data, len);
}

EXPORT_API int emipc_get_proxy_socket_id()
{
	EM_DEBUG_FUNC_BEGIN();
	return proxy_socket_fd;
}

static bool wait_for_reply (int fd)
{
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	if (select(fd + 1, &fds, NULL, NULL, NULL) == -1) {
		EM_DEBUG_EXCEPTION("[IPCLib] select: %s", strerror(errno) );
		return false;
	}

	if (FD_ISSET(fd, &fds)) return true;
	else return false;
}


EXPORT_API int emipc_recv_proxy_socket(char **data)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int recv_len = 0;
	
	if (!proxy_socket_fd) {
		EM_DEBUG_EXCEPTION("[IPCLib] proxy_socket_fd[%p] is not available or disconnected", proxy_socket_fd);
		return 0;
	}

	if( !wait_for_reply(proxy_socket_fd) ) 	return 0;

	recv_len = emipc_recv_email_socket(proxy_socket_fd, data);

	if (recv_len == 0) {
		EM_DEBUG_EXCEPTION("[IPCLib] Proxy recv delete %x", proxy_socket_fd);
		emipc_close_email_socket(proxy_socket_fd);
		proxy_socket_fd = 0;
	} else if (recv_len == -1) {
		EM_DEBUG_EXCEPTION("[IPCLib] Proxy recv error");
	}
	
	return recv_len;
}

