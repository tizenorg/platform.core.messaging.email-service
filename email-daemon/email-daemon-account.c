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
 * File: emf-account.c
 * Desc: email-daemon Account
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

#include "email-daemon.h"
#include "email-storage.h"
#include "c-client.h"
#include "email-debug-log.h"
#include "email-daemon-account.h"
#include "email-daemon-auto-poll.h"
#include <contacts.h>
#include <contacts_internal.h>
#include "email-types.h"
#include "email-core-account.h"
#include "email-core-event.h"
#include "email-core-utils.h"
#include "email-core-imap-idle.h"
#include "email-utilities.h"
#include "email-convert.h"

static int emdaemon_check_filter_id(char *multi_user_name, int filter_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d], err_code[%p]", filter_id, err_code);

	if (filter_id  <= 0) {		/*  only global rule supported. */
		EM_DEBUG_EXCEPTION("filter_id[%d]", filter_id);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_rule_tbl_t* filter = NULL;

	if (!emstorage_get_rule_by_id(multi_user_name, filter_id, &filter, true, &err)) {
		EM_DEBUG_EXCEPTION(" emstorage_get_rule_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (filter != NULL)
		emstorage_free_rule(&filter, 1, NULL);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int emdaemon_create_account(char *multi_user_name, email_account_t* account, int* err_code)
{
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!emcore_create_account(multi_user_name, account, false, &err)) {
		EM_DEBUG_EXCEPTION(" emcore_account_add failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int emdaemon_delete_account(char *multi_user_name, int account_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d] err_code[%p]", account_id, err_code);
	int ret = false;

	ret = emcore_delete_account(multi_user_name, account_id, false, err_code);

	EM_DEBUG_FUNC_END("ret[%d]", ret);
	return ret;
}

static email_account_t* duplicate_account(email_account_t *src)
{
	if(!src) {
		EM_DEBUG_EXCEPTION("INVALID_PARAM");
		return NULL;
	}
	email_account_t *dst = (email_account_t *)em_malloc(sizeof(email_account_t));
	if (!dst) {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		return NULL;
	}

	/* Need deep copy */
	memcpy(dst, src, sizeof(email_account_t));
	dst->account_name              = EM_SAFE_STRDUP(src->account_name);
	dst->incoming_server_address   = EM_SAFE_STRDUP(src->incoming_server_address);
	dst->user_email_address        = EM_SAFE_STRDUP(src->user_email_address);
	dst->incoming_server_user_name = EM_SAFE_STRDUP(src->incoming_server_user_name);
	dst->incoming_server_password  = EM_SAFE_STRDUP(src->incoming_server_password);
	dst->outgoing_server_address   = EM_SAFE_STRDUP(src->outgoing_server_address);
	dst->outgoing_server_user_name = EM_SAFE_STRDUP(src->outgoing_server_user_name);
	dst->outgoing_server_password  = EM_SAFE_STRDUP(src->outgoing_server_password);
	dst->user_display_name         = EM_SAFE_STRDUP(src->user_display_name);
	dst->reply_to_address          = EM_SAFE_STRDUP(src->reply_to_address);
	dst->return_address            = EM_SAFE_STRDUP(src->return_address);
	dst->logo_icon_path            = EM_SAFE_STRDUP(src->logo_icon_path);
	dst->certificate_path          = EM_SAFE_STRDUP(src->certificate_path);
	dst->options.display_name_from = EM_SAFE_STRDUP(src->options.display_name_from);
	dst->options.signature 	       = EM_SAFE_STRDUP(src->options.signature);
	dst->user_data                = (void*) em_malloc(src->user_data_length);
	if( !dst->user_data ) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		emcore_free_account(dst);
		EM_SAFE_FREE(dst);
		return NULL;
	}

	memcpy(dst->user_data, src->user_data, src->user_data_length);

	return dst;
}

INTERNAL_FUNC int emdaemon_validate_account(char *multi_user_name, int account_id, int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], handle[%p], err_code[%p]", account_id, handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;
	email_account_t *ref_account = NULL;

	if (account_id < 1) {
		EM_DEBUG_EXCEPTION(" account_id[%d]", account_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!(ref_account = emcore_get_account_reference(multi_user_name, account_id, false))) {
		EM_DEBUG_EXCEPTION(" emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	event_data->type = EMAIL_EVENT_VALIDATE_ACCOUNT;
	event_data->event_param_data_1 = NULL;
	event_data->event_param_data_3 = NULL;
	event_data->account_id = account_id;
    event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event(event_data, (int*)handle, &err)) {
		EM_DEBUG_EXCEPTION(" emcore_insert_event falied [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emdaemon_validate_account_ex(char *multi_user_name, email_account_t* input_account, int *output_handle)
{
	EM_DEBUG_FUNC_BEGIN("input_account[%p], output_handle[%p]", input_account, output_handle);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;

	event_data = em_malloc(sizeof(email_event_t));

	if(!event_data) { /*prevent 53095*/
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type               = EMAIL_EVENT_VALIDATE_ACCOUNT_EX;
	event_data->event_param_data_1 = (void*)input_account;
	event_data->event_param_data_3 = NULL;
	event_data->account_id         = NEW_ACCOUNT_ID;
    event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event(event_data, (int*)output_handle, &err)) {
		EM_DEBUG_EXCEPTION(" emcore_insert_event falied [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	EM_DEBUG_FUNC_END("err [%d]", err);

	return err;
}

INTERNAL_FUNC int emdaemon_validate_account_and_create(char *multi_user_name, email_account_t* new_account, int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], handle[%p], err_code[%p]", new_account, handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;

	event_data = em_malloc(sizeof(email_event_t));

	if(!event_data) { /*prevent 53093*/
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type               = EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT;
	event_data->event_param_data_1 = (void *)new_account;
	event_data->event_param_data_3 = NULL;
	event_data->account_id         = NEW_ACCOUNT_ID;
    event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event(event_data, (int*)handle, &err))  {
		EM_DEBUG_EXCEPTION(" emcore_insert_event falied [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}


INTERNAL_FUNC int emdaemon_update_account(char *multi_user_name, int account_id, email_account_t* new_account, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], new_account[%p], err_code[%p]", account_id, new_account, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t *new_account_tbl = NULL;
	email_account_t *old_account_info = NULL;

	if ((account_id <= 0) || !new_account)  {
		EM_DEBUG_EXCEPTION("Invalid Parameters.");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if((old_account_info = emcore_get_account_reference(multi_user_name, account_id, true)) == NULL) {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed ");
		goto FINISH_OFF;
	}

	if(new_account->user_email_address) {
		if ((err = em_verify_email_address (new_account->user_email_address)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_verify_email_address error [%d]", err);
			goto FINISH_OFF;
		}
	}

	if (EM_SAFE_STRCMP(new_account->incoming_server_password, old_account_info->incoming_server_password) == 0) {
		EM_SAFE_FREE(new_account->incoming_server_password);
	}

	if (EM_SAFE_STRCMP(new_account->outgoing_server_password, old_account_info->outgoing_server_password) == 0) {
		EM_SAFE_FREE(new_account->outgoing_server_password);
	}

	new_account_tbl = em_malloc(sizeof(emstorage_account_tbl_t));
	if(!new_account_tbl) {
		EM_DEBUG_EXCEPTION("allocation failed [%d]", err);
		goto FINISH_OFF;
	}

	em_convert_account_to_account_tbl(new_account, new_account_tbl);

	if (!emstorage_update_account(multi_user_name, account_id, new_account_tbl, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_update_account failed [%d]", err);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_AUTO_POLLING__
	int  change_in_auto_polling_option = 0;

	change_in_auto_polling_option = (old_account_info->check_interval  != new_account->check_interval)  ||
									(old_account_info->peak_interval   != new_account->peak_interval)   ||
									(old_account_info->peak_start_time != new_account->peak_start_time) ||
									(old_account_info->peak_end_time   != new_account->peak_end_time);

	EM_DEBUG_LOG("change_in_auto_polling_option [%d]", change_in_auto_polling_option);
#endif


#ifdef __FEATURE_AUTO_POLLING__
	if(change_in_auto_polling_option) {
		if(!emdaemon_remove_polling_alarm(account_id))
			EM_DEBUG_LOG("emdaemon_remove_polling_alarm failed");

		if(!emdaemon_add_polling_alarm(multi_user_name, account_id))
			EM_DEBUG_EXCEPTION("emdaemon_add_polling_alarm failed");

#ifdef __FEATURE_IMAP_IDLE__
		emcore_refresh_imap_idle_thread();
#endif /* __FEATURE_IMAP_IDLE__ */
	}
#endif

	ret = true;

FINISH_OFF:

	if(new_account_tbl)
		emstorage_free_account(&new_account_tbl, 1, NULL);

	if (old_account_info) {
		emcore_free_account(old_account_info);
		EM_SAFE_FREE(old_account_info);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_validate_account_and_update(char *multi_user_name, int old_account_id, email_account_t* new_account_info, int *handle,int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account[%d], new_account_info[%p], handle[%p], err_code[%p]", old_account_id, new_account_info, handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;

	event_data = em_malloc(sizeof(email_event_t));

	if(!event_data) { /*prevent 53094*/
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type = EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT;
	event_data->event_param_data_1 = (char *) duplicate_account(new_account_info);
	event_data->event_param_data_3 = NULL;
	event_data->account_id = old_account_id;
	event_data->multi_user_name    = EM_SAFE_STRDUP(multi_user_name);

#if 0
	email_account_t *pAccount = (email_account_t *)em_malloc(sizeof(email_account_t));
	if (pAccount == NULL) {
		EM_DEBUG_EXCEPTION(" malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	/* Need deep copy */
	memcpy(pAccount, new_account_info, sizeof(email_account_t));
	pAccount->account_name              = EM_SAFE_STRDUP(new_account_info->account_name);
	pAccount->incoming_server_address   = EM_SAFE_STRDUP(new_account_info->incoming_server_address);
	pAccount->user_email_address        = EM_SAFE_STRDUP(new_account_info->user_email_address);
	pAccount->incoming_server_user_name = EM_SAFE_STRDUP(new_account_info->user_email_address);
	pAccount->incoming_server_password  = EM_SAFE_STRDUP(new_account_info->incoming_server_password);
	pAccount->outgoing_server_address   = EM_SAFE_STRDUP(new_account_info->incoming_server_password);
	pAccount->outgoing_server_user_name = EM_SAFE_STRDUP(new_account_info->outgoing_server_user_name);
	pAccount->outgoing_server_password  = EM_SAFE_STRDUP(new_account_info->outgoing_server_password);
	pAccount->user_display_name         = EM_SAFE_STRDUP(new_account_info->user_display_name);
	pAccount->reply_to_address          = EM_SAFE_STRDUP(new_account_info->reply_to_address);
	pAccount->return_address            = EM_SAFE_STRDUP(new_account_info->return_address);
	pAccount->logo_icon_path            = EM_SAFE_STRDUP(new_account_info->logo_icon_path);
	pAccount->certificate_path 			= EM_SAFE_STRDUP(new_account_info->certificate_path);
	pAccount->options.display_name_from = EM_SAFE_STRDUP(new_account_info->options.display_name_from);
	pAccount->options.signature 		= EM_SAFE_STRDUP(new_account_info->options.signature);
	memcpy(pAccount->user_data, new_account_info->user_data, new_account_info->user_data_length);
#endif

	if (!emcore_insert_event(event_data, (int*)handle, &err)) {
		EM_DEBUG_EXCEPTION("emcore_insert_event falied [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}


INTERNAL_FUNC int emdaemon_get_account(char *multi_user_name, int account_id, int pulloption, email_account_t* account, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], pulloption [%d], account[%p], err_code[%p]", account_id, pulloption, account, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t *account_tbl = NULL;

	if (!account)  {
		EM_DEBUG_EXCEPTION("account_id[%d], account[%p]", account_id, account);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_account_by_id(multi_user_name, account_id, pulloption, &account_tbl, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	em_convert_account_tbl_to_account(account_tbl, account);

	ret = true;

FINISH_OFF:
	if(account_tbl)
		emstorage_free_account(&account_tbl, 1, NULL);
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int emdaemon_get_account_list(char *multi_user_name, email_account_t** account_list, int* count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_list[%p], count[%p], err_code[%p]", account_list, count, err_code);

	int ret = false, err = EMAIL_ERROR_NONE, i = 0;
	emstorage_account_tbl_t *account_tbl_array = NULL;

	if (!account_list || !count)  {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	*count = 1000;

	if (!emstorage_get_account_list(multi_user_name, count, &account_tbl_array, true, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if(account_tbl_array && (*count) > 0) {
		*account_list = (email_account_t*)em_malloc(sizeof(email_account_t) * (*count));
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
		emstorage_free_account(&account_tbl_array, (*count), NULL);

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emdaemon_free_account(email_account_t** account_list, int count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	return emcore_free_account_list(account_list, count, err_code);
}


INTERNAL_FUNC int emdaemon_get_filter(char *multi_user_name, int filter_id, email_rule_t** filter_info, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d], filter_info[%p], err_code[%p]", filter_id, filter_info, err_code);

	if (!filter_info) {
		EM_DEBUG_EXCEPTION("filter_id[%d], filter_info[%p]", filter_id, filter_info);

		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!emstorage_get_rule_by_id(multi_user_name, filter_id, (emstorage_rule_tbl_t**)filter_info, true, &err))  {
		EM_DEBUG_EXCEPTION(" emstorage_get_rule_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_get_filter_list(char *multi_user_name, email_rule_t** filter_info, int* count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p], count[%p], err_code[%p]", filter_info, count, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int is_completed;

	if (!filter_info || !count)  {
		EM_DEBUG_EXCEPTION(" filter_info[%p], count[%p]", filter_info, count);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	*count = 1000;

	if (!emstorage_get_rule(multi_user_name, ALL_ACCOUNT, 0, 0, count, &is_completed, (emstorage_rule_tbl_t**)filter_info, true, &err))  {
		EM_DEBUG_EXCEPTION(" emstorage_get_rule failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_find_filter(char *multi_user_name, email_rule_t* filter_info, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p], err_code[%p]", filter_info, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!filter_info) {
		EM_DEBUG_EXCEPTION(" filter_info[%p]", filter_info);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (filter_info->faction == EMAIL_FILTER_MOVE && filter_info->target_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("filter_info->faction[%d], filter_info->target_mailbox_id[%d]", filter_info->faction, filter_info->target_mailbox_id);
		err = EMAIL_ERROR_INVALID_FILTER;
		goto FINISH_OFF;
	}

	filter_info->account_id = ALL_ACCOUNT;		/*  MUST BE */

	if (!emstorage_find_rule(multi_user_name, (emstorage_rule_tbl_t*)filter_info, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_find_rule failed [%d]", err);
		goto FINISH_OFF;
	} else {
		if (err == EMAIL_ERROR_FILTER_NOT_FOUND) {
			EM_DEBUG_LOG("EMAIL_ERROR_FILTER_NOT_FOUND");
			err = EMAIL_ERROR_FILTER_NOT_FOUND;
			goto FINISH_OFF;
		}
	}

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_add_filter(char *multi_user_name, email_rule_t* filter_info)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p]", filter_info);

	/*  default variable */
	int err = EMAIL_ERROR_NONE;
	if (!filter_info) {
		EM_DEBUG_EXCEPTION("filter_info[%p]", filter_info);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	err = emcore_add_rule(multi_user_name, filter_info);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_rule failed : [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int emdaemon_update_filter(char *multi_user_name, int filter_id, email_rule_t* filter_info, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d], filter_info[%p], err_code[%p]", filter_id, filter_info, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if ((filter_id <= 0) || !filter_info)  {
		EM_DEBUG_EXCEPTION("filter_id[%d], filter_info[%p]", filter_id, filter_info);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emdaemon_check_filter_id(multi_user_name, filter_id, &err))  {
		EM_DEBUG_EXCEPTION("emdaemon_check_filter_id falied [%d]", err);
		goto FINISH_OFF;
	}

        err = emcore_update_rule(multi_user_name, filter_id, filter_info);
        if (err != EMAIL_ERROR_NONE) {
            EM_DEBUG_EXCEPTION("emcore_update_rule failed : [%d]", err);
            goto FINISH_OFF;
        }

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_delete_filter(char *multi_user_name, int filter_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d, err_code[%p]", filter_id, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (filter_id <= 0)  {
		EM_DEBUG_EXCEPTION(" fliter_id[%d]", filter_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	err = emcore_delete_rule(multi_user_name, filter_id);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_delete_rule failed : [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_free_filter(email_rule_t** filter_info, int count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_info[%p], count[%d], err_code[%p]", filter_info, count, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (count > 0)  {
		if (!filter_info || !*filter_info)  {
			EM_DEBUG_EXCEPTION(" filter_info[%p], count[%d]", filter_info, count);
			err = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}

		email_rule_t* p = *filter_info;
		int i;

		for (i = 0; i < count; i++)  {
			EM_SAFE_FREE(p[i].value);
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

INTERNAL_FUNC int emdaemon_apply_filter(char *multi_user_name, int filter_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("filter_id[%d, err_code[%p]", filter_id, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_rule_tbl_t *filter_info = NULL;

	if (filter_id <= 0)  {
		EM_DEBUG_EXCEPTION(" fliter_id[%d]", filter_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_rule_by_id(multi_user_name, filter_id, &filter_info, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_rule_by_id failed : [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_mail_filter_by_rule(multi_user_name, (email_rule_t *)filter_info, &err))  {
		EM_DEBUG_EXCEPTION(" emstorage_mail_filter_by_rule failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


/* ----- internal functions --------------------------------------------*/

INTERNAL_FUNC int emdaemon_insert_accountinfo_to_contact(email_account_t* account)
{
	EM_DEBUG_FUNC_BEGIN();

	if(!account)
		return false;

	int ret = false;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_update_accountinfo_to_contact(email_account_t* old_account, email_account_t* new_account)
{
	EM_DEBUG_FUNC_BEGIN();

	if(!old_account || !new_account)
		return false;

	int ret = false;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emdaemon_query_smtp_mail_size_limit(char *multi_user_name, int account_id, int *handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], handle[%p], err_code[%p]", account_id, handle, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	email_event_t *event_data = NULL;

	if (account_id < 1) {
		EM_DEBUG_EXCEPTION(" account_id[%d]", account_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	if (event_data == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type = EMAIL_EVENT_QUERY_SMTP_MAIL_SIZE_LIMIT;
	event_data->event_param_data_1 = NULL;
	event_data->event_param_data_3 = NULL;
	event_data->account_id = account_id;
	event_data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

	if (!emcore_insert_event(event_data, (int*)handle, &err)) {
		EM_DEBUG_EXCEPTION(" emcore_insert_event falied [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

/* EOF */
