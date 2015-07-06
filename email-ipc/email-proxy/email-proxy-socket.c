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



#include <unistd.h>
#include <sys/select.h>

#include "email-ipc-build.h"
#include "email-proxy-socket.h"
#include "email-ipc-socket.h"

#include "email-debug-log.h"
#include "email-internal-types.h"
#include "email-utilities.h"
#include <errno.h>
#include <glib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
	pid_t pid;
	pthread_t tid;
	int socket_fd;
} thread_socket_t;

GList *socket_head = NULL;
pthread_mutex_t proxy_mutex = PTHREAD_MUTEX_INITIALIZER;

EXPORT_API int emipc_start_proxy_socket()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = true;
	int socket_fd = 0;

	ret = emipc_init_email_socket(&socket_fd);
	if (!ret) {
		EM_DEBUG_EXCEPTION("emipc_init_email_socket failed");
		return false;
	}

	ret = emipc_connect_email_socket(socket_fd);
	if (ret != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_connect_email_socket failed");
		return ret;
	}

	thread_socket_t* cur = (thread_socket_t*) em_malloc(sizeof(thread_socket_t));
	if(!cur) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return false;
	}

	/* add a socket */
	cur->pid = getpid();
	cur->tid = pthread_self();
	cur->socket_fd = socket_fd;

	ENTER_CRITICAL_SECTION(proxy_mutex);
	socket_head = g_list_prepend(socket_head, cur);
	LEAVE_CRITICAL_SECTION(proxy_mutex);

	return true;
}

EXPORT_API bool emipc_end_proxy_socket()
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_LOG("[IPCLib] emipc_end_proxy_socket");
	pthread_t tid = pthread_self();

	ENTER_CRITICAL_SECTION(proxy_mutex);
	GList *cur = socket_head;
	while( cur ) {
		thread_socket_t* cur_socket = g_list_nth_data(cur,0);

		/* close the socket of current thread */
		if( tid == cur_socket->tid ) {
			emipc_close_email_socket(&cur_socket->socket_fd);
			EM_SAFE_FREE(cur_socket);
			GList *del = cur;
			cur = g_list_next(cur);
			socket_head = g_list_remove_link(socket_head, del);
			break;
		}

		cur = g_list_next(cur);
	}
	LEAVE_CRITICAL_SECTION(proxy_mutex);

	return true;
}

EXPORT_API bool emipc_end_all_proxy_sockets()
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_LOG("[IPCLib] emipc_end_all_proxy_sockets");

	pid_t pid = getpid();

	ENTER_CRITICAL_SECTION(proxy_mutex);
	GList *cur = socket_head;
	while( cur ) {
		thread_socket_t* cur_socket = g_list_nth_data(cur,0);

		/* close all sockets of the pid */
		if( pid == cur_socket->pid ) {
			emipc_close_email_socket(&cur_socket->socket_fd);
			EM_SAFE_FREE(cur_socket);
			GList *del = cur;
			cur = g_list_next(cur);
			socket_head = g_list_remove_link(socket_head, del);
			continue;
		}

		cur = g_list_next(cur);
	}
	LEAVE_CRITICAL_SECTION(proxy_mutex);

	return true;
}

/* return result of emipc_send_email_socket
 * EMAIL_ERROR_IPC_SOCKET_FAILURE, when no IPC connection */
EXPORT_API int emipc_send_proxy_socket(unsigned char *data, int len)
{
	EM_DEBUG_FUNC_BEGIN("data [%p] len [%d]", data, len);
	int socket_fd = emipc_get_proxy_socket_id();

	/* if thread socket is not created */
	if (!socket_fd) {
		int ret = emipc_start_proxy_socket();
		if (!ret) {
			EM_DEBUG_EXCEPTION("[IPCLib] emipc_send_proxy_socket not connected");
			return EMAIL_ERROR_IPC_SOCKET_FAILURE;
		}
		socket_fd = emipc_get_proxy_socket_id();
	}

	int send_len = emipc_send_email_socket(socket_fd, data, len);
	if (send_len == 0) {
		EM_DEBUG_EXCEPTION("[IPCLib] server closed connection %x", socket_fd);
		emipc_end_proxy_socket();
	}

	EM_DEBUG_FUNC_END("send_len [%d]", send_len);
	return send_len;
}

EXPORT_API int emipc_get_proxy_socket_id()
{
	EM_DEBUG_FUNC_BEGIN();
	pthread_t tid = pthread_self();
	int socket_fd = 0;

	ENTER_CRITICAL_SECTION(proxy_mutex);
	GList *cur = socket_head;
	/* need to acquire lock */
	for( ; cur ; cur = g_list_next(cur) ) {
		thread_socket_t* cur_socket = g_list_nth_data(cur,0);
		if( pthread_equal(tid, cur_socket->tid) ) {
			socket_fd = cur_socket->socket_fd;
			break;
		}
	}
	LEAVE_CRITICAL_SECTION(proxy_mutex);
	EM_DEBUG_FUNC_END("tid %d, socket_fd %d", tid, socket_fd);
	return socket_fd;
}

/* return true, when event occurred
 * false, when select error
 */
static bool wait_for_reply (int fd)
{
	int err = -1;
	fd_set fds;
	struct timeval tv;

	if (fd < 0) {
		EM_DEBUG_EXCEPTION("Invalid file description : [%d]", fd);
		return false;
	}

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	tv.tv_sec  = 20; /* should be tuned */
	tv.tv_usec = 0;

	EM_DEBUG_LOG_DEV ("wait for response [%d]", fd);
	err = select(fd + 1, &fds, NULL, NULL, &tv);
	if (err == -1) {
		EM_DEBUG_EXCEPTION("[IPCLib] select error[%d] fd[%d]", errno, fd);
		return false;
	}
	else if (err == 0) {
		EM_DEBUG_EXCEPTION("[IPCLib] select timeout fd[%d]", fd);
		return false;
	}

	if (FD_ISSET(fd, &fds)) return true;

	return false;
}


/* return result of emipc_recv_email_socket
 * EMAIL_ERROR_IPC_SOCKET_FAILURE, when no IPC connection, or wrong fd */
EXPORT_API int emipc_recv_proxy_socket(char **data)
{
	EM_DEBUG_FUNC_BEGIN();
	int socket_fd = emipc_get_proxy_socket_id();
	if (!socket_fd) {
		EM_DEBUG_EXCEPTION("[IPCLib] proxy_socket_fd[%p] is not available or disconnected", socket_fd);
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

	if( !wait_for_reply(socket_fd) ) {
		return EMAIL_ERROR_IPC_SOCKET_FAILURE;
	}

	int recv_len = emipc_recv_email_socket(socket_fd, data);
	if (recv_len == 0) {
		EM_DEBUG_EXCEPTION("[IPCLib] server closed connection %x", socket_fd);
		emipc_end_proxy_socket();
	}

	return recv_len;
}

