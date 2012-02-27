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


#ifndef _IPC_PARAMLIST_H_
#define _IPC_PARAMLIST_H_


#include "ipc-param.h"
#include "emf-dbglog.h"

typedef enum {
	eSTREAM_APIID,
	eSTREAM_RESID,
	eSTREAM_APPID,
	eSTREAM_COUNT,
	eSTREAM_DATA,
	eSTREAM_MAX
	
}IPC_STREAM_INFO;

class ipcEmailParamList {
public:
	ipcEmailParamList();
	virtual ~ipcEmailParamList();
	
	bool	ParseStream(void* a_pStreamData);
	void*	GetStream();
	int 	GetStreamLength();

	bool	AddParam(void* a_pData, int a_nLen);

	void*	GetParam(int a_nIndex);
	int 	GetParamLen(int a_nIndex);
	int 	GetParamCount();

private:
	long	ParseParameterCount(void *a_pStream);
	
private:
	int m_nParamCount;
	ipcEmailParam m_lstParams[10];
	unsigned char *m_pbyteStream;

};

#endif	/* _IPC_PARAMLIST_H_ */


