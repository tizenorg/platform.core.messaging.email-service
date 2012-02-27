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


#ifndef _IPC_MESSAGE_Q_H_
#define _IPC_MESSAGE_Q_H_
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include "emf-dbglog.h"
#define IPC_MSGQ_SIZE 512

typedef struct {
	long mtype;					/* type of message 메세지 타입 */
	char mtext[IPC_MSGQ_SIZE];	/* message text 메세지 내용 */
}ipcMsgbuf;

class cmSysMsgQueue
{
public:
	cmSysMsgQueue(int a_nMsgType);
	virtual ~ cmSysMsgQueue();

private:
	int 		m_nMsgType;
	/* key_t		m_nKey; */ /*CID = 17033 BU not used */
	long		m_nMsgqid;
	ipcMsgbuf	*m_pSendBuf;
	ipcMsgbuf	*m_pRecvBuf;
	
public:
	void MsgCreate();
	int MsgSnd(void *a_pMsg, int a_nSize, long a_nType);
	int MsgRcv(void *a_pMsg, int a_nSize);
	bool MsgDestroy();

};

#endif	/* _IPC_MESSAGE_Q_H_ */




