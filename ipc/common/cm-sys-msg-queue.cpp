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
#include <unistd.h>
#include <stdlib.h>
#include "cm-sys-msg-queue.h"
#include "ipc-library-build.h"

cmSysMsgQueue::cmSysMsgQueue(int a_nMsgType)
{
	m_nMsgType = a_nMsgType;
	m_pSendBuf = NULL;
	m_pRecvBuf = NULL;	
	m_nMsgqid = -1;
	m_pSendBuf = (ipcMsgbuf * )malloc(sizeof(ipcMsgbuf));
	m_pRecvBuf = (ipcMsgbuf * )malloc(sizeof(ipcMsgbuf));
}

cmSysMsgQueue::~cmSysMsgQueue()
{
	if(m_pSendBuf) {
		free(m_pSendBuf);
		m_pSendBuf = NULL;
	}
	if(m_pRecvBuf) {
		free(m_pRecvBuf);
		m_pRecvBuf = NULL;
	}
	
}

void cmSysMsgQueue::MsgCreate()
{
	EM_DEBUG_FUNC_BEGIN();
	key_t key = (key_t)640;/* (key_t)a_nKey; */
	
	/* if ((m_nMsgqid = msgget(key, IPC_CREAT | IPC_EXCL | 0640)) == -1)  */
	if ((m_nMsgqid = msgget(key, IPC_CREAT | IPC_EXCL | 0777)) == -1) /*  Security Issue - DAC */ {
		m_nMsgqid = msgget(key, 1);

		/* 기존의 메세지 큐가 존재한다면 일단 지우고 다시 생성한다. */
		if(errno == EEXIST) {
			EM_DEBUG_LOG("Message Queue Exist ");
			/* msgctl(m_nMsgqid, IPC_RMID, (struct msqid_ds *)NULL);  */
			/* m_nMsgqid = msgget(key, IPC_CREAT | IPC_EXCL | 0640); */
			EM_DEBUG_LOG("===================================================== ");
			EM_DEBUG_LOG("[cmSysMsgQueue::MsgCreate][EXIST]");
			EM_DEBUG_LOG("=====================================================");
		}
	} 
	EM_DEBUG_LOG("=====================================================");
	EM_DEBUG_LOG("[cmSysMsgQueue::MsgCreate][msggid = %d]", m_nMsgqid);
	EM_DEBUG_LOG("=====================================================");
	
}
int cmSysMsgQueue::MsgSnd(void *a_pMsg, int a_nSize, long a_nType)
{
	EM_DEBUG_FUNC_BEGIN();
	memset(m_pSendBuf, 0x00, sizeof(ipcMsgbuf));
	m_pSendBuf->mtype = a_nType;
	memcpy(m_pSendBuf->mtext, a_pMsg, a_nSize);
	int nRet;
	EM_DEBUG_LOG("===========================================================");
	EM_DEBUG_LOG("[cmSysMsgQueue::MsgSnd][m_nMsgqid = %d]", m_nMsgqid);
	EM_DEBUG_LOG("[cmSysMsgQueue::MsgSnd][a_nSize = %d]", a_nSize);
	EM_DEBUG_LOG("[cmSysMsgQueue::MsgSnd][a_nType = %d]", a_nType);
	EM_DEBUG_LOG("===========================================================");
	nRet = msgsnd(m_nMsgqid, m_pSendBuf, IPC_MSGQ_SIZE, 0);		/*  0 값 말고 IPC_NOWAIT */

	EM_DEBUG_LOG("=====================================================");
	EM_DEBUG_LOG("[cmSysMsgQueue::MsgSnd][ret = %d]", nRet);
	EM_DEBUG_LOG("=====================================================");
	return nRet;
}

int cmSysMsgQueue::MsgRcv(void *a_pMsg, int a_nSize)
{
	int nRet;
	EM_DEBUG_LOG("===========================================================");
	EM_DEBUG_LOG("[cmSysMsgQueue::MsgRcv][m_nMsgqid = %d]", m_nMsgqid);
	EM_DEBUG_LOG("[cmSysMsgQueue::MsgRcv][m_nMsgType = %d]", m_nMsgType);
	EM_DEBUG_LOG("===========================================================");
	nRet = msgrcv(m_nMsgqid, m_pRecvBuf, IPC_MSGQ_SIZE, m_nMsgType, 0);
	if (nRet == -1) {  	EM_DEBUG_LOG("===========================================================");
		EM_DEBUG_LOG("[cmSysMsgQueue::MsgRcv][FAIL]");
		EM_DEBUG_LOG("[cmSysMsgQueue::MsgRcv][error:=%d][errorstr =%s]", errno, strerror(errno));
		EM_DEBUG_LOG("===========================================================");
	}
	memcpy(a_pMsg, m_pRecvBuf->mtext, a_nSize);
	EM_DEBUG_LOG("=====================================================");
	EM_DEBUG_LOG("[cmSysMsgQueue::MsgRcv][ret = %d][mtype = %x][Size = %d]", nRet, m_nMsgType, a_nSize);
	EM_DEBUG_LOG("=====================================================");
	return nRet;
}

bool cmSysMsgQueue::MsgDestroy()
{
	if(msgctl(m_nMsgqid, IPC_RMID, NULL) == -1)
		return false;
	
	
	EM_DEBUG_LOG("=====================================================");
	EM_DEBUG_LOG("[cmSysMsgQueue][MsgDestroy]");
	EM_DEBUG_LOG("=====================================================");
	return true;
}

