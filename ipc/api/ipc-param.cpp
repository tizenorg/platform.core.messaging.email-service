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
#include <stdlib.h>

#include "ipc-param.h"
#include "emf-dbglog.h"

ipcEmailParam::ipcEmailParam()
{
	m_nLength = 0;
	m_pData = NULL;

}
ipcEmailParam::~ipcEmailParam()
{
	EM_SAFE_FREE(m_pData);
}

bool ipcEmailParam::SetParam(void* a_pData, int a_nLen)
{
	if(a_nLen == 0)
		return true;
	
	m_pData = (void*)malloc(a_nLen);
	if(m_pData == NULL)
		return false;
	memset(m_pData, 0x00, a_nLen);
	
	if(a_pData != NULL) {
		memcpy(m_pData, a_pData, a_nLen);
	}
	m_nLength = a_nLen;
	return true;
}

int ipcEmailParam::GetLength()
{
	return m_nLength;
}

void* ipcEmailParam::GetData()
{
	return m_pData;
}


