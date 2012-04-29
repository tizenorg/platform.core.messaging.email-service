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



#ifndef	_IPC_STUB_MAIN_H_
#define	_IPC_STUB_MAIN_H_

#include "email-ipc.h"
#include "email-ipc-api-info.h"

EXPORT_API bool emipc_initialize_stub_main(PFN_EXECUTE_API fn_api_mapper);

EXPORT_API bool emipc_finalize_stub_main();

EXPORT_API bool emipc_execute_api_proxy_to_stub(emipc_email_api_info *api_info);

EXPORT_API bool emipc_execute_api_stub_to_proxy(emipc_email_api_info *api_info);

EXPORT_API bool emipc_set_response_info(long app_id, long api_id);

#endif	/* _IPC_STUB_MAIN_H_ */


