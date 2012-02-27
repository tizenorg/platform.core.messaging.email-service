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
#include "ipc-library-build.h"
#include "ipc-proxy-main.h"
#include "emf-types.h"

ipcEmailProxyMain* ipcEmailProxyMain::s_pThis = NULL;

ipcEmailProxyMain::ipcEmailProxyMain() : mx()
{
	EM_DEBUG_FUNC_BEGIN();
	m_pSyncSocket = NULL;
	m_pProxyMessageQ = NULL;
	m_bRecvStopFlag = false;
	m_nReference = 0;
	m_nAPPID = 0;
	EM_DEBUG_FUNC_END();
}

ipcEmailProxyMain::~ipcEmailProxyMain()
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_FUNC_END();
}

ipcEmailProxyMain* ipcEmailProxyMain::Instance()
{
	EM_DEBUG_FUNC_BEGIN();
	if(s_pThis)
		return s_pThis;

	s_pThis = new ipcEmailProxyMain();

	EM_DEBUG_FUNC_END();
	return s_pThis;
}

void ipcEmailProxyMain::FreeInstance()
{
	EM_DEBUG_FUNC_BEGIN();
	
	if(s_pThis) {
		s_pThis->Finalize();
		delete s_pThis;
		s_pThis = NULL;
	}
	EM_DEBUG_FUNC_END();
}

int ipcEmailProxyMain::Initialize()
{
	EM_DEBUG_FUNC_BEGIN();

	if (m_nReference > 0) {
		EM_DEBUG_EXCEPTION("Already Initialized m_nReference[%d]", m_nReference);
		return EMF_ERROR_NONE;
	}
	
	m_nReference++;

	m_pSyncSocket = new ipcEmailProxySocket();

	if(!m_pSyncSocket) {
		EM_DEBUG_EXCEPTION("m_pSyncSocket == NULL");
		return EMF_ERROR_IPC_SOCKET_FAILURE;
	}
	
	if( m_pSyncSocket->Start() != EMF_SUCCESS ) {
		EM_DEBUG_EXCEPTION("Socket start FAILs");
		return EMF_ERROR_IPC_CONNECTION_FAILURE;
	}

	EM_DEBUG_LOG("Socket ID : %x", m_pSyncSocket->GetSocketID());
	EM_DEBUG_FUNC_END();
	return EMF_ERROR_NONE;
}

int ipcEmailProxyMain::Finalize()
{
	EM_DEBUG_FUNC_BEGIN();
	
	if (--m_nReference > 0) {
		EM_DEBUG_EXCEPTION("More than one reference m_nReference[%d]",m_nReference);
		return EMF_ERROR_NONE;
	}

	if(m_pSyncSocket) {
		delete m_pSyncSocket;
		m_pSyncSocket = NULL;
	}
	
	if(m_pProxyMessageQ) {
		delete m_pProxyMessageQ;
		m_pProxyMessageQ = NULL;
	}
	EM_DEBUG_FUNC_END();
	return EMF_ERROR_NONE;
}


bool ipcEmailProxyMain::ExecuteAPI(ipcEmailAPIInfo* a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();

	if(!a_hAPI) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		return false;
	}
	
	int nRet;
	char* pInStream = (char*)a_hAPI->GetStream(ePARAMETER_IN);
	int nLength = a_hAPI->GetStreamLength(ePARAMETER_IN);
	bool result = false;
	int sending_bytes;

	mx.lock();
	sending_bytes = m_pSyncSocket->Send(pInStream, nLength);
	mx.unlock();
	
	EM_DEBUG_LOG("Proxy=>Stub Sending %dB.", sending_bytes);

	if( sending_bytes > 0 ) {
#ifdef IPCLIB_STREAM_TRACE_ON	
		int index;
		for(index=0; index< nLength; index++)
			EM_DEBUG_LOG("pInStream[index] : [%x]", pInStream[index]);
#endif	

		char* ipc_buf = NULL;

		/* now waiting for response from email-service */
		mx.lock();
		nRet = m_pSyncSocket->Recv(&ipc_buf);
		mx.unlock();
		EM_DEBUG_LOG("Recv length : %d", nRet);

		if (nRet > 0)
			result = a_hAPI->ParseStream(ePARAMETER_OUT, ipc_buf);
		else
			result = false;

		if(ipc_buf)
			delete []ipc_buf;
	}

	EM_DEBUG_FUNC_END();
	return result;
}

bool ipcEmailProxyMain::Dispatch()
{
	EM_DEBUG_FUNC_BEGIN();
	
	unsigned char pStream[IPC_MSGQ_SIZE];
	
	while(!m_bRecvStopFlag) {
		memset(pStream, 0x00, sizeof(pStream));
		
		if(m_pProxyMessageQ->MsgRcv(pStream, sizeof(pStream))>0) {
			ipcEmailAPIInfo *pAPIInfo = new ipcEmailAPIInfo();
			int nAPIID = *((int*)pStream);
			
			pAPIInfo->SetAPIID(nAPIID);
			pAPIInfo->ParseStream(ePARAMETER_OUT, pStream);
			EM_DEBUG_LOG("Proxy Message Queue Recv [APIID=%x], [RecvLen=%d]", nAPIID, pAPIInfo->GetStreamLength(ePARAMETER_OUT));

#ifdef IPCLIB_STREAM_TRACE_ON		
			int index;
			for(index=0; index< pAPIInfo->GetStreamLength(ePARAMETER_OUT); index++)
				EM_DEBUG_LOG("[%x]", pStream[index]);
#endif
		}
		usleep(1000);	
	}
	EM_DEBUG_FUNC_END();
	return false;
}
