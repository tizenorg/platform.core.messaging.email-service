/* Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of SAMSUNG 
 * ELECTRONICS ("Confidential Information"). You agree and acknowledge that 
 * this software is owned by Samsung and you shall not disclose such 
 * Confidential Information and shall use it only in accordance with the terms
 * of the license agreement you entered into with SAMSUNG ELECTRONICS. 
 * SAMSUNG make no representations or warranties about the suitability of the 
 * software, either express or implied, including but not limited to the 
 * implied warranties of merchantability, fitness for a particular purpose, 
 * or non-infringement. SAMSUNG shall not be liable for any damages suffered 
 * by licensee arising out of or releated to this software.
 * 
 */

#include "include/ipc-library-build.h"
#include "include/ipc-library.h"
#include "proxy/include/ipc-proxy-main.h"
#include "api/include/ipc-api-info.h"
#include "api/include/ipc-param-list.h"
#include "emf-dbglog.h"
#include "ipc-socket.h"

EXPORT_API int ipcEmailProxy_Initialize()
{
	EM_DEBUG_FUNC_BEGIN();

	int err = ipcEmailProxyMain::Instance()->Initialize();

	EM_DEBUG_FUNC_END();
	return err;
}

EXPORT_API int ipcEmailProxy_Finalize()
{
	EM_DEBUG_FUNC_BEGIN();
	return ipcEmailProxyMain::Instance()->Finalize();;
}

EXPORT_API bool ipcEmailProxy_ExecuteAPI(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret;
	int err = EMF_ERROR_NONE;
	ipcEmailAPIInfo *pApi = static_cast<ipcEmailAPIInfo *>(a_hAPI);

	EM_DEBUG_LOG("pApi [%p]", pApi);
		
	if(pApi == NULL) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;
	}
		
	EM_DEBUG_LOG("APIID [%s], ResponseID [%d], APPID[%d]", EM_APIID_TO_STR(pApi->GetAPIID()), pApi->GetResponseID(), pApi->GetAPPID());

	ret = ipcEmailProxyMain::Instance()->ExecuteAPI(pApi);

	/* connection retry */

	if (!ret) {
		ipcEmailProxy_Finalize();

		err = ipcEmailProxy_Initialize();
		if (err != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("Failed to open the socket : [%d]", err);
			return EMF_ERROR_CONNECTION_FAILURE;
		}
	
		ret = ipcEmailProxyMain::Instance()->ExecuteAPI(pApi);

		if (!ret) {
			EM_DEBUG_EXCEPTION("ipcEmailProxyMain::ExecuteAPI failed [%d]", err);
			return EMF_ERROR_CONNECTION_FAILURE;
		}
	}
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;	
}
