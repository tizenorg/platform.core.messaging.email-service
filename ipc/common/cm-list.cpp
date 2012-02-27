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
#include "cm-list.h"
#include "ipc-queue.h"
#include "ipc-library-build.h"

cmList::cmList()
{
	m_pHead = NULL;
	m_pTail = NULL;
	m_nCount = 0;
}

cmList::~cmList()
{
}

bool cmList::AddTail(void* a_pData)
{
	if(!a_pData)
		return false;

	ipcEmailQueueItem *pItem = new ipcEmailQueueItem(a_pData);
	if(!pItem)
		return false;

	if(m_pTail) {
		m_pTail->m_pNext = pItem;
		m_pTail = pItem;
	} else {
		m_pHead = m_pTail = pItem;
	}
	
	m_nCount++;
	
	return true;
}

bool cmList::RemoveItem(void* a_pData)
{
	if(m_pHead) {
		ipcEmailQueueItem *pItem = m_pHead;
		ipcEmailQueueItem *pPrevItem = NULL;
		int nIndex = 0;
		for(nIndex = 0; nIndex < m_nCount; nIndex++) {
			if(pItem->m_pData == a_pData) {
				if(pPrevItem) {
					pPrevItem->m_pNext = pItem->m_pNext;

					if(pItem->m_pNext == NULL)	/*  Tail을 지우는 경우 */
						m_pTail = pPrevItem;	/*  Tail을 바로 이전 것으로 설정 */
				} else	/*  Head를 지우는 경우 */ {
					m_pHead = m_pHead->m_pNext;

					if(m_pHead == NULL)	/*  다 지워졌으면 Tail도 NULL로 초기화 */
						m_pTail = NULL;
				}
	
				delete pItem;
				pItem = NULL;
				m_nCount--;
				return true;
			}
			pPrevItem = pItem; /*  prevent 20071030 */
			pItem = pItem->m_pNext;
		}
	}
	return false;
}

void* cmList::GetAt(int a_nIndex)
{
	if(m_pHead) {
		ipcEmailQueueItem *pItem = m_pHead;
		int nIndex = 0;
		for(nIndex = 0; nIndex < a_nIndex; nIndex++) {
			pItem = pItem->m_pNext;
		}
		
		if(pItem) {
			return pItem->m_pData;
		}
	}
	return NULL;
}

int cmList::GetCount()
{
	return m_nCount;
}


