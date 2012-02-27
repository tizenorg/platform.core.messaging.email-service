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
 * File :  em-core-global.h
 * Desc :  Mail Engine Global Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.01  :  created
 *****************************************************************************/
#ifndef __EM_CORE_GLOBAL_H__
#define __EM_CORE_GLOBAL_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <glib.h>
#include "vconf-keys.h"
#include "emf-types.h"

extern emf_account_list_t *g_account_list;
extern int g_account_num;
extern int g_account_retrieved;

EXPORT_API emf_account_t *em_core_account_get_new_account_ref();
extern char *strcasestr (__const char *__haystack, __const char *__needle) __THROW __attribute_pure__ __nonnull ((1, 2));

extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind) __THROW __nonnull ((1));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
