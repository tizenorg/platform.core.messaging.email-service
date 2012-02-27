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
#include <stdlib.h>
#include "ipc-library-build.h"
#include "ipc-param-list.h"


ipcEmailParamList::ipcEmailParamList()
{
	EM_DEBUG_FUNC_BEGIN();
	m_nParamCount = 0;
	m_pbyteStream = NULL;
}

ipcEmailParamList::~ipcEmailParamList()
{
	EM_DEBUG_FUNC_BEGIN();
	EM_SAFE_FREE(m_pbyteStream);
	EM_DEBUG_FUNC_END();
}

/*  stream */
/*  +----------------------------------------------------------------------------------------------------------+ */
/*  | API ID(4B) | Resp. ID (4B) | Param Count(4B) | Param1 Len | Param1 Data | ... | ParamN Len | ParamN data |  */
/*  +----------------------------------------------------------------------------------------------------------+ */
bool ipcEmailParamList::ParseStream(void* a_pStreamData)
{
	EM_DEBUG_FUNC_BEGIN();
	long nParameterCount = ParseParameterCount(a_pStreamData);
	if(nParameterCount <= 0) {
		EM_DEBUG_EXCEPTION("There is no parameter. ");
		return false;
	}

	unsigned char* pStream = (unsigned char*)a_pStreamData;
	long nIndex, nParamLen, nPos = sizeof(long)*eSTREAM_DATA;
/* 	_pal_ITrace_IPCPrintF("[IPCLib] Parameter Count [%x]  ", nParameterCount); */

	for(nIndex = 0; nIndex < nParameterCount; nIndex++) {
/* 		unsigned char szLen[5] = {NULL,}; */
/* 		memcpy(szLen, pStream+nPos, sizeof(long)); */
/* 		nParamLen = *(long*)(szLen); */

		long nLen =0;
		memcpy(&nLen, pStream+nPos, sizeof(long));
		nParamLen = nLen;
		EM_DEBUG_LOG("Parameter Length [%d] : %d ", nIndex, nParamLen);
		nPos += sizeof(long);	/*  Move from length position to data position */

		AddParam((void*)(pStream+nPos), nParamLen);
		nPos += nParamLen;		/*  move to next parameter	 */
	}
	
	return true;
}

void* ipcEmailParamList::GetStream()
{
	EM_DEBUG_FUNC_BEGIN();
	if(m_pbyteStream)
		return m_pbyteStream;
	
	int nStreamLen = GetStreamLength();
	
	if ( nStreamLen > 0 )  {
		m_pbyteStream = (unsigned char*) calloc(1, nStreamLen);
		int nPos = sizeof(long)*eSTREAM_COUNT;

		if ( nPos + (int)sizeof(m_nParamCount) > nStreamLen ) {
			EM_DEBUG_EXCEPTION("%d > nStreamLen", nPos + sizeof(m_nParamCount));			
			EM_SAFE_FREE(m_pbyteStream);
			return NULL;
		}
		
		memcpy((void*)(m_pbyteStream+nPos), (void*)&m_nParamCount, sizeof(m_nParamCount));
		
		nPos += sizeof(long);
		int nIndex = 0, nLength = 0;

		/*   check memory overflow */
		for(nIndex=0; nIndex<m_nParamCount; nIndex++) {
			nLength = m_lstParams[nIndex].GetLength();
			if ( nLength <= 0 ) {
			 	EM_DEBUG_EXCEPTION("nLength = %d", nIndex, nLength);			
				EM_SAFE_FREE(m_pbyteStream);
				return NULL;
			}

			if ( nPos + (int)sizeof(long) > nStreamLen ) {
			 	EM_DEBUG_EXCEPTION("%d > nStreamLen", nPos + sizeof(long));			
				EM_SAFE_FREE(m_pbyteStream);
				return NULL;
			}
			memcpy((void*)(m_pbyteStream+nPos), (void*)&nLength, sizeof(long));
			nPos += sizeof(long);

			if ( nPos + nLength > nStreamLen ) {
				EM_DEBUG_EXCEPTION("%d > nStreamLen", nPos + nLength);			
				EM_SAFE_FREE(m_pbyteStream);
				return NULL;
			}
			
			memcpy((void*)(m_pbyteStream+nPos), (void*)m_lstParams[nIndex].GetData(), nLength);
			nPos += nLength;
		}
		return (void*)m_pbyteStream;
	}
	
	EM_DEBUG_EXCEPTION("failed.");			
	EM_SAFE_FREE(m_pbyteStream);

	EM_DEBUG_FUNC_END();
	return NULL;
}


int ipcEmailParamList::GetStreamLength()
{
	/* if(m_lstParams ) // prevent 20071030 : m_lstParams is array */ {
		int nLength = sizeof(long)*eSTREAM_DATA;	/* API ID, Response ID, Param count */
		int nIndex;
		for(nIndex=0; nIndex<m_nParamCount; nIndex++) {
			nLength += sizeof(long);					/* Length size */
			nLength += m_lstParams[nIndex].GetLength();	/* data size */
		}
		return nLength;
	}
	return 0;	
}

bool ipcEmailParamList::AddParam(void* a_pData, int a_nLen)
{
	EM_DEBUG_FUNC_BEGIN();
	
	if(m_lstParams[m_nParamCount].SetParam(a_pData, a_nLen)) {
		m_nParamCount++;
		EM_SAFE_FREE(m_pbyteStream);
		return true;
	}

	return false;
	
}

void* ipcEmailParamList::GetParam(int a_nIndex)
{
	EM_DEBUG_FUNC_BEGIN("a_nIndex [%d]", a_nIndex);
	if(a_nIndex < 0 || a_nIndex >= m_nParamCount) {
		EM_DEBUG_EXCEPTION("Index value is not valid ");
		return NULL;
	}

	return (void*)m_lstParams[a_nIndex].GetData();
}

int ipcEmailParamList::GetParamLen(int a_nIndex)
{
	EM_DEBUG_FUNC_BEGIN();
	if(a_nIndex < 0 || a_nIndex >= m_nParamCount) {
		EM_DEBUG_EXCEPTION("Index value is not valid ");
		return 0;
	}
	
	return m_lstParams[a_nIndex].GetLength();
}

int ipcEmailParamList::GetParamCount()
{
	EM_DEBUG_FUNC_BEGIN("Paramter count [%d]  ", m_nParamCount);
	return m_nParamCount;
}

long ipcEmailParamList::ParseParameterCount(void *a_pStream)
{
	EM_DEBUG_FUNC_BEGIN();
	long *pParameterCountPostion = (long*)a_pStream + eSTREAM_COUNT;

	return *pParameterCountPostion;
}

