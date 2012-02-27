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


#include <string.h>
#include <unistd.h>
#include "ipc-api-info.h"
#include "ipc-param-list.h"
#include "ipc-library-build.h"

ipcEmailAPIInfo::ipcEmailAPIInfo()
{
	m_pParams[ePARAMETER_IN] = NULL;
	m_pParams[ePARAMETER_OUT] = NULL;
	m_nAPIID = 0;
	m_nResponseID = 0;
	m_nAPPID = 0;
}

ipcEmailAPIInfo::~ipcEmailAPIInfo()
{
	if(m_pParams[ePARAMETER_IN]) {
		delete m_pParams[ePARAMETER_IN];
		m_pParams[ePARAMETER_IN] = NULL;
	}

	if(m_pParams[ePARAMETER_OUT]) {
		delete m_pParams[ePARAMETER_OUT];
		m_pParams[ePARAMETER_OUT] = NULL;
	}
}

bool ipcEmailAPIInfo::SetAPIID(long a_nAPIID)
{
	m_nAPIID = a_nAPIID;
	return true;
}

long ipcEmailAPIInfo::GetAPIID()
{
	return m_nAPIID;
}


bool ipcEmailAPIInfo::SetAPPID(long a_nAPPID)
{
	m_nAPPID = a_nAPPID;
	return true;
}

long ipcEmailAPIInfo::GetAPPID()
{
	return m_nAPPID;
}


bool ipcEmailAPIInfo::SetResponseID(long a_nResponseID)
{
	m_nResponseID = a_nResponseID;
	return true;
}

long ipcEmailAPIInfo::GetResponseID()
{
	return m_nResponseID;
}

bool ipcEmailAPIInfo::ParseStream(EPARAMETER_DIRECTION a_eDirection, void* a_pStream)
{
	if(m_pParams[a_eDirection] == NULL) {
		m_pParams[a_eDirection] = new ipcEmailParamList();
	}

	ParseAPIID(a_pStream);
	ParseResponseID(a_pStream);
	ParseAPPID(a_pStream);
	return m_pParams[a_eDirection]->ParseStream(a_pStream);
}

void* ipcEmailAPIInfo::GetStream(EPARAMETER_DIRECTION a_eDirection)
{

	if(m_pParams[a_eDirection] == NULL) {
		m_pParams[a_eDirection] = new ipcEmailParamList();
	}

	unsigned char* pStream = (unsigned char*)m_pParams[a_eDirection]->GetStream();
	if ( pStream != NULL ) {
		memcpy(pStream, (void*)&m_nAPIID, sizeof(m_nAPIID));
		memcpy(pStream+(sizeof(long)*eSTREAM_RESID), (void*)&m_nResponseID, sizeof(m_nResponseID));
		memcpy(pStream+(sizeof(long)*eSTREAM_APPID), (void*)&m_nAPPID, sizeof(m_nAPPID));
	}

	return (void*)pStream;
}

int ipcEmailAPIInfo::GetStreamLength(EPARAMETER_DIRECTION a_eDirection)
{
	if(m_pParams[a_eDirection] == NULL)
		return 0;

	return m_pParams[a_eDirection]->GetStreamLength();
}

void* ipcEmailAPIInfo::GetParameters(EPARAMETER_DIRECTION a_eDirection)
{
	if(m_pParams[a_eDirection] == NULL) {
		m_pParams[a_eDirection] = new ipcEmailParamList();
	}
	return m_pParams[a_eDirection];
}


long ipcEmailAPIInfo::ParseAPIID(void* a_pStream)
{
	m_nAPIID = *((long*)a_pStream + eSTREAM_APIID);
	return m_nAPIID;
}

long ipcEmailAPIInfo::ParseResponseID(void* a_pStream)
{
	m_nResponseID = *((long*)a_pStream + eSTREAM_RESID);
	return m_nResponseID;
}


long ipcEmailAPIInfo::ParseAPPID(void* a_pStream)
{
	m_nAPPID = *((long*)a_pStream + eSTREAM_APPID);
	return m_nAPPID;

}
