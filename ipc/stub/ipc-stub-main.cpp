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



#include "ipc-stub-main.h"
#include "ipc-library.h"
#include "ipc-response-info.h"
#include "ipc-param-list.h"
#include "ipc-library-build.h"


ipcEmailStubMain* ipcEmailStubMain::s_pThis = NULL;

ipcEmailStubMain::ipcEmailStubMain()
{
	EM_DEBUG_FUNC_BEGIN();
	m_pfnAPIMapper = NULL;
	m_pStubSocket = NULL;
	m_pMsgSender = NULL;
	m_pResponseList = NULL;
	EM_DEBUG_FUNC_END();
}

ipcEmailStubMain::~ipcEmailStubMain()
{
	EM_DEBUG_FUNC_BEGIN();
	Finalize();
	EM_DEBUG_FUNC_END();
}


ipcEmailStubMain* ipcEmailStubMain::Instance()
{
	EM_DEBUG_FUNC_BEGIN();
	if(s_pThis)
		return s_pThis;

	s_pThis = new ipcEmailStubMain();	
	EM_DEBUG_FUNC_END();
	return s_pThis;
}

void ipcEmailStubMain::FreeInstance()
{
	EM_DEBUG_FUNC_BEGIN();
	if(s_pThis) {
		delete s_pThis;
		s_pThis = NULL;
	}
	EM_DEBUG_FUNC_END();
}

bool ipcEmailStubMain::Initialize(PFN_EXECUTE_API a_pfnAPIMapper)
{
	EM_DEBUG_FUNC_BEGIN();
	m_pfnAPIMapper = a_pfnAPIMapper;

	if(!a_pfnAPIMapper || m_pStubSocket) {
		EM_DEBUG_EXCEPTION("Invalid Param");
		return false;
	}

	m_pfnAPIMapper = a_pfnAPIMapper;

	m_pResponseList = new cmList();
	if(!m_pResponseList) {
		EM_DEBUG_EXCEPTION("m_pResponseList == NULL\n");
		return false;
	}

	m_pMsgSender = new cmSysMsgQueue(0);
	if(!m_pMsgSender) {
		EM_DEBUG_EXCEPTION("m_pMsgSender == NULL\n");
		return false;
	}
	m_pMsgSender->MsgCreate();

	m_pStubSocket = new ipcEmailStubSocket;
	
	if(m_pStubSocket) {
		return m_pStubSocket->Start();
	} else {
		EM_DEBUG_EXCEPTION("m_pStubSocket == NULL\n");
		return false;
	}
	EM_DEBUG_FUNC_END();
	return false;
}

bool ipcEmailStubMain::Finalize()
{
	EM_DEBUG_FUNC_BEGIN();

	if(m_pResponseList) {
		int nIndex;
		for(nIndex = 0; nIndex<m_pResponseList->GetCount(); nIndex++) {
			ipcEmailResponseInfo *pResponseInfo = (ipcEmailResponseInfo *)m_pResponseList->GetAt(nIndex);
			if(pResponseInfo)
				m_pResponseList->RemoveItem(pResponseInfo);
		}
		delete m_pResponseList;
	}
	if(m_pMsgSender) {
		m_pMsgSender->MsgDestroy();
		m_pMsgSender = NULL;
	}
	
	if(m_pStubSocket) {
		m_pStubSocket->End();
		delete m_pStubSocket;
		m_pStubSocket = NULL;
	}
	
	if(m_pfnAPIMapper)
		m_pfnAPIMapper = NULL;
		
	EM_DEBUG_FUNC_END();
	return true;
}

bool ipcEmailStubMain::ExecuteAPIProxyToStub(ipcEmailAPIInfo *a_pAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	if(!m_pfnAPIMapper || !a_pAPI) {
		EM_DEBUG_EXCEPTION("Invalid Param");
		return false;
	}
	
	m_pfnAPIMapper(a_pAPI);
	EM_DEBUG_FUNC_END();
	return true;
}

bool ipcEmailStubMain::ExecuteAPIStubToProxy(ipcEmailAPIInfo *a_pAPI)
{
	EM_DEBUG_FUNC_BEGIN("a_pAPI [%p]", a_pAPI);
	EM_IF_NULL_RETURN_VALUE(a_pAPI, false);
	EM_DEBUG_LOG("APIID [%s], Response SockID [%d], APPID [%d]", EM_APIID_TO_STR(a_pAPI->GetAPIID()), a_pAPI->GetResponseID(), a_pAPI->GetAPPID());
	
	unsigned char* pStream = (unsigned char*)a_pAPI->GetStream(ePARAMETER_OUT);
	int nStreamLength = a_pAPI->GetStreamLength(ePARAMETER_OUT);
	EM_DEBUG_LOG("Data : %p, Data length : %dB",pStream, nStreamLength);
	EM_DEBUG_LOG("Stub => Proxy Sending %dB", nStreamLength);
	m_pStubSocket->Send(a_pAPI->GetResponseID(), pStream, nStreamLength);

#ifdef IPCLIB_STREAM_TRACE_ON
	int nIndex;
	for(nIndex=0; nIndex< nStreamLength; nIndex++)
		EM_DEBUG_LOG("pStream[nIndex] :[%d]", pStream[nIndex]);
#endif

	EM_DEBUG_FUNC_END();
	return true;
}

bool ipcEmailStubMain::SetResponseInfo( long a_nAPPID, long a_nAPIID)
{
	EM_DEBUG_FUNC_BEGIN("ResponseID [%d]", a_nAPPID);

	if(a_nAPPID <= 0)	
		return true;

	ipcEmailResponseInfo *pResponseInfo = new ipcEmailResponseInfo();
	if(pResponseInfo) {
		pResponseInfo->SetVaue(a_nAPPID, a_nAPIID);
		m_pResponseList->AddTail(pResponseInfo);
		return true;
	}
	EM_DEBUG_FUNC_END();
	return false;
}

