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
 * File: emf-etc.c
 * Desc: email-daemon Etc Implementations
 *
 * Auth:
 *
 * History:
 *    2006.08.16 : created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <glib.h>
#include <glib-object.h>
#include <sys/smack.h>
#include <notification.h>
#include <notification_type.h>

#include "email-daemon.h"
#include "email-daemon-account.h"
#include "email-debug-log.h"
#include "email-internal-types.h"
#include "email-core-account.h"
#include "email-core-event.h"
#include "email-core-utils.h"
#include "email-core-alarm.h"
#include "email-core-smtp.h"
#include "email-core-sound.h"
#include "email-utilities.h"
#include "email-storage.h"
#include "email-ipc.h"

extern pthread_mutex_t sound_mutex;
extern pthread_cond_t sound_condition;

int emdaemon_register_event_callback(email_action_t action, email_event_callback callback, void* event_data)
{
	return emcore_register_event_callback(action, callback, event_data);
}

int emdaemon_unregister_event_callback(email_action_t action, email_event_callback callback)
{
	return emcore_unregister_event_callback(action, callback);
}

INTERNAL_FUNC int emdaemon_cancel_job(int account_id, int handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], handle[%d], err_code[%p]", account_id, handle, err_code);
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	
	
	if (!emcore_cancel_thread(handle, NULL, &err))  {
		EM_DEBUG_EXCEPTION("emcore_cancel_thread failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int emdaemon_cancel_sending_mail_job(int account_id, int mail_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], err_code[%p]", account_id, mail_id, err_code);
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;	
	int handle = 0;
	email_account_t* ref_account = NULL;
	
	if (account_id <= 0)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d]", account_id, mail_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

