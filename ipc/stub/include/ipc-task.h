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


#ifndef _IPC_TASK_H_
#define _IPC_TASK_H_

#include "ipc-api-info.h"

class ipcEmailTask
{
public:
	ipcEmailTask();
	virtual ~ipcEmailTask();

private:
	ipcEmailAPIInfo *m_pAPIInfo;
	int m_nResponseChannel;

public:
	bool Run();

	bool ParseStream(void* a_pStream, int nResponseFD);

	ipcEmailAPIInfo* GetAPIInfo();
	int GetResponseChannel();
};

#endif	/* _IPC_TASK_H */


