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


#include <stdio.h>
#include "ipc-queue.h"
#include "ipc-library-build.h"
#include "emf-dbglog.h"

ipcEmailQueue::ipcEmailQueue()
{
	m_pHead = NULL;
	m_pTail = NULL;
	m_nCount = 0;
}

ipcEmailQueue::~ipcEmailQueue()
{
}

void* ipcEmailQueue::Pop()
{
	if(m_pHead) {
		ipcEmailQueueItem *pPopped = m_pHead;
		void* pData = pPopped->Get();

		if(pPopped->m_pNext) {
			m_pHead = pPopped->m_pNext;
		} else {
			m_pHead = m_pTail = NULL;
		}

		delete pPopped;
		pPopped = NULL;
		m_nCount = (m_nCount <= 0)? 0 : m_nCount-1;

		return pData;
	}
	return NULL;
}

bool ipcEmailQueue::Push(void* a_pData)
{
	/* EM_DEBUG_FUNC_BEGIN(); */
	if(!a_pData) {
		EM_DEBUG_EXCEPTION("[IPCLib] ipcEmailQueue::Push - invalid input \n");
		return false;
	}

	ipcEmailQueueItem *pItem = new ipcEmailQueueItem(a_pData);
	if(m_pTail) {
		m_pTail->m_pNext = pItem;
		m_pTail = pItem;
	} else {
		m_pHead = m_pTail = pItem;
	}

	m_nCount++;
	pItem->m_pNext = NULL;
	return true;
}

int ipcEmailQueue::GetCount()
{
	return m_nCount;
}




ipcEmailQueueItem::ipcEmailQueueItem()
{
	m_pData = NULL;
	m_pNext = NULL;
}

ipcEmailQueueItem::ipcEmailQueueItem(void* a_pData)
{
	m_pData = a_pData;
	m_pNext = NULL;
}

ipcEmailQueueItem::~ipcEmailQueueItem()
{
}

void* ipcEmailQueueItem::Get()
{
	return m_pData;
}

void ipcEmailQueueItem::Set(void* a_pData)
{
	m_pData = a_pData;
}

