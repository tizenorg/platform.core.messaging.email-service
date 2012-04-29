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
 * File :  email-core-global.h
 * Desc :  email-core-global Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.01  :  created
 *****************************************************************************/
#ifndef __EMAIL_CORE_GLOBAL_H__
#define __EMAIL_CORE_GLOBAL_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "email-internal-types.h"

extern emf_account_list_t *g_account_list;
extern int g_account_num;
extern int g_account_retrieved;

INTERNAL_FUNC emf_account_t *emcore_get_new_account_reference();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