#ifdef __FEATURE_PROGRESS_IN_OUTBOX__

	/*	h.gahlaut@samsung.com: Moved this code from email_cancel_sending_mail API to email-service engine
		since this code has update DB operation which is failing in context of email application process 
		with an sqlite error -> sqlite3_step fail:8 */
		
	/* 	which means #define SQLITE_READONLY   8 */  /* Attempt to write a readonly database */ 
	emstorage_mail_tbl_t *mail_tbl_data = NULL;

	if (!emstorage_get_mail_by_id(mail_id, &mail_tbl_data, false, &err))  {
		EM_DEBUG_EXCEPTION("emcore_get_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	if (mail_tbl_data) {
		if (mail_tbl_data->save_status == EMAIL_MAIL_STATUS_SEND_CANCELED) {
			EM_DEBUG_EXCEPTION(">>>> EMAIL_MAIL_STATUS_SEND_CANCELED Already set for Mail ID [ %d ]", mail_id);
			goto FINISH_OFF;
		}
		else {			
			mail_tbl_data->save_status = EMAIL_MAIL_STATUS_SEND_CANCELED;

			if(!emstorage_set_field_of_mails_with_integer_value(mail_tbl_data->account_id, &mail_id, 1, "save_status", EMAIL_MAIL_STATUS_SEND_CANCELED, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_set_field_of_mails_with_integer_value failed [%d]",err);
				goto FINISH_OFF;
			}
		}


	}

	if ((err = emcore_delete_alram_data_by_reference_id(EMAIL_ALARM_CLASS_SCHEDULED_SENDING, mail_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_LOG("emcore_delete_alram_data_by_reference_id failed [%d]",err);
	}

#endif

	if(!emcore_get_handle_by_mailId_from_transaction_info(mail_id , &handle )) {
		EM_DEBUG_EXCEPTION("emcore_get_handle_by_mailId_from_transaction_info failed for mail_id[%d]", mail_id);
		err = EMAIL_ERROR_HANDLE_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!(ref_account = emcore_get_account_reference(account_id)))  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		err = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (!emcore_cancel_send_mail_thread(handle, NULL, &err))  {
		EM_DEBUG_EXCEPTION("emcore_cancel_send_mail_thread failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if(!emcore_delete_transaction_info_by_mailId(mail_id))
		EM_DEBUG_EXCEPTION("emcore_delete_transaction_info_by_mailId failed for mail_id[%d]", mail_id);
	
	ret = true;
	
FINISH_OFF:
	if(err_code != NULL)
		*err_code = err;
	
	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

#ifdef __FEATURE_PROGRESS_IN_OUTBOX__
	if(!emstorage_free_mail(&mail_tbl_data, 1, &err))
		EM_DEBUG_EXCEPTION("emcore_free_mail Failed [%d ]", err);	

#endif
	EM_DEBUG_FUNC_END();
	return ret;
}	

static char *_make_time_string_to_time_t(time_t time)
{
	char *time_string = NULL;
	struct tm *struct_time = NULL;

	if (!time) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		return NULL;
	}

	time_string = em_malloc(MAX_DATETIME_STRING_LENGTH);
	if (time_string == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return NULL;
	}

	struct_time = localtime(&time);
	SNPRINTF(time_string, MAX_DATETIME_STRING_LENGTH, "%d/%d/%d", struct_time->tm_mon + 1, struct_time->tm_mday, struct_time->tm_year + 1900);

	EM_DEBUG_LOG("time string = [%s]", time_string);
	return time_string;	
}

static char *_make_criteria_to_search_filter(email_search_filter_t *search_filter, int search_filter_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("search_filter : [%p], search_filter_count : [%d]", search_filter, search_filter_count);

	int i = 0;
	int err = EMAIL_ERROR_NONE;
	char *criteria = NULL;
	char *temp_criteria = NULL;
	char *time_string = NULL;
	
	if (search_filter == NULL || search_filter_count < 0) {
		EM_DEBUG_EXCEPTION("Invalid paramter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	criteria = (char *)em_malloc(STRING_LENGTH_FOR_DISPLAY * search_filter_count);
	if (criteria == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	temp_criteria = (char *)em_malloc(STRING_LENGTH_FOR_DISPLAY);
	if (temp_criteria == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (i = 0; i < search_filter_count; i++) {
		EM_DEBUG_LOG("search_filter_type [%d]", search_filter[i].search_filter_type);
		memset(temp_criteria, 0x00, STRING_LENGTH_FOR_DISPLAY);

		switch (search_filter[i].search_filter_type) {
		case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO :
		case EMAIL_SEARCH_FILTER_TYPE_UID :
		case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER :
		case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN :
			EM_DEBUG_LOG("integer_type_key_value [%d]", search_filter[i].search_filter_key_value.integer_type_key_value);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_BCC :
		case EMAIL_SEARCH_FILTER_TYPE_CC :
		case EMAIL_SEARCH_FILTER_TYPE_TO :
		case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID :
			EM_DEBUG_LOG("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_FROM :
			EM_DEBUG_LOG("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "from %s ", search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(criteria, temp_criteria, EM_SAFE_STRLEN(temp_criteria));
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SUBJECT :
			EM_DEBUG_LOG("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "subject %s ", search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(criteria, temp_criteria, EM_SAFE_STRLEN(temp_criteria));
			break;

		case EMAIL_SEARCH_FILTER_TYPE_KEYWORD :
			EM_DEBUG_LOG("string_type_key_value [%s]", search_filter[i].search_filter_key_value.string_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "keyword %s ", search_filter[i].search_filter_key_value.string_type_key_value);
			strncat(criteria, temp_criteria, EM_SAFE_STRLEN(temp_criteria));
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE :
			EM_DEBUG_LOG("time_type_key_value [%d]", search_filter[i].search_filter_key_value.time_type_key_value);
			time_string = _make_time_string_to_time_t(search_filter[i].search_filter_key_value.time_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "before %s ", time_string);
			strncat(criteria, temp_criteria, EM_SAFE_STRLEN(temp_criteria));
			break;
	
		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON :
			EM_DEBUG_LOG("time_type_key_value [%d]", search_filter[i].search_filter_key_value.time_type_key_value);
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE :
			EM_DEBUG_LOG("time_type_key_value [%d]", search_filter[i].search_filter_key_value.time_type_key_value);
			time_string = _make_time_string_to_time_t(search_filter[i].search_filter_key_value.time_type_key_value);
			SNPRINTF(temp_criteria, STRING_LENGTH_FOR_DISPLAY, "since %s ", time_string);
			strncat(criteria, temp_criteria, EM_SAFE_STRLEN(temp_criteria));
			break;

		default :
			EM_DEBUG_EXCEPTION("Invalid list_filter_item_type [%d]", search_filter);
			err = EMAIL_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
		EM_SAFE_FREE(time_string); /*prevent 26258*/
	}

FINISH_OFF:
	
	EM_SAFE_FREE(temp_criteria);
	EM_SAFE_FREE(time_string);

	if (err_code != NULL)
		*err_code = err;
		
	EM_DEBUG_FUNC_END();
	return criteria;
}

INTERNAL_FUNC int emdaemon_search_mail_on_server(int input_account_id, int input_mailbox_id, email_search_filter_t *input_search_filter, int input_search_filter_count, unsigned int *output_handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d], mailbox_id [%d], input_search_filter [%p], input_search_filter_count [%d], output_handle [%p]", input_account_id, input_mailbox_id, input_search_filter, input_search_filter_count, output_handle);
	int error = EMAIL_ERROR_NONE;
	int ret = false;
	char *criteria = NULL;
	email_event_t *event_data = NULL;
	
	if (input_mailbox_id == 0 || input_account_id < 0) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		error = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	criteria = _make_criteria_to_search_filter(input_search_filter, input_search_filter_count, &error);
	if (criteria == NULL) {
		EM_DEBUG_EXCEPTION("_make_criteria_to_search_filter failed");
		goto FINISH_OFF;
	}

	event_data = em_malloc(sizeof(email_event_t));
	event_data->type = EMAIL_EVENT_SEARCH_ON_SERVER;
	event_data->account_id = input_account_id;
	event_data->event_param_data_1 = EM_SAFE_STRDUP(criteria);
	event_data->event_param_data_4 = input_mailbox_id;

	if (!emcore_insert_event(event_data, (int *)output_handle, &error)) {
		EM_DEBUG_EXCEPTION("emcore_insert_event failed [%d]", error);
		error = EMAIL_ERROR_NONE;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ret == false && event_data) {
		emcore_free_event(event_data);
		EM_SAFE_FREE(event_data);
	}

	EM_SAFE_FREE(criteria);
	
	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END("error [%d]", error);
	return ret;
}

INTERNAL_FUNC int emdaemon_reschedule_sending_mail()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	char *conditional_clause_string = NULL;
	email_list_filter_t filter_list[7];
	email_mail_list_item_t *result_mail_list = NULL;
	int filter_rule_count = 7;
	int result_mail_count = 0;
	int i = 0;

	memset(filter_list, 0 , sizeof(email_list_filter_t) * filter_rule_count);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SCHEDULED_SENDING_TIME;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value  = 0;

	filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_AND;

	filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[2].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_LEFT_PARENTHESIS;

	filter_list[3].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[3].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS;
	filter_list[3].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[3].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAIL_STATUS_SEND_SCHEDULED;

	filter_list[4].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[4].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;

	filter_list[5].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[5].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS;
	filter_list[5].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[5].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAIL_STATUS_SEND_DELAYED;

	filter_list[6].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[6].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_RIGHT_PARENTHESIS;

	/* Get scheduled mail list */
	if( (err = emstorage_write_conditional_clause_for_getting_mail_list(filter_list, filter_rule_count, NULL, 0, -1, -1, &conditional_clause_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_write_conditional_clause_for_getting_mail_list failed[%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("conditional_clause_string[%s].", conditional_clause_string);

	if(!emstorage_query_mail_list(conditional_clause_string, true, &result_mail_list, &result_mail_count, &err) && !result_mail_list) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_list [%d]", err);
		goto FINISH_OFF;
	}

	/* Add alarm for scheduled mail */
	for(i = 0; i < result_mail_count; i++) {
		if((err = emcore_schedule_sending_mail(result_mail_list[i].mail_id, result_mail_list[i].scheduled_sending_time)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_schedule_sending_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	EM_SAFE_FREE(result_mail_list);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emdaemon_clear_all_mail_data(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int error = EMAIL_ERROR_NONE;
	
	if (emdaemon_initialize(&error)) {
		if (!emstorage_clear_mail_data(true, &error))
			EM_DEBUG_EXCEPTION("emstorage_clear_mail_data failed [%d]", error);
	}
	else {
		EM_DEBUG_EXCEPTION("emdaemon_initialize failed [%d]", error);
		if (err_code)
			*err_code = error;
		return false;
	}

	emcore_display_unread_in_badge();

	ret = true;

	if (!emstorage_create_table(EMAIL_CREATE_DB_NORMAL, &error)) 
		EM_DEBUG_EXCEPTION("emstorage_create_table failed [%d]", error);
	
	emdaemon_finalize(&error);
	
	if (err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
    return ret;
}

INTERNAL_FUNC int emdaemon_kill_daemon_if_no_account()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	email_account_t *account_list = NULL;
	int account_count = 0;

	if (!emcore_get_account_reference_list(&account_list, &account_count, &err)) {
		EM_DEBUG_LOG("emcore_get_account_reference_list failed [%d]", err);
	}

	EM_DEBUG_LOG("account_count [%d]", account_count);

	if (account_count == 0) {
		EM_DEBUG_LOG("kill email-service daemon");
		exit(44); /* exit with exit code 44 to prevent restarting */
	}

	if(account_list)
		emcore_free_account_list(&account_list, account_count, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emdaemon_check_smack_rule(int app_sockfd, char *file_path)
{
	EM_DEBUG_FUNC_BEGIN_SEC("app_sockfd[%d], file_path[%s]", app_sockfd, file_path);

	if (app_sockfd <= 0 || !file_path) {
		EM_DEBUG_LOG("Invalid parameter");
		return false;
	}

	char *app_label = NULL;
	char *file_label = NULL;
	char *real_file_path = NULL;
	int ret = 0;
	int result = false;

	static int have_smack = -1;

	if (-1 == have_smack) {
		if (NULL == smack_smackfs_path()) {
			have_smack = 0;
		} else {
			have_smack = 1;
		}
	}

	if(!have_smack) {
		EM_DEBUG_LOG("smack is disabled");
		result = true;
		goto FINISH_OFF;
	}

	/* Smack is enabled */

	ret = smack_new_label_from_socket(app_sockfd, &app_label);
	if (ret < 0) {
		EM_DEBUG_LOG("smack_new_label_from_socket failed");
		result = false;
		goto FINISH_OFF;
	}

	real_file_path = realpath(file_path, NULL);
	if (!real_file_path) {
		EM_DEBUG_LOG("realpath failed");
		result = false;
		goto FINISH_OFF;
	}

	ret = smack_getlabel(real_file_path, &file_label, SMACK_LABEL_ACCESS);
	if (ret < 0) {
		EM_DEBUG_LOG("smack_getlabel failed");
		result = false;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("APP_LABEL[%s], FILE_LABEL[%s]", app_label, file_label);

	ret = smack_have_access(app_label, file_label, "r");
	if (ret == 0) {
		/* Access Denied */
		result = false;
		goto FINISH_OFF;
	}

	/* Access granted */
	result = true;

FINISH_OFF:

	EM_SAFE_FREE(app_label);
	EM_SAFE_FREE(file_label);
	EM_SAFE_FREE(real_file_path);

	EM_DEBUG_FUNC_END();
	return result;
}

INTERNAL_FUNC int emdaemon_set_smack_label(char *file_path, char *label)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = 0;
	int result = true;

	if ((ret = smack_setlabel(file_path, label, SMACK_LABEL_ACCESS)) < 0) {
		EM_DEBUG_LOG("smack_setlabel failed");
		result = false;
	}

	EM_DEBUG_FUNC_END();
	return result;
}

INTERNAL_FUNC int emdaemon_finalize_sync(int account_id, int total_mail_count, int unread_mail_count, int *error)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], total_mail_count [%d], unread_mail_count [%d], error [%p]", account_id, total_mail_count, unread_mail_count, error);
	int err = EMAIL_ERROR_NONE, ret = true, result_sync_status = SYNC_STATUS_FINISHED;
	int topmost = false;

	if ((err = emcore_update_sync_status_of_account(account_id, SET_TYPE_MINUS, SYNC_STATUS_SYNCING)) != EMAIL_ERROR_NONE)
		EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);

	if (!emstorage_get_sync_status_of_account(ALL_ACCOUNT, &result_sync_status, &err))
		EM_DEBUG_EXCEPTION("emstorage_get_sync_status_of_account failed [%d]", err);

	/* Check the topmost of email app */
	if (vconf_get_int(VCONF_KEY_TOPMOST_WINDOW, &topmost) != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
	}

	if (result_sync_status == SYNC_STATUS_SYNCING) {
		if (topmost) {
			EM_DEBUG_LOG("The email app is topmost");
		} else {
			if ((err = emcore_add_notification(account_id, 0, total_mail_count, 0, EMAIL_ERROR_NONE, NOTIFICATION_DISPLAY_APP_ALL^NOTIFICATION_DISPLAY_APP_TICKER)) != EMAIL_ERROR_NONE)
				EM_DEBUG_EXCEPTION("emcore_add_notification failed : [%d]", err);
		}
	} else if (result_sync_status == SYNC_STATUS_HAVE_NEW_MAILS) {
		if (topmost) {
			EM_DEBUG_LOG("The email app is topmost");
		} else {

			if ((err = emcore_add_notification(account_id, 0, total_mail_count, 1, EMAIL_ERROR_NONE, NOTIFICATION_DISPLAY_APP_ALL)) != EMAIL_ERROR_NONE)
				EM_DEBUG_EXCEPTION("emcore_add_notification failed : [%d]", err);

#ifdef __FEATURE_BLOCKING_MODE__
			emcore_set_blocking_mode_status(false);
#endif /* __FEATURE_BLOCKING_MODE__ */

			if ((err = emcore_start_driving_mode(account_id)) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emcore_start_driving_mode failed : [%d]", err);
			}
		}

		if (!emcore_display_unread_in_badge())
			EM_DEBUG_EXCEPTION("emcore_display_unread_in_badge failed");

		if ((err = emcore_update_sync_status_of_account(account_id, SET_TYPE_MINUS, SYNC_STATUS_HAVE_NEW_MAILS)) != EMAIL_ERROR_NONE)
			EM_DEBUG_EXCEPTION("emcore_update_sync_status_of_account failed [%d]", err);
	} else {
		EM_DEBUG_LOG("sync status : [%d]", result_sync_status);
	}

	EM_DEBUG_FUNC_END();
	return ret;
}
/* --------------------------------------------------------------------------------*/
