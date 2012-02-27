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


#include "ipc-socket.h"
#include "ipc-library-build.h"
#include "emf-dbglog.h"
#include "emf-types.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <unistd.h>
#include <errno.h>

/*  Constructor */
ipcEmailSocket::ipcEmailSocket() : maxfd(-1)
{
	m_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	EM_DEBUG_LOG("fd = %d", m_sockfd);

	if (m_sockfd < 0)  {
		EM_DEBUG_LOG("socket creation fails!!!: %s", strerror(errno));
	}

	FD_ZERO(&fds);
}

/*  Destructor */
ipcEmailSocket::~ipcEmailSocket()
{
	FD_ZERO(&fds);
	mapFds.clear();
	EM_DEBUG_LOG("socket object is destoryed");
}

/*  Close */
void ipcEmailSocket::Close()
{
	close(m_sockfd);
	m_sockfd = 0;
	m_bConnect = false;
	EM_DEBUG_LOG("sockfd = [%d] is closed", m_sockfd);
}

void ipcEmailSocket::Close(int fd)
{
	EM_DEBUG_FUNC_BEGIN("fd [%d]",fd);

	if (fd == -1)  {
		EM_DEBUG_LOG("server_socket not init");
		return;
	}
	
	EM_DEBUG_LOG("%d to be removed", fd);
	FD_CLR(fd, &fds);

	std::map<int, int>::iterator it = mapFds.find(fd);
	if(it == mapFds.end()) {
		EM_DEBUG_LOG("No matching socket fd [%d]", fd);
		return;
	} else
		mapFds.erase(it);

	if ( fd == maxfd ) {
		int newmax = 0;
		for( it = mapFds.begin() ; it != mapFds.end() ; it++ )
			newmax = (it->second > newmax )? it->second : newmax; 
		maxfd = newmax;
	}
	EM_DEBUG_LOG("fd %d removal done", fd);
	::close(fd);
	EM_DEBUG_FUNC_END();
}

bool ipcEmailSocket::IsConnected()
{
	return m_bConnect;
}

int ipcEmailSocket::writen (int fd, const char *buf, int len)
{
	int nleft, nwrite;

	nleft = len;
	while( nleft > 0 )  {
		nwrite = ::send(fd, (const void*) buf, nleft, MSG_NOSIGNAL);
/* 		nwrite = ::write(fd, (const void*) buf, nleft); */
		if( nwrite == -1 )  {
			EM_DEBUG_LOG("write: %s", EM_STRERROR(errno));
			if (errno == EINTR) continue;
			return nwrite;
		} else if ( nwrite == 0 ) 
			break;
		
		nleft -= nwrite;
		buf += nwrite;
	}
	return (len-nleft);
}

int ipcEmailSocket::Send(int fd, char* buffer, int buf_len)
{
	EM_DEBUG_FUNC_BEGIN("fd [%d], buffer [%p], buf_len [%d]", fd, buffer, buf_len);

	if( !buffer ) {
		EM_DEBUG_EXCEPTION("No data to send");
		return 0;
	}

	/*
	if(!IsConnected()) {
		EM_DEBUG_EXCEPTION("Socket is not connected.");
		return FAILURE;
	}
	*/

	int send_bytes = sizeof(int)+buf_len;
	char buf[send_bytes];
	memcpy(buf, (char*) &buf_len, sizeof(int)); /*  write buffer lenth */
	memcpy(buf + sizeof(int), buffer, buf_len); /*  write buffer */

	EM_DEBUG_LOG("Sending %dB data to [fd = %d] ", send_bytes, fd);
    int nRet = writen(fd, buf, send_bytes);

	EM_DEBUG_FUNC_END();	
    return nRet;
/*
 	EM_DEBUG_LOG("Sending %dB data to [fd = %d] ", buf_len, fd);
	int n = writen(fd, buffer, buf_len);

	if( n != buf_len ) {
		EM_DEBUG_LOG("WARNING: write buf_size[%d] != send_len [%d]", n, buf_len);
		return FAILURE;
	}
	EM_DEBUG_FUNC_END();		
	return n;
*/
}


int ipcEmailSocket::readn( int fd, char *buf, int len )
{
	int nleft, nread;

	nleft = len;
	while( nleft > 0 ) {
		nread = ::read(fd, (void*)buf, nleft);
		if( nread < 0 ) {
			EM_DEBUG_EXCEPTION("read: %s", strerror(errno));
			if (errno == EINTR) continue;
			return nread;
		} else if( nread == 0 )
			break;
		
		nleft -= nread;
		buf += nread;
	}
	return (len-nleft);
}

