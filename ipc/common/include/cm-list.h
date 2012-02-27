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


#ifndef _IPC_LIST_H_
#define _IPC_LIST_H_

class ipcEmailQueueItem;

class cmList
{
public:
	cmList();
	virtual ~cmList();

private:
	ipcEmailQueueItem* m_pHead;
	ipcEmailQueueItem* m_pTail;
	int m_nCount;

public:
	bool	AddTail(void* a_pData);

	bool	RemoveItem(void* a_pData);
	void*	GetAt(int a_nIndex);
	int 	GetCount();
};

#endif	/* _IPC_LIST_H_ */


