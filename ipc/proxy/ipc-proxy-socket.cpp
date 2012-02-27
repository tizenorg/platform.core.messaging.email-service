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
#include "ipc-library-build.h"
#include "ipc-proxy-socket.h"

ipcEmailProxySocket::ipcEmailProxySocket()
{
	EM_DEBUG_FUNC_BEGIN();
	memset(&fds_read, 0, sizeof(fd_set));/*  CID = 17042 BU */
	m_pSocket = NULL;	
}

ipcEmailProxySocket::~ipcEmailProxySocket()
{
	EM_DEBUG_FUNC_BEGIN();
	End();
}

int ipcEmailProxySocket::Start()
{
	EM_DEBUG_FUNC_BEGIN();
	m_pSocket = new ipcEmailSocket();

	if(m_pSocket) {
		return m_pSocket->Connect();
	}
	
	return -1;
}

bool ipcEmailProxySocket::End()
{
	EM_DEBUG_FUNC_BEGIN();	
	EM_DEBUG_LOG("[IPCLib] ipcEmailProxySocket::End ");

	if(m_pSocket) {
		m_pSocket->Close(); /*  prevent 20071030 */
		delete(m_pSocket);
		m_pSocket = NULL;
	}

	return true;
}

int ipcEmailProxySocket::Send(char* pData , int a_nLen)
{
	EM_DEBUG_FUNC_BEGIN();
	if(m_pSocket && m_pSocket->IsConnected()) {
		return m_pSocket->Send(m_pSocket->GetSocketID(), pData, a_nLen);
	} else {
		EM_DEBUG_EXCEPTION("[IPCLib] ipcEmailProxySocket not connect");
		/* m_pSocket->Connect(); */
		return 0;
	}
}

int ipcEmailProxySocket::GetSocketID()
{
	EM_DEBUG_FUNC_BEGIN();
	if(m_pSocket)
		return m_pSocket->GetSocketID();

	return 0;
}


int ipcEmailProxySocket::Recv(char** pData)
{
	EM_DEBUG_FUNC_BEGIN();


	int nRecvLen = 0;

	if(m_pSocket /*&& m_pSocket->IsConnected()*/)
		nRecvLen = m_pSocket->Recv(m_pSocket->GetSocketID(), pData); else {
		EM_DEBUG_EXCEPTION("[IPCLib] m_pSocket[%p] is not available or disconnected", m_pSocket);
		return 0;
	}

	if( nRecvLen == 0 ) {
		EM_DEBUG_EXCEPTION("[IPCLib] Proxy Recv delete %x", m_pSocket->GetSocketID());
		m_pSocket->Close();
	} else if(nRecvLen == -1) {
		EM_DEBUG_EXCEPTION("[IPCLib] Proxy Recv error");
	}

	return nRecvLen;
/*
	int ret_select;
	fd_set fds_buf;
	int nRecvLen = -2, timeout_count = 0;
	struct timeval timeout;

	while(1) {
		fds_buf = fds_read;

		FD_ZERO(&fds_buf);

		FD_SET(m_pSocket->GetSocketID(), &fds_buf);
		timeout.tv_sec = 0;		
		timeout.tv_usec = 1000000;	
		ret_select = select(m_pSocket->GetSocketID()+1, &fds_buf, NULL, NULL, &timeout);

		if(ret_select == -1) {
			EM_DEBUG_EXCEPTION("[IPCLib] Proxy select error");
			break;
		} else if(ret_select ==  0) {
			EM_DEBUG_EXCEPTION("[IPCLib] Proxy select timeout");
			
			if(timeout_count++ > 10) {
				EM_DEBUG_EXCEPTION("[IPCLib] exit from select loop");
				break;
			}
			continue;
		}
		
		if(FD_ISSET(m_pSocket->GetSocketID(), &fds_buf)) {
			nRecvLen = m_pSocket->Recv(m_pSocket->GetSocketID(), pData);
			if(nRecvLen > 0) {
				EM_DEBUG_LOG("[IPCLib] Proxy Recv[fd = %x], [nRecvLen = %d]", m_pSocket->GetSocketID(), nRecvLen);
				break;
			}
			else if(nRecvLen == 0) {
				EM_DEBUG_EXCEPTION("[IPCLib] Proxy Recv delete %x", m_pSocket->GetSocketID());
				m_pSocket->Close();
				break;
			}
			else if(nRecvLen == -1) {
				EM_DEBUG_EXCEPTION("[IPCLib] Proxy Recv error");
				break;
			}
		}

	}
*/
	return nRecvLen;
	
}