int ipcEmailSocket::Recv(int fd, char** buffer)
{
	EM_DEBUG_FUNC_BEGIN();
	if ( !buffer ) {
		EM_DEBUG_LOG("buffer MUST NOT NULL");
		return FAILURE;
	}

	/*  read the data size first and store it to len */
	int n, len = 0;
	
	EM_DEBUG_LOG("[IPC Socket] Receiving header begins");
	n = readn(fd, (char*) &len, sizeof(int));
	EM_DEBUG_LOG("[IPC Socket] Receiving header %dB data", len);
	
	if ( n == 0 ) /*  if service gets down, it signals to all IPC clients */
		return n; else if ( n != sizeof(int) ) {
		EM_DEBUG_LOG("WARNING: read header_size[%d] not matched [%d]", n, sizeof(int));
		return FAILURE;
	}
/*	if( ioctl(fd, FIONREAD, &len) == -1 )  {
        EM_DEBUG_LOG("FIONREAD %s", EM_STRERROR(errno));
        return FAILURE;
    }
*/
	/*  alloc buffer and read data */
	*buffer = new char[len];

	EM_DEBUG_LOG("[IPC Socket] Receiving Body begins for [%d] bytes", len);
	n = readn(fd, *buffer, len);
	if ( n !=  len ) {
		delete[] *buffer;
		*buffer = NULL;
		EM_DEBUG_LOG("WARNING: read buf_size [%d] != read_len[%d]", n, len);
		return FAILURE;
	}

	EM_DEBUG_LOG("[IPC Socket] Receiving [%d] bytes Completed", len);

	return n;
}

int ipcEmailSocket::GetSocketID()
{
	return m_sockfd;
}


int ipcEmailSocket::accept()
{
	EM_DEBUG_FUNC_BEGIN();

	if (m_sockfd == -1)  {
		EM_DEBUG_LOG("server_socket not init");
		return FAILURE;
	}
	
	struct sockaddr_un remote;

	int t = sizeof(remote);
	int fd = ::accept(m_sockfd, (struct sockaddr *)&remote, (socklen_t*) &t);
	if (fd == -1)  {
		EM_DEBUG_LOG("accept: %s", EM_STRERROR(errno));
		return FAILURE;
	}

	addfd(fd);
	EM_DEBUG_LOG("%d is added", fd);

	EM_DEBUG_FUNC_END();
	return fd;
}

int ipcEmailSocket::open(const char* path)
{
	EM_DEBUG_FUNC_BEGIN("path [%s]", path);

	if (!path || strlen(path) > 108) {
		EM_DEBUG_LOG("path is null");
		return FAILURE;
	}
	
	if ( m_sockfd <= 0 ) {
		EM_DEBUG_LOG("Server_socket not created %d", m_sockfd);
		return FAILURE;
	}

	struct sockaddr_un local;
	
	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, path);
	unlink(local.sun_path);
	
	int len = strlen(local.sun_path) + sizeof(local.sun_family);
	
	if (bind(m_sockfd, (struct sockaddr *)&local, len) == -1)  {
		EM_DEBUG_LOG("bind: %s", strerror(errno));
		return FAILURE;
	}

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
		EM_DEBUG_LOG("chmod: %s", strerror(errno));
		return FAILURE;
	}

	if (listen(m_sockfd, 10) == -1) {
		EM_DEBUG_LOG("listen: %s", strerror(errno));
		return FAILURE;
	}

	addfd(m_sockfd);

	EM_DEBUG_FUNC_END();
	return 0;
}


int ipcEmailSocket::Connect()
{
	EM_DEBUG_FUNC_BEGIN();
	
	struct sockaddr_un serverSA;
	serverSA.sun_family = AF_UNIX;
	strcpy(serverSA.sun_path, EM_SOCKET_PATH); /*  "./socket" */
	
	int len = strlen(serverSA.sun_path) + sizeof(serverSA.sun_family);
	
	if (::connect(m_sockfd, (struct sockaddr *)&serverSA, len) == -1)  {
		EM_DEBUG_LOG("cannot connect server %s", strerror(errno));
		return FAILURE;
	}

	/* add fd for select() */
	addfd(m_sockfd);
	m_bConnect = true;

	return 0;
}

void ipcEmailSocket::addfd(int fd)
{
	EM_DEBUG_LOG("%d added", fd);
	FD_SET(fd, &fds);

	std::map<int, int>::iterator it = mapFds.find(fd);
	if(it != mapFds.end()) {
		EM_DEBUG_LOG("Duplicate FD %d", fd);
		return;
	} else
		mapFds[fd] = fd;
	
	if ( fd > maxfd )
		maxfd = fd;
}


