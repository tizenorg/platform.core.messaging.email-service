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
 * File: emf-account.c
 * Desc: Mail Framework Account
 *
 * Auth:
 *
 * History:
 *    2006.08.16 : created
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vconf.h>

#include "emflib.h"
#include "em-storage.h"
#include "emf-global.h"
#include "c-client.h"
#include "emf-dbglog.h"
#include "emf-account.h"
#include <contacts-svc.h>
#include "emf-types.h"
#include "em-core-account.h" 
#include "em-core-event.h" 
#include "em-core-utils.h"
#include "Msg_Convert.h"

static int emf_refresh_account_reference()
{
	EM_DEBUG_FUNC_BEGIN();
	return em_core_refresh_account_reference();
}

static int emf_filter_check_filter_id(int account_id, int filter_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], filter_id[%d], err_code[%p]", account_id, filter_id, err_code);
	
	if (account_id != ALL_ACCOUNT) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION(" account_id[%d], filter_id[%d]", account_id, filter_id);
		
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_mail_rule_tbl_t* filter = NULL;
	
	if (!em_storage_get_rule_by_id(account_id, filter_id, &filter, true, &err)) {
		EM_DEBUG_EXCEPTION(" em_storage_get_rule_by_id failed [%d]", err);
		
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (filter != NULL)
		em_storage_free_rule(&filter, 1, NULL);
	
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


EXPORT_API int emf_account_create(emf_account_t* account, int* err_code)
{
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	switch (account->account_bind_type)  {
		case EMF_BIND_TYPE_DISABLE: 
		case EMF_BIND_TYPE_EM_CORE:
			if (!em_core_account_create(account, &err)) {
				EM_DEBUG_EXCEPTION(" em_core_account_add failed [%d]", err);
				goto FINISH_OFF;
			}
			emf_refresh_account_reference();
			break;
		default:
			EM_DEBUG_EXCEPTION(" unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
	}
			
	ret = true; 			
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret; 
}


EXPORT_API int emf_account_delete(int account_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret;
	
	ret = em_core_account_delete(account_id, err_code);
	EM_DEBUG_FUNC_END();
	return ret;
}
	
	
EXPORT_API int emf_account_validate(int account_id, unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], handle[%p], err_code[%p]", account_id, handle, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;

	if (account_id < 1)  {
		EM_DEBUG_EXCEPTION(" account_id[%d]", account_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	emf_event_t event_data = {0};
	emf_account_t* ref_account = NULL;
	
	if (!(ref_account = emf_get_account_reference(account_id))) {
		EM_DEBUG_EXCEPTION(" emf_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE:
			event_data.type = EMF_EVENT_VALIDATE_ACCOUNT;
			event_data.event_param_data_1 = NULL;
			event_data.event_param_data_3 = NULL;
			event_data.account_id = account_id;
						
			if (!em_core_insert_event(&event_data, (int*)handle, &err))  {
				EM_DEBUG_EXCEPTION(" em_core_insert_event falied [%d]", err);			
		goto FINISH_OFF;
	}
			break;
			
		default:
			EM_DEBUG_EXCEPTION(" unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


EXPORT_API int emf_account_validate_and_create(emf_account_t* new_account, unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], handle[%p], err_code[%p]", new_account, handle, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_event_t event_data = {0};
	
	switch (new_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE:
			event_data.type = EMF_EVENT_VALIDATE_AND_CREATE_ACCOUNT;
			event_data.event_param_data_1 = NULL;
			event_data.event_param_data_3 = NULL;
			event_data.account_id = NEW_ACCOUNT_ID;
	
			if (!em_core_insert_event(&event_data, (int*)handle, &err))  {
				EM_DEBUG_EXCEPTION(" em_core_insert_event falied [%d]", err);			
				goto FINISH_OFF;
			}
			break;
		default:
			EM_DEBUG_EXCEPTION(" unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


EXPORT_API int emf_account_modify(int account_id, emf_account_t* new_account, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], new_account[%p], err_code[%p]", account_id, new_account, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_mail_account_tbl_t *new_account_tbl = NULL;
	
	if ((account_id <= 0) || !new_account)  {
		EM_DEBUG_EXCEPTION("Invalid Parameters.");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("new_account->email_addr[%s]", new_account->email_addr);
	if(new_account->email_addr) {
		if (!em_core_verify_email_address(new_account->email_addr, true, &err)) {
			err = EMF_ERROR_INVALID_ADDRESS;
			EM_DEBUG_EXCEPTION("Invalid Email Address");
			goto FINISH_OFF;
		}
	}

	new_account_tbl = em_core_malloc(sizeof(emf_mail_account_tbl_t));
	if(!new_account_tbl) {
		EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
		goto FINISH_OFF;
	}

	em_convert_account_to_account_tbl(new_account, new_account_tbl);
	
	if (!em_storage_update_account(account_id, new_account_tbl, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_update_account falied [%d]", err);
		goto FINISH_OFF;
	}
	
	emf_refresh_account_reference();
	
	ret = true;
	
FINISH_OFF:
	if(new_account_tbl)
		em_storage_free_account(&new_account_tbl, 1, NULL);
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_account_validate_and_update(int old_account_id, emf_account_t* new_account_info, unsigned* handle,int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%d], new_account_info[%p], handle[%p], err_code[%p]", old_account_id, new_account_info, handle, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_event_t event_data = {0};
	
	switch (new_account_info->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE:
			event_data.type = EMF_EVENT_VALIDATE_AND_UPDATE_ACCOUNT;
			event_data.event_param_data_3 = NULL;
			event_data.account_id = old_account_id;
	
			emf_account_t *pAccount = (emf_account_t *)malloc(sizeof(emf_account_t));
			if (pAccount == NULL) {
				EM_DEBUG_EXCEPTION(" malloc failed...");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			memcpy(pAccount, new_account_info, sizeof(emf_account_t));
			event_data.event_param_data_1 = (char *) pAccount;

			if (!em_core_insert_event(&event_data, (int*)handle, &err)) {
				EM_DEBUG_EXCEPTION("em_core_insert_event falied [%d]", err);			
				goto FINISH_OFF;
			}

			break;
		default:
			EM_DEBUG_EXCEPTION("unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret; 
}


EXPORT_API int emf_account_get(int account_id, int pulloption, emf_account_t** account, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], pulloption [%d], account[%p], err_code[%p]", account_id, pulloption, account, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_mail_account_tbl_t *account_tbl = NULL;
	
	if (!account)  {
		EM_DEBUG_EXCEPTION("account_id[%d], account[%p]", account_id, account);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!em_storage_get_account_by_id(account_id, pulloption, &account_tbl, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_account_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	*account = em_core_malloc(sizeof(emf_account_t));
	if(!*account) {
		EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
		goto FINISH_OFF;
	}
	em_convert_account_tbl_to_account(account_tbl, *account);
	
	ret = true;
	
FINISH_OFF:
	if(account_tbl)
		em_storage_free_account(&account_tbl, 1, NULL);
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int emf_account_get_list(emf_account_t** account_list, int* count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%p], err_code[%p]", account_list, count, err_code);
	
	int ret = false, err = EMF_ERROR_NONE, i = 0;
	emf_mail_account_tbl_t *account_tbl_array = NULL;
	
	if (!account_list || !count)  {
		EM_DEBUG_EXCEPTION("account_list[%p], count[%p]", account_list, (*count));
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	*count = 1000;
	
	if (!em_storage_get_account_list(count, &account_tbl_array, true, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if(account_tbl_array && (*count) > 0) {
		*account_list = (emf_account_t*)em_core_malloc(sizeof(emf_account_t) * (*count));
		if(!*account_list) {
			EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
			goto FINISH_OFF;
		}

		for(i = 0 ; i < (*count); i++)
			em_convert_account_tbl_to_account(account_tbl_array + i, (*account_list) + i);
	}

	ret = true;

FINISH_OFF:
	if(account_tbl_array)
		em_storage_free_account(&account_tbl_array, (*count), NULL);
	
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


EXPORT_API int emf_account_free(emf_account_t** account_list, int count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	return em_core_account_free(account_list, count, err_code);
}


EXPORT_API int emf_filter_get(int filter_id, emf_rule_t** filter_info, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d], filter_info[%p], err_code[%p]", filter_id, filter_info, err_code);
	
	if (!filter_info) {
		EM_DEBUG_EXCEPTION("filter_id[%d], filter_info[%p]", filter_id, filter_info);
		
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!em_storage_get_rule_by_id(ALL_ACCOUNT, filter_id, (emf_mail_rule_tbl_t**)filter_info, true, &err))  {
		EM_DEBUG_EXCEPTION(" em_storage_get_rule_by_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_filter_get_list(emf_rule_t** filter_info, int* count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p], count[%p], err_code[%p]", filter_info, count, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	int is_completed;
	
	if (!filter_info || !count)  {
		EM_DEBUG_EXCEPTION(" filter_info[%p], count[%p]", filter_info, count);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	*count = 1000;
	
	if (!em_storage_get_rule(ALL_ACCOUNT, 0, 0, count, &is_completed, (emf_mail_rule_tbl_t**)filter_info, true, &err))  {
		EM_DEBUG_EXCEPTION(" em_storage_get_rule failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_filter_find(emf_rule_t* filter_info, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p], err_code[%p]", filter_info, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;

	if (!filter_info)  {
		EM_DEBUG_EXCEPTION(" filter_info[%p]", filter_info);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (filter_info->faction == EMF_FILTER_MOVE && !filter_info->mailbox) {
		EM_DEBUG_EXCEPTION(" filter_info->faction[%d], filter_info->mailbox[%p]", filter_info->faction, filter_info->mailbox);
		err = EMF_ERROR_INVALID_FILTER;	
		goto FINISH_OFF;
	}

	filter_info->account_id = ALL_ACCOUNT;		/*  MUST BE */
	
	if (!em_storage_find_rule((emf_mail_rule_tbl_t*)filter_info, true, &err))  {
		EM_DEBUG_EXCEPTION(" em_storage_find_rule failed [%d]", err);
		err = EMF_ERROR_FILTER_NOT_FOUND;
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_filter_add(emf_rule_t* filter_info, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p], err_code[%p]", filter_info, err_code);
	
	/*  default variable */
	int ret = false, err = EMF_ERROR_NONE;
	if (!filter_info || !(filter_info->value))  {
		EM_DEBUG_EXCEPTION("filter_info[%p]", filter_info);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	/*
	if (filter_info->faction != EMF_FILTER_BLOCK)  {
		EM_DEBUG_EXCEPTION("filter_info->faction[%d] is not supported", filter_info->faction);
		err = EMF_ERROR_NOT_SUPPORTED;
		goto FINISH_OFF;
	}
	
	if (filter_info->faction == EMF_FILTER_MOVE && !filter_info->mailbox)  {
		EM_DEBUG_EXCEPTION("filter_info->faction[%d], filter_info->mailbox[%p]", filter_info->faction, filter_info->mailbox);
		err = EMF_ERROR_INVALID_FILTER;	
		goto FINISH_OFF;
	}
	*/
	
	filter_info->account_id = ALL_ACCOUNT;
	
	if (em_storage_find_rule((emf_mail_rule_tbl_t*)filter_info, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_find_rule failed [%d]", err);
		err = EMF_ERROR_ALREADY_EXISTS;
		goto FINISH_OFF;
	}
	
	if (!em_storage_add_rule((emf_mail_rule_tbl_t*)filter_info, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_add_rule failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!em_core_mail_filter_by_rule((emf_rule_t*)filter_info, &err))  {
		EM_DEBUG_EXCEPTION("em_core_mail_filter_by_rule failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_filter_change(int filter_id, emf_rule_t* filter_info, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d], filter_info[%p], err_code[%p]", filter_id, filter_info, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if ((filter_id <= 0) || !filter_info)  {
		EM_DEBUG_EXCEPTION("filter_id[%d], filter_info[%p]", filter_id, filter_info);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_filter_check_filter_id(ALL_ACCOUNT, filter_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_filter_check_filter_id falied [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!em_storage_change_rule(ALL_ACCOUNT, filter_id, (emf_mail_rule_tbl_t*)filter_info, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_change_rule falied [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_filter_delete(int filter_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d, err_code[%p]", filter_id, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (filter_id <= 0)  {
		EM_DEBUG_EXCEPTION(" fliter_id[%d]", filter_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_filter_check_filter_id(ALL_ACCOUNT, filter_id, &err))  {
		EM_DEBUG_EXCEPTION(" emf_filter_check_filter_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!em_storage_delete_rule(ALL_ACCOUNT, filter_id, true, &err))  {
		EM_DEBUG_EXCEPTION(" em_storage_delete_rule failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_filter_free(emf_rule_t** filter_info, int count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p], count[%d], err_code[%p]", filter_info, count, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (count > 0)  {
		if (!filter_info || !*filter_info)  {
			EM_DEBUG_EXCEPTION(" filter_info[%p], count[%d]", filter_info, count);
			err = EMF_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		
		emf_rule_t* p = *filter_info;
		int i;
		
		for (i = 0; i < count; i++)  {
			EM_SAFE_FREE(p[i].value);
			EM_SAFE_FREE(p[i].mailbox);
		}
		
		EM_SAFE_FREE(p); *filter_info  = NULL;
	}
	
	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


/* ----- internal functions --------------------------------------------*/
int emf_init_account_reference()
{
	EM_DEBUG_FUNC_BEGIN();
	return em_core_init_account_reference();
}

emf_account_t* emf_get_account_reference(int account_id)
{
	EM_DEBUG_FUNC_BEGIN();
	return em_core_get_account_reference(account_id);
}

int emf_free_account_reference()
{
	EM_DEBUG_FUNC_BEGIN();
	return em_core_free_account_reference();
}

EXPORT_API int emf_account_insert_accountinfo_to_contact(emf_account_t* account)
{
	EM_DEBUG_FUNC_BEGIN();

	if(!account)
		return false;

	int ret = false;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_account_update_accountinfo_to_contact(emf_account_t* old_account, emf_account_t* new_account)
{
	EM_DEBUG_FUNC_BEGIN();

	if(!old_account || !new_account)
		return false;

	int ret = false;
	EM_DEBUG_FUNC_END();
	return ret;
}

/* EOF */
