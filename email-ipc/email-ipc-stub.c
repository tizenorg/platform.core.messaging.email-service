/*
*  email-service
*
* Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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


#include "email-ipc.h"
#include "email-ipc-build.h"
#include "email-stub-main.h"

#include "email-debug-log.h"

EXPORT_API bool emipc_initialize_stub(PFN_EXECUTE_API api_mapper)
{
	EM_DEBUG_LOG("[IPCLib] ipcEmailStub_Initialize ");

	return emipc_initialize_stub_main(api_mapper);
}

EXPORT_API bool emipc_finalize_stub()
{
	emipc_finalize_stub_main();
	return true;
}

EXPORT_API bool emipc_execute_stub_api(HIPC_API api)
{
	EM_DEBUG_LOG_DEV("ipcEmailStub_ExecuteAPI [%x]", api);
	emipc_email_api_info *api_info = (emipc_email_api_info *)api;
	if (api_info == NULL)
		return false;

	return emipc_execute_api_stub_to_proxy(api_info);
}

