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
#include "ipc-task-manager.h"
#include "ipc-task.h"
#include "ipc-library-build.h"
#include "emf-dbglog.h"
#include <pthread.h>
#include <errno.h>

pthread_cond_t email_task_queue_signal;
pthread_mutex_t email_task_queue_lock;


/*
void* worker_func(void *a_pOwner)
{
	EM_DEBUG_FUNC_BEGIN();
	ipcEmailTaskManager *pTaskManager = (ipcEmailTaskManager*) a_pOwner;
	if(pTaskManager) {
		pTaskManager->DoTask();
	}
	return NULL;
}
*/
ipcEmailTaskManager::ipcEmailTaskManager() : m_TaskQueue(), m_hTaskThread(0), m_bStopFlag(false),  mx(), cv()
{
}

ipcEmailTaskManager::~ipcEmailTaskManager()
{
	StopTaskThread();
	pthread_cancel(m_hTaskThread);

	ipcEmailTask* pTask = (ipcEmailTask*)m_TaskQueue.Pop();
	while(pTask) {
		delete pTask;
		pTask = (ipcEmailTask*)m_TaskQueue.Pop();
	}
}

bool ipcEmailTaskManager::StartTaskThread()
{
	EM_DEBUG_FUNC_BEGIN();
	if(m_hTaskThread)
		return true;
/*
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);	

	if (pthread_create(&m_hTaskThread, &thread_attr, &TaskThreadProc, (void*)this) != 0) 
*/
	if (pthread_create(&m_hTaskThread, NULL, &TaskThreadProc, (void*)this) != 0)  {
		EM_DEBUG_LOG("Worker thread creation failed: %s", strerror(errno));
		return false;
	}

	return true;
}

bool ipcEmailTaskManager::StopTaskThread()
{
	m_bStopFlag = true;
	return true;
}

void* ipcEmailTaskManager::TaskThreadProc(void *a_pOwner)
{
	EM_DEBUG_FUNC_BEGIN();
	ipcEmailTaskManager *pTaskManager = (ipcEmailTaskManager*)a_pOwner;
	if(pTaskManager) {
		pTaskManager->DoTask();
	}
	return NULL;
}


/* important! m_TaskQueue is shared by worker thread and ipc handler thread */
/* code for worker thread */
bool ipcEmailTaskManager::DoTask()
{
	EM_DEBUG_FUNC_BEGIN();

	ipcEmailTask *pTask = NULL;

	while(!m_bStopFlag) {

		mx.lock();
		while(m_TaskQueue.GetCount() == 0) {
			EM_DEBUG_LOG("Blocked until new task arrives %p.", &cv);
			cv.wait(mx.pMutex());
		}

		pTask = (ipcEmailTask*)m_TaskQueue.Pop();
		mx.unlock();
		
		if(pTask) {
			pTask->Run();
			delete pTask;
			pTask = NULL;
		}
	}

	return false;
}

/* code for ipc handler */
bool ipcEmailTaskManager::CreateTask(unsigned char* a_pTaskStream, int a_nResponseChannel)
{
	EM_DEBUG_FUNC_BEGIN();

/* 	pthread_mutex_init(&email_task_queue_lock, NULL); */
/* 	pthread_cond_init(&email_task_queue_signal, NULL); */

/*
	if(!m_hTaskThread) {
		if (pthread_create(&m_hTaskThread, NULL, &worker_func, (void*)this) != 0)  {
			EM_DEBUG_LOG("Worker thread creation failed: %s", strerror(errno));
			return false;
		}
		EM_DEBUG_LOG("* Worker thread now running *");
	}
*/	
	ipcEmailTask *pTask = new ipcEmailTask();
	if(pTask) {
		pTask->ParseStream(a_pTaskStream, a_nResponseChannel);

		EM_DEBUG_LOG("[IPCLib] ====================================================");
		EM_DEBUG_LOG("[IPCLib] Register new task : %p", pTask);
		EM_DEBUG_LOG("[IPCLib] Task API ID : %s", EM_APIID_TO_STR(pTask->GetAPIInfo()->GetAPIID()));
		EM_DEBUG_LOG("[IPCLib] Task Response ID : %d", pTask->GetAPIInfo()->GetResponseID());
		EM_DEBUG_LOG("[IPCLib] Task APP ID : %d", pTask->GetAPIInfo()->GetAPPID());	
		EM_DEBUG_LOG("[IPCLib] ====================================================");

		mx.lock();
		bool bRtn = m_TaskQueue.Push(pTask);

		int err = cv.signal();
		if(err)
			EM_DEBUG_LOG("cv wakeup error: %s", strerror(errno));

		mx.unlock();
		
		return bRtn;
	}
	return false;
}


