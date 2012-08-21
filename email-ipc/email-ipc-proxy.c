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

#include "email-ipc-build.h"
#include "email-ipc.h"
#include "email-ipc-api-info.h"
#include "email-ipc-param-list.h"
#include "email-ipc-socket.h"
#include "email-proxy-main.h"

#include "email-debug-log.h"
#include "email-api.h"
#include "email-types.h"
#include "email-internal-types.h"

pthread_mutex_t proxy_mutex = PTHREAD_MUTEX_INITIALIZER;

EXPORT_API int emipc_initialize_proxy()
{
	EM_DEBUG_FUNC_BEGIN();

	int err = emipc_initialize_proxy_main();

	EM_DEBUG_FUNC_END();
	return err;
}

EXPORT_API int emipc_finalize_proxy()
{
	EM_DEBUG_FUNC_BEGIN();
	return emipc_finalize_proxy_main();
}

EXPORT_API int emipc_execute_proxy_api(HIPC_API api)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret;
	int err = EMAIL_ERROR_NONE;
	emipc_email_api_info *api_info = (emipc_email_api_info *)api;

	EM_DEBUG_LOG("API [%p]", api_info);
		
	if(api_info == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}
		
	EM_DEBUG_LOG("APIID [%s], ResponseID [%d], APPID[%d]",
				EM_APIID_TO_STR(api_info->api_id), api_info->response_id, api_info->app_id);

	ENTER_CRITICAL_SECTION(proxy_mutex);
	ret = emipc_execute_api_of_proxy_main(api_info);

	/* connection retry */
	if (!ret) {
		EM_DEBUG_LOG("Connection retry");
		emipc_finalize_proxy();

		err = emipc_initialize_proxy();
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("Failed to open the socket : [%d]", err);
			err = EMAIL_ERROR_CONNECTION_FAILURE;
			goto FINISH_OFF;
		}

		ret = emipc_execute_api_of_proxy_main(api_info);
		if (!ret) {
			EM_DEBUG_EXCEPTION("emipc_proxy_main : emipc_execute_api failed [%d]", err);
			err = EMAIL_ERROR_CONNECTION_FAILURE;
			goto FINISH_OFF;
		}
	}
	
FINISH_OFF:
	LEAVE_CRITICAL_SECTION(proxy_mutex);
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;	
}
