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
 * File :  em-core-global.c
 * Desc :  Mail Engine Global
 *
 * Auth :  Kyuho Jo 
 *
 * History : 
 *    2010.08.25  :  created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emf-types.h"



emf_account_t g_new_account;
emf_account_list_t *g_account_list = NULL;
int g_account_num = 0;
int g_account_retrieved = 0;

EXPORT_API emf_account_t *em_core_account_get_new_account_ref()
{
	return &g_new_account;
}





