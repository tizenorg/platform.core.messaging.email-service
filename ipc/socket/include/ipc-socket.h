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


#ifndef _IPC_SOCKET_H_ 
#define _IPC_SOCKET_H_ 

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <map>

#define SOCKET_IP		"127.0.0.1"
#define SOCKET_PORT		9000

#define BUF_SIZE			204800
#define OPT_USEC		100000


#define EM_SOCKET_PATH "/tmp/.emailfw_socket"

class ipcEmailSocket
{
	
	protected:	
		int m_sockfd;
		bool m_bConnect;

		fd_set 				fds; 
		int 				maxfd;
		std::map<int, int> 	mapFds;
		
	public:
		ipcEmailSocket();
		~ipcEmailSocket();

	private:
		int readn( int fd, char *buf, int len );
		int writen (int fd, const char *buf, int len);

	public:
		bool IsConnected();

		void Close();
		void Close(int fd);
		int Recv(int fd, char** buffer);
		int Send(int fd, char* buffer, int len);
		int GetSocketID();

		int accept();
		int open(const char* path);
		int Connect();
		void addfd(int fd);

		int 	maxFd() { return (maxfd+1); }
		fd_set 	fdSet() { return fds; }

};

#endif /*  _IPC_SOCKET_H_		 */



	


