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

/******************************************************************************
 * File :  email-network.h
 * Desc :  
 *
 * Auth : 
 *
 * History : 
 *    2011.12.16  :  created
 *****************************************************************************/
#ifndef __EMAIL_NETWORK_H__
#define __EMAIL_NETWORK_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "email-internal-types.h"

INTERNAL_FUNC int emnetwork_check_network_status(int *err_code);
INTERNAL_FUNC int emnetwork_get_roaming_status(int *output_roaming_status);
INTERNAL_FUNC int emnetwork_get_wifi_status(int *wifi_status);
INTERNAL_FUNC long emnetwork_callback_ssl_cert_query(char *reason, char *host, char *cert);
INTERNAL_FUNC void emnetwork_set_network_status(int input_network_status);
INTERNAL_FUNC int emnetwork_get_network_status();


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
