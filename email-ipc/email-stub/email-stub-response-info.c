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

#include "email-stub-response-info.h"

EXPORT_API bool emipc_set_value_in_response_info(emipc_email_response_info *response_info, long response_id, long api_id)
{
	response_info->response_id = response_id;
	response_info->api_id = api_id;
	return true; 
}

EXPORT_API long emipc_get_response_id_in_response_info(emipc_email_response_info *response_info)
{
	return response_info->response_id;
}

EXPORT_API long emipc_get_api_id_in_response_info(emipc_email_response_info *response_info)
{
	return response_info->api_id;
}
