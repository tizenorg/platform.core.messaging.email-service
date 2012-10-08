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
#include <glib.h>

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
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

