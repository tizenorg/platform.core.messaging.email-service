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

/******************************************************************************
 * File :  em-network.h
 * Desc :  
 *
 * Auth : 
 *
 * History : 
 *    2011.12.16  :  created
 *****************************************************************************/
#ifndef __EM_NETWORK_H__
#define __EM_NETWORK_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "emf-types.h"

EXPORT_API long em_core_ssl_cert_query_cb(char *reason, char *host, char *cert);
EXPORT_API int em_core_check_network_status(int *err_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
