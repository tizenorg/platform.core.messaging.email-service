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



#include "ipc-callback-info.h"

ipcEmailCallbackInfo::ipcEmailCallbackInfo()
{
	m_nAPIID = 0;
	m_pfnCallBack = 0;
}

ipcEmailCallbackInfo::~ipcEmailCallbackInfo()
{
}


bool ipcEmailCallbackInfo::SetValue(int a_nAPIID, void* a_pfnCallBack)
{
	m_nAPIID = a_nAPIID;
	m_pfnCallBack = a_pfnCallBack;
	return true;
}

int  ipcEmailCallbackInfo::GetAPIID()
{
	return m_nAPIID;
}

void* ipcEmailCallbackInfo::GetCallBack()
{
	return m_pfnCallBack;
}
