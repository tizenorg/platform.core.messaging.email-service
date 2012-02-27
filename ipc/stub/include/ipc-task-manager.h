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


#ifndef _IPC_TASK_MANAGER_H_
#define _IPC_TASK_MANAGER_H_

#include <unistd.h>
#include <pthread.h>
#include "ipc-task.h"
#include "ipc-queue.h"
#include "emf-mutex.h"

#define IPC_TASK_MAX	64


class ipcEmailTaskManager
{
public:
	ipcEmailTaskManager();
	virtual ~ipcEmailTaskManager();

private:
	ipcEmailQueue m_TaskQueue;
	pthread_t	m_hTaskThread;
	bool		m_bStopFlag;
	Mutex mx;
	CndVar cv;

public:
	bool StartTaskThread();
	bool StopTaskThread();

	bool CreateTask(unsigned char* a_pTaskStream, int a_nResponseChannel);
	bool DoTask();
private:
	static void* TaskThreadProc(void *a_pOwner);
};

#endif	/* _IPC_TASK_MANAGER_H_ */


