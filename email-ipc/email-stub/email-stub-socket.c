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



#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/epoll.h>
#include <pthread.h>

#include "email-ipc-build.h"
#include "email-ipc-api-info.h"
#include "email-ipc-socket.h"
#include "email-stub-task.h"
#include "email-stub-task-manager.h"
#include "email-stub-socket.h"

#include "email-debug-log.h"

#define MAX_EPOLL_EVENT 50

static int stub_socket = 0;
static pthread_t stub_socket_thread = 0;
static bool stop_thread = false;

static void *emipc_stub_socket_thread_proc();

EXPORT_API bool emipc_start_stub_socket()
{
	bool ret = true;

	ret = emipc_init_email_socket(&stub_socket);
	if (!ret) {
		EM_DEBUG_EXCEPTION("emipc_init_email_socket failed");
		return ret;
	}

	ret = emipc_start_stub_socket_thread();
	if (!ret) {
		EM_DEBUG_EXCEPTION("emipc_start_stub_socket_thread failed");
		return ret;
	}

	ret = emipc_start_task_thread();
	if (!ret) {
		EM_DEBUG_EXCEPTION("emipc_start_task_thread failed");
		return ret;
	}

	return ret;
}

EXPORT_API bool emipc_start_stub_socket_thread()
{
	EM_DEBUG_LOG("[IPCLib] emipc_email_stub_socket_thread start");
	if (stub_socket_thread)
		return true;
		
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&stub_socket_thread, &thread_attr, &emipc_stub_socket_thread_proc, NULL) != 0) {
		EM_DEBUG_EXCEPTION("[IPCLib] emipc_start_stub_socket_thread() - fail to create a thread");
		return false;
	}
	return true;
}

EXPORT_API bool emipc_stop_stub_socket_thread()
{
	if (stub_socket_thread)
		stop_thread = true;

	return true;
}

static void *emipc_stub_socket_thread_proc()
{
	emipc_wait_for_ipc_request();
	stub_socket_thread = 0;
	return NULL;
}

EXPORT_API void emipc_wait_for_ipc_request()
{
	struct epoll_event ev = {0};
	struct epoll_event events[MAX_EPOLL_EVENT] = {{0}, };
	int epfd = 0;
	int event_num = 0;

	if (!stub_socket) {
		EM_DEBUG_EXCEPTION("Server Socket is not initialized");
		return;
	}

	emipc_open_email_socket(stub_socket, EM_SOCKET_PATH);
	
	epfd = epoll_create(MAX_EPOLL_EVENT);
	if (epfd < 0) {
		EM_DEBUG_EXCEPTION("epoll_ctl: %s[%d]", strerror(errno), errno);
		EM_DEBUG_CRITICAL_EXCEPTION("epoll_create: %s[%d]", strerror(errno), errno);
		abort();
	}

	ev.events = EPOLLIN;
	ev.data.fd = stub_socket;
	
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, stub_socket, &ev) == -1) {
		EM_DEBUG_EXCEPTION("epoll_ctl: %s[%d]", strerror(errno), errno);
		EM_DEBUG_CRITICAL_EXCEPTION("epoll_ctl:%s[%d]", strerror(errno), errno); 	
	}
	while (1) {
		int i = 0;

		event_num = epoll_wait(epfd, events, MAX_EPOLL_EVENT, -1);
		
		if (event_num == -1) {
			EM_DEBUG_EXCEPTION("epoll_wait: %s[%d]", strerror(errno), errno);
			EM_DEBUG_CRITICAL_EXCEPTION("epoll_wait: %s[%d]", strerror(errno), errno);
			if (errno == EINTR) continue; /* resume when interrupted system call*/
			else abort();
		} else {
			for (i = 0; i < event_num; i++) {
				int event_fd = events[i].data.fd;

				if (event_fd == stub_socket) { /*  if it is socket connection request */
					int cfd = emipc_accept_email_socket(stub_socket);
					if (cfd < 0) {
						EM_DEBUG_EXCEPTION("accept error: %s[%d]", strerror(errno), errno);
						EM_DEBUG_CRITICAL_EXCEPTION("accept error: %s[%d]", strerror(errno), errno);
						/*  abort(); */
					}
					ev.events = EPOLLIN;
					ev.data.fd = cfd;
					epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
				} else {
					int recv_len;
					char *sz_buf = NULL;
					
					recv_len = emipc_recv_email_socket(event_fd, &sz_buf);
					
					if(recv_len > 0) {
						EM_DEBUG_LOG("====================================================================");
						EM_DEBUG_LOG("[IPCLib]Stub Socket Recv [Socket ID = %d], [recv_len = %d]", event_fd, recv_len);
						EM_DEBUG_LOG("====================================================================");

						/* IPC request stream is at least 16byte */
						if (recv_len >= sizeof(long) * eSTREAM_DATA) {
							emipc_create_task((unsigned char *)sz_buf, event_fd);
						} else
							EM_DEBUG_LOG("[IPCLib] Stream size is less than default size");
					} else if( recv_len == 0 ) {
						EM_DEBUG_LOG("[IPCLib] Client closed connection [%d]", event_fd);
						epoll_ctl(epfd, EPOLL_CTL_DEL, event_fd, events);
						close(event_fd);
					} 
					EM_SAFE_FREE(sz_buf);
				}
			}
		}
	}	
}

EXPORT_API bool emipc_end_stub_socket()
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (stub_socket) {
		emipc_close_email_socket(&stub_socket);
	}

	if (stub_socket_thread) {
		emipc_stop_stub_socket_thread(stub_socket_thread);
		pthread_cancel(stub_socket_thread);
		stub_socket_thread = 0;
	}

	if (!emipc_stop_task_thread()) {
		EM_DEBUG_EXCEPTION("emipc_stop_task_thread failed");
		return false;	
	}
		
	return true;
}

EXPORT_API int emipc_send_stub_socket(int sock_fd, void *data, int len)
{
	EM_DEBUG_FUNC_BEGIN("Stub socket sending %d bytes", len);

	int sending_bytes = emipc_send_email_socket(sock_fd, data, len);
	
	EM_DEBUG_FUNC_END("sending_bytes = %d", sending_bytes);
	return sending_bytes;
}
/* stub socket accpet, recv, send */

