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

#include "ipc-response-info.h"

ipcEmailResponseInfo::ipcEmailResponseInfo()
{
	m_nResponseID = 0;
	m_nAPIID = 0;
}

ipcEmailResponseInfo::~ipcEmailResponseInfo()
{
}

bool ipcEmailResponseInfo::SetVaue(long a_nResponseID, long a_nAPIID)
{
	m_nResponseID = a_nResponseID;
	m_nAPIID = a_nAPIID;
	return true;
}

long ipcEmailResponseInfo::GetResponseID()
{
	return m_nResponseID;
}


long ipcEmailResponseInfo::GetAPIID()
{
	return m_nAPIID;
}


