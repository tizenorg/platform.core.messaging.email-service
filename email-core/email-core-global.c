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
 * File :  email-core-global.c
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
#include "email-core-global.h"
#include "email-debug-log.h"
#include "email-types.h"
#include "email-utilities.h"
#include "email-core-account.h"

email_account_list_t *g_unvalidated_account_list = NULL;
email_account_list_t *g_account_list = NULL;
int g_account_num = 0;
int g_account_retrieved = 0;

extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind) __THROW __nonnull ((1));

INTERNAL_FUNC int emcore_get_account_from_unvalidated_account_list(int input_unvalidated_account_id, email_account_t **oupput_account)
{
	EM_DEBUG_FUNC_BEGIN("input_unvalidated_account_id[%d], oupput_account[%p]", input_unvalidated_account_id, oupput_account);
	email_account_list_t *account_node = g_unvalidated_account_list;
	int err = EMAIL_ERROR_NONE;

	if(oupput_account == NULL) {
		err = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		goto FINISH_OFF;
	}

	*oupput_account = NULL;

	while(account_node) {
		if(account_node->account->account_id == input_unvalidated_account_id) {
			*oupput_account = account_node->account;
			break;
		}

		account_node = account_node->next;
	}

	if(*oupput_account == NULL)
		err = EMAIL_ERROR_DATA_NOT_FOUND;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_add_account_to_unvalidated_account_list(email_account_t *input_new_account)
{
	EM_DEBUG_FUNC_BEGIN("input_new_account[%p]", input_new_account);
	email_account_list_t **account_node = &g_unvalidated_account_list;
	email_account_list_t *new_account_node = NULL;
	int new_account_id = -1;
	int err = EMAIL_ERROR_NONE;

	if(input_new_account == NULL) {
		err = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		goto FINISH_OFF;
	}

	new_account_node = em_malloc(sizeof(email_account_list_t));

	if(new_account_node == NULL) {
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		goto FINISH_OFF;
	}

	new_account_node->account = input_new_account;
	new_account_node->next = NULL;

	while(*account_node) {
		if((*account_node)->account->account_id < new_account_id) {
			new_account_id = (*account_node)->account->account_id - 1;
		}
		account_node = &((*account_node)->next);
	}

	input_new_account->account_id = new_account_id;
	*account_node = new_account_node;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_delete_account_from_unvalidated_account_list(int input_account_id)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id[%d]", input_account_id);
	email_account_list_t *account_node = g_unvalidated_account_list, *prev_node = NULL;
	email_account_t *found_account = NULL;
	int err = EMAIL_ERROR_NONE;

	while(account_node) {
		if(account_node->account->account_id == input_account_id) {
			found_account = account_node->account;
			if(prev_node)
				prev_node->next = account_node->next;
			else
				g_unvalidated_account_list = account_node->next;
			break;
		}
		prev_node = account_node;
		account_node = account_node->next;
	}

	if(found_account) {
		EM_SAFE_FREE(account_node);
	}
	else {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_DATA_NOT_FOUND");
		err = EMAIL_ERROR_DATA_NOT_FOUND;
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}



