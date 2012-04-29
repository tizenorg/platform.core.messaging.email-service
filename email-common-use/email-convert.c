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

#include <stdlib.h>
#include <stdio.h>
#include "string.h"
#include "email-convert.h"
#include "email-core-mail.h"
#include "email-debug-log.h"
#include "email-core-utils.h"
#include "email-utilities.h"
#include "email-storage.h"


#define fSEEN 0x1
#define fDELETED 0x2
#define fFLAGGED 0x4
#define fANSWERED 0x8
#define fOLD 0x10
#define fDRAFT 0x20
#define fATTACHMENT 0x40
#define fFORWARD    0x80

INTERNAL_FUNC int em_convert_mail_flag_to_int(emf_mail_flag_t flag, int *i_flag, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("i_flag[%p], err_code[%p]", i_flag, err_code);
	
	if (!i_flag)  {
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
		
	}

	*i_flag = 
			(flag.seen           ? fSEEN       : 0) |
			(flag.deleted        ? fDELETED    : 0) |
			(flag.flagged        ? fFLAGGED    : 0) |
			(flag.answered       ? fANSWERED   : 0) |
			(flag.recent         ? fOLD        : 0) |
			(flag.draft          ? fDRAFT      : 0) |
			(flag.has_attachment ? fATTACHMENT : 0) |
			(flag.forwarded      ? fFORWARD    : 0) ;

	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int em_convert_mail_int_to_flag(int i_flag, emf_mail_flag_t* flag, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("i_flag[0x%02x], flag[%p], err_code[%p]", i_flag, flag, err_code);
	
	if (!flag) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		if (err_code != NULL)
			*err_code = EMF_ERROR_INVALID_PARAM;
		return false;
	}
	
	flag->seen           = (i_flag & fSEEN ? 1 : 0);
	flag->deleted        = (i_flag & fDELETED ? 1 : 0);
	flag->flagged        = (i_flag & fFLAGGED ? 1 : 0);
	flag->answered       = (i_flag & fANSWERED ? 1 : 0);
	flag->recent         = (i_flag & fOLD ? 1 : 0);
	flag->draft          = (i_flag & fDRAFT ? 1 : 0);
	flag->has_attachment = (i_flag & fATTACHMENT ? 1 : 0);
	flag->forwarded      = (i_flag & fFORWARD    ? 1 : 0);

	EM_DEBUG_FUNC_END("FLAGS : seen[%d], deleted[%d], flagged[%d], answered[%d], recent[%d], draft[%d], has_attachment[%d], forwarded[%d]",
		flag->seen, flag->deleted, flag->flagged, flag->answered, flag->recent, flag->draft, flag->has_attachment, flag->forwarded);

    return true;
}

INTERNAL_FUNC int em_convert_mail_tbl_to_mail_status(emstorage_mail_tbl_t *mail_tbl_data, int *result_mail_status, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_tbl_data[%p], result_mail_status [%p], err_code[%p]", mail_tbl_data, result_mail_status, err_code);
	int ret = false, error_code = EMF_ERROR_NONE;
	int has_attachment = 0;
	
	if(!mail_tbl_data || !result_mail_status) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		error_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
				
	has_attachment = (mail_tbl_data->attachment_count > mail_tbl_data->inline_content_count) ? 1 : 0;

	*result_mail_status = (mail_tbl_data->flags_seen_field      ? fSEEN       : 0) |
	                      (mail_tbl_data->flags_deleted_field   ? fDELETED    : 0) |
	                      (mail_tbl_data->flags_flagged_field   ? fFLAGGED    : 0) |
	                      (mail_tbl_data->flags_answered_field  ? fANSWERED   : 0) |
	                      (mail_tbl_data->flags_recent_field    ? fOLD        : 0) |
	                      (mail_tbl_data->flags_draft_field     ? fDRAFT      : 0) |
	                      (has_attachment                       ? fATTACHMENT : 0) |
	                      (mail_tbl_data->flags_forwarded_field ? fFORWARD    : 0);

	ret = true;
FINISH_OFF:
		if (err_code != NULL)
		*err_code = error_code;
		
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
	}
	
INTERNAL_FUNC int em_convert_mail_status_to_mail_tbl(int mail_status, emstorage_mail_tbl_t *result_mail_tbl_data, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_status[%d], result_mail_tbl_data [%p], err_code[%p]", mail_status, result_mail_tbl_data, err_code);
	int ret = false, error_code = EMF_ERROR_NONE;

	if(!result_mail_tbl_data) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		error_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	result_mail_tbl_data->flags_seen_field           = (mail_status & fSEEN       ? 1 : 0);
	result_mail_tbl_data->flags_deleted_field        = (mail_status & fDELETED    ? 1 : 0);
	result_mail_tbl_data->flags_flagged_field        = (mail_status & fFLAGGED    ? 1 : 0);
	result_mail_tbl_data->flags_answered_field       = (mail_status & fANSWERED   ? 1 : 0);
	result_mail_tbl_data->flags_recent_field         = (mail_status & fOLD        ? 1 : 0);
	result_mail_tbl_data->flags_draft_field          = (mail_status & fDRAFT      ? 1 : 0);
	result_mail_tbl_data->flags_forwarded_field      = (mail_status & fFORWARD    ? 1 : 0);

	ret = true;
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error_code;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int em_convert_mail_tbl_to_mail_flag(emstorage_mail_tbl_t *mail_tbl_data, emf_mail_flag_t *result_flag, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_tbl_data[%p], result_flag [%p], err_code[%p]", mail_tbl_data, result_flag, err_code);
	int ret = false, error_code = EMF_ERROR_NONE;

	if(!mail_tbl_data || !result_flag) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		error_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	result_flag->seen           = mail_tbl_data->flags_seen_field;
	result_flag->deleted        = mail_tbl_data->flags_deleted_field;
	result_flag->flagged        = mail_tbl_data->flags_flagged_field;
	result_flag->answered       = mail_tbl_data->flags_answered_field;
	result_flag->recent         = mail_tbl_data->flags_recent_field;
	result_flag->draft          = mail_tbl_data->flags_draft_field;
	result_flag->has_attachment = (mail_tbl_data->attachment_count > mail_tbl_data->inline_content_count) ? 1 : 0;
	result_flag->forwarded      = mail_tbl_data->flags_forwarded_field;

	ret = true;
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error_code;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

INTERNAL_FUNC int em_convert_mail_flag_to_mail_tbl(emf_mail_flag_t *flag, emstorage_mail_tbl_t *result_mail_tbl_data,  int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("flag[%p], result_mail_tbl_data [%p], err_code[%p]", flag, result_mail_tbl_data, err_code);
	int ret = false, error_code = EMF_ERROR_NONE;

	if(!flag || !result_mail_tbl_data) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		error_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	result_mail_tbl_data->flags_seen_field           = flag->seen;
	result_mail_tbl_data->flags_deleted_field        = flag->deleted;
	result_mail_tbl_data->flags_flagged_field        = flag->flagged;
	result_mail_tbl_data->flags_answered_field       = flag->answered;
	result_mail_tbl_data->flags_recent_field         = flag->recent;
	result_mail_tbl_data->flags_draft_field          = flag->draft;
	result_mail_tbl_data->flags_forwarded_field      = flag->forwarded;

	ret = true;
FINISH_OFF:
	if (err_code != NULL)
		*err_code = error_code;
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int em_convert_account_to_account_tbl(emf_account_t *account, emstorage_account_tbl_t *account_tbl)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], account_tbl[%p]", account, account_tbl);
	int ret = 1;

	account_tbl->account_bind_type = account->account_bind_type;
	account_tbl->account_name = EM_SAFE_STRDUP(account->account_name);
	account_tbl->receiving_server_type = account->receiving_server_type;
	account_tbl->receiving_server_addr = EM_SAFE_STRDUP(account->receiving_server_addr);
	account_tbl->email_addr = EM_SAFE_STRDUP(account->email_addr);
	account_tbl->user_name = EM_SAFE_STRDUP(account->user_name);
	account_tbl->password = EM_SAFE_STRDUP(account->password);
	account_tbl->retrieval_mode = account->retrieval_mode;
	account_tbl->port_num = account->port_num;
	account_tbl->use_security = account->use_security;
	account_tbl->sending_server_type = account->sending_server_type;
	account_tbl->sending_server_addr = EM_SAFE_STRDUP(account->sending_server_addr);
	account_tbl->sending_port_num = account->sending_port_num;
	account_tbl->sending_auth = account->sending_auth;
	account_tbl->sending_security = account->sending_security;
	account_tbl->sending_user = EM_SAFE_STRDUP(account->sending_user);
	account_tbl->sending_password = EM_SAFE_STRDUP(account->sending_password);
	account_tbl->display_name = EM_SAFE_STRDUP(account->display_name);
	account_tbl->reply_to_addr = EM_SAFE_STRDUP(account->reply_to_addr);
	account_tbl->return_addr = EM_SAFE_STRDUP(account->return_addr);
	account_tbl->account_id = account->account_id;
	account_tbl->keep_on_server = account->keep_on_server;
	account_tbl->flag1 = account->flag1;
	account_tbl->flag2 = account->flag2;
	account_tbl->pop_before_smtp = account->pop_before_smtp;
	account_tbl->apop =  account->apop;
	account_tbl->logo_icon_path = EM_SAFE_STRDUP(account->logo_icon_path);
	account_tbl->preset_account = account->preset_account;
	account_tbl->options.priority = account->options.priority;
	account_tbl->options.keep_local_copy = account->options.keep_local_copy;
	account_tbl->options.req_delivery_receipt = account->options.req_delivery_receipt;
	account_tbl->options.req_read_receipt = account->options.req_read_receipt;
	account_tbl->options.download_limit = account->options.download_limit;
	account_tbl->options.block_address = account->options.block_address;
	account_tbl->options.block_subject = account->options.block_subject;
	account_tbl->options.display_name_from = EM_SAFE_STRDUP(account->options.display_name_from);
	account_tbl->options.reply_with_body = account->options.reply_with_body;
	account_tbl->options.forward_with_files = account->options.forward_with_files;
	account_tbl->options.add_myname_card = account->options.add_myname_card;
	account_tbl->options.add_signature = account->options.add_signature;
	account_tbl->options.signature = EM_SAFE_STRDUP(account->options.signature);
	account_tbl->options.add_my_address_to_bcc = account->options.add_my_address_to_bcc;
	account_tbl->target_storage = account->target_storage;
	account_tbl->check_interval = account->check_interval;
	account_tbl->my_account_id = account->my_account_id;
	account_tbl->index_color = account->index_color;
	
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int em_convert_account_tbl_to_account(emstorage_account_tbl_t *account_tbl, emf_account_t *account)
{
	EM_DEBUG_FUNC_BEGIN("account_tbl[%p], account[%p]", account_tbl, account);
	int ret = 1;

	account->account_bind_type             = account_tbl->account_bind_type;
	account->account_name                  = EM_SAFE_STRDUP(account_tbl->account_name);
	account->receiving_server_type         = account_tbl->receiving_server_type;
	account->receiving_server_addr         = EM_SAFE_STRDUP(account_tbl->receiving_server_addr);
	account->email_addr                    = EM_SAFE_STRDUP(account_tbl->email_addr);
	account->user_name                     = EM_SAFE_STRDUP(account_tbl->user_name);
	account->password                      = EM_SAFE_STRDUP(account_tbl->password);
	account->retrieval_mode                = account_tbl->retrieval_mode;
	account->port_num                      = account_tbl->port_num;
	account->use_security                  = account_tbl->use_security;
	account->sending_server_type           = account_tbl->sending_server_type;
	account->sending_server_addr           = EM_SAFE_STRDUP(account_tbl->sending_server_addr);
	account->sending_port_num              = account_tbl->sending_port_num;
	account->sending_auth                  = account_tbl->sending_auth;
	account->sending_security              = account_tbl->sending_security;
	account->sending_user                  = EM_SAFE_STRDUP(account_tbl->sending_user);
	account->sending_password              = EM_SAFE_STRDUP(account_tbl->sending_password);
	account->display_name                  = EM_SAFE_STRDUP(account_tbl->display_name);
	account->reply_to_addr                 = EM_SAFE_STRDUP(account_tbl->reply_to_addr);
	account->return_addr                   = EM_SAFE_STRDUP(account_tbl->return_addr);
	account->account_id                    = account_tbl->account_id;
	account->keep_on_server                = account_tbl->keep_on_server;
	account->flag1                         = account_tbl->flag1;
	account->flag2                         = account_tbl->flag2;
	account->pop_before_smtp               = account_tbl->pop_before_smtp;
	account->apop                          = account_tbl->apop;
	account->logo_icon_path                = EM_SAFE_STRDUP(account_tbl->logo_icon_path);
	account->preset_account                = account_tbl->preset_account;
	account->options.priority              = account_tbl->options.priority;
	account->options.keep_local_copy       = account_tbl->options.keep_local_copy;
	account->options.req_delivery_receipt  = account_tbl->options.req_delivery_receipt;
	account->options.req_read_receipt      = account_tbl->options.req_read_receipt;
	account->options.download_limit        = account_tbl->options.download_limit;
	account->options.block_address         = account_tbl->options.block_address;
	account->options.block_subject         = account_tbl->options.block_subject;
	account->options.display_name_from     = EM_SAFE_STRDUP(account_tbl->options.display_name_from);
	account->options.reply_with_body       = account_tbl->options.reply_with_body;
	account->options.forward_with_files    = account_tbl->options.forward_with_files;
	account->options.add_myname_card       = account_tbl->options.add_myname_card;
	account->options.add_signature         = account_tbl->options.add_signature;
	account->options.signature             = EM_SAFE_STRDUP(account_tbl->options.signature);
	account->options.add_my_address_to_bcc = account_tbl->options.add_my_address_to_bcc;
	account->target_storage                = account_tbl->target_storage;
	account->check_interval                = account_tbl->check_interval;
	account->my_account_id                 = account_tbl->my_account_id;
	account->index_color                   = account_tbl->index_color;
	
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int em_convert_mail_tbl_to_mail_data(emstorage_mail_tbl_t *mail_table_data, int item_count, emf_mail_data_t **mail_data, int *error)
{
	EM_DEBUG_FUNC_BEGIN("mail_table_data[%p], item_count [%d], mail_data[%p]", mail_table_data, item_count, mail_data);
	int i, ret = false, err_code = EMF_ERROR_NONE;
	emf_mail_data_t *temp_mail_data = NULL;

	if (!mail_table_data || !mail_data || !item_count) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	temp_mail_data = em_malloc(sizeof(emf_mail_data_t) * item_count);
	
	if(!temp_mail_data) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err_code = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < item_count; i++) {
		temp_mail_data[i].mail_id                 = mail_table_data[i].mail_id;
		temp_mail_data[i].account_id              = mail_table_data[i].account_id;
		temp_mail_data[i].mailbox_name            = EM_SAFE_STRDUP(mail_table_data[i].mailbox_name);
		temp_mail_data[i].mailbox_type            = mail_table_data[i].mailbox_type;
		temp_mail_data[i].subject                 = EM_SAFE_STRDUP(mail_table_data[i].subject);
		temp_mail_data[i].date_time               = mail_table_data[i].date_time;
		temp_mail_data[i].server_mail_status      = mail_table_data[i].server_mail_status;
		temp_mail_data[i].server_mailbox_name     = EM_SAFE_STRDUP(mail_table_data[i].server_mailbox_name);
		temp_mail_data[i].server_mail_id          = EM_SAFE_STRDUP(mail_table_data[i].server_mail_id);
		temp_mail_data[i].message_id              = EM_SAFE_STRDUP(mail_table_data[i].message_id);
		temp_mail_data[i].full_address_from       = EM_SAFE_STRDUP(mail_table_data[i].full_address_from);
		temp_mail_data[i].full_address_reply      = EM_SAFE_STRDUP(mail_table_data[i].full_address_reply);
		temp_mail_data[i].full_address_to         = EM_SAFE_STRDUP(mail_table_data[i].full_address_to);
		temp_mail_data[i].full_address_cc         = EM_SAFE_STRDUP(mail_table_data[i].full_address_cc);
		temp_mail_data[i].full_address_bcc        = EM_SAFE_STRDUP(mail_table_data[i].full_address_bcc);
		temp_mail_data[i].full_address_return     = EM_SAFE_STRDUP(mail_table_data[i].full_address_return);
		temp_mail_data[i].email_address_sender    = EM_SAFE_STRDUP(mail_table_data[i].email_address_sender);
		temp_mail_data[i].email_address_recipient = EM_SAFE_STRDUP(mail_table_data[i].email_address_recipient);
		temp_mail_data[i].alias_sender            = EM_SAFE_STRDUP(mail_table_data[i].alias_sender);
		temp_mail_data[i].alias_recipient         = EM_SAFE_STRDUP(mail_table_data[i].alias_recipient);
		temp_mail_data[i].body_download_status    = mail_table_data[i].body_download_status;
		temp_mail_data[i].file_path_plain         = EM_SAFE_STRDUP(mail_table_data[i].file_path_plain);
		temp_mail_data[i].file_path_html          = EM_SAFE_STRDUP(mail_table_data[i].file_path_html);
		temp_mail_data[i].mail_size               = mail_table_data[i].mail_size;
		temp_mail_data[i].flags_seen_field        = mail_table_data[i].flags_seen_field;
		temp_mail_data[i].flags_deleted_field     = mail_table_data[i].flags_deleted_field;
		temp_mail_data[i].flags_flagged_field     = mail_table_data[i].flags_flagged_field;
		temp_mail_data[i].flags_answered_field    = mail_table_data[i].flags_answered_field;
		temp_mail_data[i].flags_recent_field      = mail_table_data[i].flags_recent_field;
		temp_mail_data[i].flags_draft_field       = mail_table_data[i].flags_draft_field;
		temp_mail_data[i].flags_forwarded_field   = mail_table_data[i].flags_forwarded_field;
		temp_mail_data[i].DRM_status              = mail_table_data[i].DRM_status;
		temp_mail_data[i].priority                = mail_table_data[i].priority;
		temp_mail_data[i].save_status             = mail_table_data[i].save_status;
		temp_mail_data[i].lock_status             = mail_table_data[i].lock_status;
		temp_mail_data[i].report_status           = mail_table_data[i].report_status;
		temp_mail_data[i].attachment_count        = mail_table_data[i].attachment_count;
		temp_mail_data[i].inline_content_count    = mail_table_data[i].inline_content_count;
		temp_mail_data[i].thread_id               = mail_table_data[i].thread_id;
		temp_mail_data[i].thread_item_count       = mail_table_data[i].thread_item_count;
		temp_mail_data[i].preview_text            = EM_SAFE_STRDUP(mail_table_data[i].preview_text);
		temp_mail_data[i].meeting_request_status  = mail_table_data[i].meeting_request_status;
	}

	*mail_data = temp_mail_data;

	ret = true;
FINISH_OFF:

	if(error)
			*error = err_code;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int   em_convert_mail_data_to_mail_tbl(emf_mail_data_t *mail_data, int item_count, emstorage_mail_tbl_t **mail_table_data, int *error)
{
	EM_DEBUG_FUNC_BEGIN("mail_data[%p], item_count [%d], mail_table_data[%p]", mail_data, item_count, mail_table_data);
	int i, ret = false, err_code = EMF_ERROR_NONE;
	emstorage_mail_tbl_t *temp_mail_tbl = NULL;

	if (!mail_data || !mail_table_data || !item_count) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err_code = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	temp_mail_tbl = em_malloc(sizeof(emstorage_mail_tbl_t) * item_count);
	
	if(!temp_mail_tbl) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err_code = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < item_count; i++) {
		temp_mail_tbl[i].mail_id                 = mail_data[i].mail_id;
		temp_mail_tbl[i].account_id              = mail_data[i].account_id;
		temp_mail_tbl[i].mailbox_name            = EM_SAFE_STRDUP(mail_data[i].mailbox_name);
		temp_mail_tbl[i].mailbox_type            = mail_data[i].mailbox_type;
		temp_mail_tbl[i].date_time               = mail_data[i].date_time;
		temp_mail_tbl[i].subject                 = EM_SAFE_STRDUP(mail_data[i].subject);
		temp_mail_tbl[i].server_mail_status      = mail_data[i].server_mail_status;
		temp_mail_tbl[i].server_mailbox_name     = EM_SAFE_STRDUP(mail_data[i].server_mailbox_name);
		temp_mail_tbl[i].server_mail_id          = EM_SAFE_STRDUP(mail_data[i].server_mail_id);
		temp_mail_tbl[i].message_id              = EM_SAFE_STRDUP(mail_data[i].message_id);
		temp_mail_tbl[i].full_address_from       = EM_SAFE_STRDUP(mail_data[i].full_address_from);
		temp_mail_tbl[i].full_address_reply      = EM_SAFE_STRDUP(mail_data[i].full_address_reply);
		temp_mail_tbl[i].full_address_to         = EM_SAFE_STRDUP(mail_data[i].full_address_to);
		temp_mail_tbl[i].full_address_cc         = EM_SAFE_STRDUP(mail_data[i].full_address_cc);
		temp_mail_tbl[i].full_address_bcc        = EM_SAFE_STRDUP(mail_data[i].full_address_bcc);
		temp_mail_tbl[i].full_address_return     = EM_SAFE_STRDUP(mail_data[i].full_address_return);
		temp_mail_tbl[i].email_address_sender    = EM_SAFE_STRDUP(mail_data[i].email_address_sender);
		temp_mail_tbl[i].email_address_recipient = EM_SAFE_STRDUP(mail_data[i].email_address_recipient);
		temp_mail_tbl[i].alias_sender            = EM_SAFE_STRDUP(mail_data[i].alias_sender);
		temp_mail_tbl[i].alias_recipient         = EM_SAFE_STRDUP(mail_data[i].alias_recipient);
		temp_mail_tbl[i].body_download_status    = mail_data[i].body_download_status;
		temp_mail_tbl[i].file_path_plain         = EM_SAFE_STRDUP(mail_data[i].file_path_plain);
		temp_mail_tbl[i].file_path_html          = EM_SAFE_STRDUP(mail_data[i].file_path_html);
		temp_mail_tbl[i].mail_size               = mail_data[i].mail_size;
		temp_mail_tbl[i].flags_seen_field        = mail_data[i].flags_seen_field;
		temp_mail_tbl[i].flags_deleted_field     = mail_data[i].flags_deleted_field;
		temp_mail_tbl[i].flags_flagged_field     = mail_data[i].flags_flagged_field;
		temp_mail_tbl[i].flags_answered_field    = mail_data[i].flags_answered_field;
		temp_mail_tbl[i].flags_recent_field      = mail_data[i].flags_recent_field;
		temp_mail_tbl[i].flags_draft_field       = mail_data[i].flags_draft_field;
		temp_mail_tbl[i].flags_forwarded_field   = mail_data[i].flags_forwarded_field;
		temp_mail_tbl[i].DRM_status              = mail_data[i].DRM_status;
		temp_mail_tbl[i].priority                = mail_data[i].priority;
		temp_mail_tbl[i].save_status             = mail_data[i].save_status;
		temp_mail_tbl[i].lock_status             = mail_data[i].lock_status;
		temp_mail_tbl[i].report_status           = mail_data[i].report_status;
		temp_mail_tbl[i].attachment_count        = mail_data[i].attachment_count;
		temp_mail_tbl[i].inline_content_count    = mail_data[i].inline_content_count;
		temp_mail_tbl[i].thread_id               = mail_data[i].thread_id;
		temp_mail_tbl[i].thread_item_count       = mail_data[i].thread_item_count;
		temp_mail_tbl[i].preview_text            = EM_SAFE_STRDUP(mail_data[i].preview_text);
		temp_mail_tbl[i].meeting_request_status  = mail_data[i].meeting_request_status;
	}

	*mail_table_data = temp_mail_tbl;

	ret = true;
FINISH_OFF:

	if(error)
			*error = err_code;

	EM_DEBUG_FUNC_END();
	return ret;
	
}



INTERNAL_FUNC int em_convert_string_to_time_t(char *input_datetime_string, time_t *output_time)
{
	EM_DEBUG_FUNC_BEGIN("input_datetime_string[%s], output_time[%p]", input_datetime_string, output_time);

	char buf[16] = { 0, };
	struct tm temp_time_info = { 0 };

	if (!input_datetime_string || !output_time) {
		EM_DEBUG_EXCEPTION("input_datetime_string[%p], output_time[%p]", input_datetime_string, output_time);
		return EMF_ERROR_INVALID_PARAM;
	}

	memset(buf, 0x00, sizeof(buf));
	SNPRINTF(buf, sizeof(buf), "%.4s", input_datetime_string);
	temp_time_info.tm_year = atoi(buf) - 1970;

	memset(buf, 0x00, sizeof(buf));
	SNPRINTF(buf, sizeof(buf), "%.2s", input_datetime_string + 4);
	temp_time_info.tm_mon = atoi(buf) - 1;

	memset(buf, 0x00, sizeof(buf));
	SNPRINTF(buf, sizeof(buf), "%.2s", input_datetime_string + 6);
	temp_time_info.tm_mday = atoi(buf);

	memset(buf, 0x00, sizeof(buf));
	SNPRINTF(buf, sizeof(buf), "%.2s", input_datetime_string + 8);
	temp_time_info.tm_hour = atoi(buf);

	memset(buf, 0x00, sizeof(buf));
	SNPRINTF(buf, sizeof(buf), "%.2s", input_datetime_string + 10);
	temp_time_info.tm_min = atoi(buf);

	memset(buf, 0x00, sizeof(buf));
	SNPRINTF(buf, sizeof(buf), "%.2s", input_datetime_string + 12);
	temp_time_info.tm_sec = atoi(buf);

	*output_time = timegm(&temp_time_info);

	EM_DEBUG_LOG("*output_time [%d", *output_time);

	EM_DEBUG_FUNC_END("err %d", EMF_ERROR_NONE);
	return EMF_ERROR_NONE;
}

INTERNAL_FUNC int em_convert_time_t_to_string(time_t *input_time, char **output_datetime_string)
{
	EM_DEBUG_FUNC_BEGIN("input_time[%p], output_datetime_string[%p]", input_time, output_datetime_string);
	char temp_buffer[20] = { 0, };
	struct tm *temp_time_info;

	if (!input_time || !output_datetime_string) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;
	}

	temp_time_info = localtime(input_time);

	if(!temp_time_info) {
		EM_DEBUG_EXCEPTION("localtime failed.");
		return EMF_ERROR_SYSTEM_FAILURE;
	}
	SNPRINTF(temp_buffer, sizeof(temp_buffer), "%04d%02d%02d%02d%02d%02d",
		temp_time_info->tm_year + 1970, temp_time_info->tm_mon, temp_time_info->tm_mday, temp_time_info->tm_hour, temp_time_info->tm_min, temp_time_info->tm_sec);

	*output_datetime_string = EM_SAFE_STRDUP(temp_buffer);

	EM_DEBUG_FUNC_END("err %d", EMF_ERROR_NONE);
	return EMF_ERROR_NONE;
}

static char* append_sized_data_to_stream(char *input_stream, int *input_output_stream_length, char *input_sized_data, int input_data_size)
{
	EM_DEBUG_FUNC_BEGIN("input_stream [%p], input_output_stream_length [%p], input_sized_data [%p], input_data_size [%d]", input_stream, input_output_stream_length, input_sized_data, input_data_size);
	char *new_stream = NULL;
	int source_stream_length = 0;

	if( !input_output_stream_length || input_data_size == 0 || input_sized_data == NULL|| 
			(input_stream != NULL && *input_output_stream_length == 0) || (input_stream == NULL && *input_output_stream_length != 0) ) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return NULL;	
	}

	source_stream_length = *input_output_stream_length;

	new_stream = (char*)em_malloc((source_stream_length) * sizeof(char) + input_data_size);	

	if(!new_stream) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_OUT_OF_MEMORY");
		return NULL;	
	}

	if(input_stream != NULL)
		memcpy(new_stream, input_stream, source_stream_length * sizeof(char));

	memcpy(new_stream + source_stream_length, input_sized_data, input_data_size);

	*input_output_stream_length = source_stream_length + input_data_size;
	
	EM_SAFE_FREE(input_stream);	
	EM_DEBUG_FUNC_END("*input_output_stream_length [%d]", *input_output_stream_length);
	return new_stream;	
}

static char* append_string_to_stream(char *input_stream, int *input_output_stream_length, char *input_source_string)
{
	EM_DEBUG_FUNC_BEGIN("input_stream [%p], input_output_stream_length [%p] input_source_string[%p]", input_stream, input_output_stream_length, input_source_string);
	char *new_stream = NULL;
	int   data_length = 0;
	int   source_stream_length = 0;

	if( !input_output_stream_length || (input_stream != NULL && *input_output_stream_length == 0) || (input_stream == NULL && *input_output_stream_length != 0)) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return NULL;	
	}

	source_stream_length = *input_output_stream_length;

	if(input_source_string != NULL)
		data_length = EM_SAFE_STRLEN(input_source_string);

	new_stream = (char*)em_malloc((source_stream_length + data_length) * sizeof(char) + sizeof(int));	

	if(!new_stream) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_OUT_OF_MEMORY");
		return NULL;	
	}

	if(input_stream != NULL)
		memcpy(new_stream, input_stream, source_stream_length * sizeof(char));

	memcpy(new_stream + source_stream_length, (char*)&data_length, sizeof(int));

	if(input_source_string) 
		memcpy(new_stream + source_stream_length + sizeof(int), input_source_string, data_length);

	*input_output_stream_length = source_stream_length + sizeof(int) + data_length;
	
	EM_SAFE_FREE(input_stream);	
	EM_DEBUG_FUNC_END("*input_output_stream_length [%d]", *input_output_stream_length);
	return new_stream;	
}

static int fetch_sized_data_from_stream(char *input_stream, int *input_output_stream_offset, int input_data_size, char *output_data) 
{
	EM_DEBUG_FUNC_BEGIN("input_stream [%p], input_output_stream_offset [%p] input_data_size [%d], output_data[%p]", input_stream, input_output_stream_offset, input_data_size, output_data);
	int stream_offset = 0;

	if( !input_stream || !input_output_stream_offset || !input_data_size || !output_data) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;	
	}

	stream_offset = *input_output_stream_offset;

	memcpy(output_data, input_stream + stream_offset, input_data_size);
	stream_offset += input_data_size;

	*input_output_stream_offset = stream_offset;

	EM_DEBUG_FUNC_END("stream_offset [%d]", stream_offset);
	return EMF_ERROR_NONE;
}

static int fetch_string_from_stream(char *input_stream, int *input_output_stream_offset, char **output_string) 
{
	EM_DEBUG_FUNC_BEGIN("input_stream [%p], input_output_stream_offset [%p] output_string[%p]", input_stream, input_output_stream_offset, output_string);
	int string_length = 0;
	int stream_offset = 0;
	char *result_string = NULL;

	if( !input_stream || !input_output_stream_offset || !output_string) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;	
	}

	stream_offset = *input_output_stream_offset;
	/*
	EM_DEBUG_LOG("stream_offset [%d]", stream_offset);
	*/

	memcpy(&string_length, input_stream + stream_offset, sizeof(int));
	stream_offset +=  sizeof(int);

	/*
	EM_DEBUG_LOG("string_length [%d]", string_length);
	*/

	if(string_length != 0) {
		result_string = (char*)em_malloc(string_length + 1);
		if(result_string) {
			memcpy(result_string, input_stream + stream_offset, string_length);
			stream_offset += string_length;
		}
	}
	/*
	if(result_string)
		EM_DEBUG_LOG("result_string [%s]", result_string);
	*/

	*output_string              = result_string;
	*input_output_stream_offset = stream_offset;
	
	EM_DEBUG_FUNC_END("stream_offset [%d]", stream_offset);
	return EMF_ERROR_NONE;
}

INTERNAL_FUNC char* em_convert_account_to_byte_stream(emf_account_t* input_account, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_account [%p], output_stream_size [%p]", input_account, output_stream_size);
	char *result_stream = NULL;
	int stream_size = 0;

	EM_IF_NULL_RETURN_VALUE(input_account, NULL);
	
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->account_bind_type), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->account_name);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->receiving_server_type), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->receiving_server_addr);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->email_addr);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->user_name);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->password);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->retrieval_mode), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->port_num), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->use_security), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->sending_server_type), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->sending_server_addr);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->sending_port_num), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->sending_auth), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->sending_security), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->sending_user);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->sending_password);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->display_name);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->reply_to_addr);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->return_addr);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->account_id), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->keep_on_server), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->flag1), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->flag2), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->pop_before_smtp), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->apop), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->logo_icon_path);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->preset_account), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.priority), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.keep_local_copy), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.req_delivery_receipt), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.req_read_receipt), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.download_limit), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.block_address), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.block_subject), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->options.display_name_from);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.reply_with_body), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.forward_with_files), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.add_myname_card), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.add_signature), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->options.signature);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->options.add_my_address_to_bcc), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->target_storage), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->check_interval), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->my_account_id), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->index_color), sizeof(int));

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END("stream_size [%d]", stream_size);
	return result_stream;
}


INTERNAL_FUNC void em_convert_byte_stream_to_account(char *input_stream,  emf_account_t *output_account)
{
	EM_DEBUG_FUNC_BEGIN();
	int stream_offset = 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_account);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->account_bind_type);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->account_name);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->receiving_server_type);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->receiving_server_addr);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->email_addr);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->user_name);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->password);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->retrieval_mode);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->port_num);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->use_security);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->sending_server_type);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->sending_server_addr);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->sending_port_num);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->sending_auth);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->sending_security);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->sending_user);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->sending_password);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->display_name);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->reply_to_addr);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->return_addr);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->account_id);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->keep_on_server);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->flag1);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->flag2);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->pop_before_smtp);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->apop);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->logo_icon_path);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->preset_account);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.priority);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.keep_local_copy);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.req_delivery_receipt);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.req_read_receipt);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.download_limit);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.block_address);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.block_subject);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->options.display_name_from);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.reply_with_body);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.forward_with_files);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.add_myname_card);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.add_signature);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->options.signature);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->options.add_my_address_to_bcc);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->target_storage);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->check_interval);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->my_account_id);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->index_color);
	
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC char* em_convert_attachment_info_to_byte_stream(emf_attachment_info_t *input_attachment_info, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN();

	char *result_stream = NULL;
	int stream_size = 0;

	EM_IF_NULL_RETURN_VALUE(input_attachment_info, NULL);
	EM_IF_NULL_RETURN_VALUE(output_stream_size, NULL);

	while(input_attachment_info) {
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_info->inline_content, sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_info->attachment_id, sizeof(int)); 
		result_stream = append_string_to_stream(result_stream, &stream_size, input_attachment_info->name);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_info->size, sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_info->downloaded, sizeof(int));
		result_stream = append_string_to_stream(result_stream, &stream_size, input_attachment_info->savename);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_info->drm, sizeof(int)); 

		input_attachment_info = input_attachment_info->next;
	}

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END();
	return result_stream;
}

INTERNAL_FUNC void em_convert_byte_stream_to_attachment_info(char *input_stream, int attachment_count, emf_attachment_info_t **output_attachment_info)
{
	EM_DEBUG_FUNC_BEGIN();

	int i = 0;
	int stream_offset = 0;
	emf_attachment_info_t *temp_attachment_info = NULL;
	emf_attachment_info_t *current_attachment_info = NULL;

	EM_NULL_CHECK_FOR_VOID(input_stream);

	for(i = 0; i < attachment_count; i++) {	
		temp_attachment_info = (emf_attachment_info_t*)malloc(sizeof(emf_attachment_info_t));

		if(i == 0)
			*output_attachment_info = current_attachment_info = temp_attachment_info;
		else {
			current_attachment_info->next = temp_attachment_info;
			current_attachment_info = temp_attachment_info;
		}
		
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&temp_attachment_info->inline_content);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&temp_attachment_info->attachment_id); 
		fetch_string_from_stream(input_stream, &stream_offset, &temp_attachment_info->name);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&temp_attachment_info->size);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&temp_attachment_info->downloaded);
		fetch_string_from_stream(input_stream, &stream_offset, &temp_attachment_info->savename);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&temp_attachment_info->drm); 

		temp_attachment_info->next = NULL;
	}
	
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC char* em_convert_mail_data_to_byte_stream(emf_mail_data_t *input_mail_data, int input_mail_data_count, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data [%p], input_mail_data_count[%d], output_stream_size[%p]", input_mail_data, input_mail_data_count, output_stream_size);
	
	char *result_stream = NULL;
	int stream_size = 0;
	int i = 0;
	
	EM_IF_NULL_RETURN_VALUE(input_mail_data, NULL);
	EM_IF_NULL_RETURN_VALUE(output_stream_size, NULL);

	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_mail_data_count, sizeof(int));

	for(i = 0; i < input_mail_data_count; i++) {
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].mail_id), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].account_id), sizeof(int));
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].mailbox_name);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].mailbox_type), sizeof(int));
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].subject);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].date_time), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].server_mail_status), sizeof(int));
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].server_mailbox_name);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].server_mail_id);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].message_id);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].full_address_from);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].full_address_reply);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].full_address_to);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].full_address_cc);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].full_address_bcc);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].full_address_return);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].email_address_sender);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].email_address_recipient);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].alias_sender);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].alias_recipient);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].body_download_status), sizeof(int));
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].file_path_plain);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].file_path_html);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].mail_size), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].flags_seen_field), sizeof(char));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].flags_deleted_field), sizeof(char));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].flags_flagged_field), sizeof(char));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].flags_answered_field), sizeof(char));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].flags_recent_field), sizeof(char));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].flags_draft_field), sizeof(char));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].flags_forwarded_field), sizeof(char));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].DRM_status), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].priority), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].save_status), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].lock_status), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].report_status), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].attachment_count), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].inline_content_count), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].thread_id), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].thread_item_count), sizeof(int));
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].preview_text);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].meeting_request_status), sizeof(int));
	}

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END("stream_size [%d]", stream_size);
	return result_stream;
}

INTERNAL_FUNC void em_convert_byte_stream_to_mail_data(char *input_stream, emf_mail_data_t **output_mail_data, int *output_mail_data_count)
{
	EM_DEBUG_FUNC_BEGIN("input_stream [%p], output_mail_data[%p], output_mail_data_count[%p]", input_stream, output_mail_data, output_mail_data_count);

	int stream_offset = 0;
	int i = 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_mail_data);
	EM_NULL_CHECK_FOR_VOID(output_mail_data_count);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)output_mail_data_count);

	EM_DEBUG_LOG("*output_mail_data_count [%d]", *output_mail_data_count);

	if(output_mail_data_count <= 0) {
		EM_DEBUG_EXCEPTION("no mail data.");
		return;
	}

	*output_mail_data = (emf_mail_data_t*)em_malloc(sizeof(emf_mail_data_t) * (*output_mail_data_count));

	if(!*output_mail_data) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return;
	}

	for(i = 0; i < *output_mail_data_count; i++) {
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].mail_id);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].account_id);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].mailbox_name);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].mailbox_type);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].subject);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].date_time);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].server_mail_status);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].server_mailbox_name);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].server_mail_id);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].message_id);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].full_address_from);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].full_address_reply);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].full_address_to);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].full_address_cc);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].full_address_bcc);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].full_address_return);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].email_address_sender);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].email_address_recipient);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].alias_sender);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].alias_recipient);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].body_download_status);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].file_path_plain);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].file_path_html);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].mail_size);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(char), (char*)&(*output_mail_data)[i].flags_seen_field);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(char), (char*)&(*output_mail_data)[i].flags_deleted_field);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(char), (char*)&(*output_mail_data)[i].flags_flagged_field);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(char), (char*)&(*output_mail_data)[i].flags_answered_field);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(char), (char*)&(*output_mail_data)[i].flags_recent_field);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(char), (char*)&(*output_mail_data)[i].flags_draft_field);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(char), (char*)&(*output_mail_data)[i].flags_forwarded_field);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].DRM_status);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].priority);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].save_status);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].lock_status);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].report_status);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].attachment_count);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].inline_content_count);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].thread_id);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].thread_item_count);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].preview_text);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].meeting_request_status);
	}

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC char* em_convert_attachment_data_to_byte_stream(emf_attachment_data_t *input_attachment_data, int input_attachment_count, int* output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_attachment_data [%p], input_attachment_count [%d], output_stream_size [%p]", input_attachment_data, input_attachment_count, output_stream_size);
	
	char *result_stream = NULL;
	int stream_size = 0;
	int i = 0;

	if(input_attachment_count > 0)
		EM_IF_NULL_RETURN_VALUE(input_attachment_data, NULL);
	EM_IF_NULL_RETURN_VALUE(output_stream_size, NULL);

	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_count, sizeof(int));

	for(i = 0; i < input_attachment_count; i++) {
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].attachment_id, sizeof(int));
		result_stream = append_string_to_stream(result_stream, &stream_size, input_attachment_data[i].attachment_name);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_attachment_data[i].attachment_path);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].attachment_size, sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].mail_id, sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].account_id, sizeof(int));
		result_stream = append_string_to_stream(result_stream, &stream_size, input_attachment_data[i].mailbox_name);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].save_status, sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].drm_status, sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].inline_content_status,sizeof(int));
	}

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END("stream_size [%d]", stream_size);
	return result_stream;
}

INTERNAL_FUNC void em_convert_byte_stream_to_attachment_data(char *input_stream, emf_attachment_data_t **output_attachment_data, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("input_stream [%p], output_attachment_data[%p]", input_stream, output_attachment_data);

	int stream_offset = 0;
	int i = 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_attachment_data);
	EM_NULL_CHECK_FOR_VOID(output_attachment_count);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)output_attachment_count);

	EM_DEBUG_LOG("*output_attachment_count [%d]", *output_attachment_count);

	if(output_attachment_count <= 0) {
		EM_DEBUG_EXCEPTION("no attachment data.");
		return;
	}

	*output_attachment_data = (emf_attachment_data_t*)em_malloc(sizeof(emf_attachment_data_t) * (*output_attachment_count));

	if(!*output_attachment_data) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return;
	}

	for(i = 0; i < *output_attachment_count; i++) {
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].attachment_id));
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_attachment_data)[i].attachment_name);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_attachment_data)[i].attachment_path);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].attachment_size));
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].mail_id));
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].account_id));
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_attachment_data)[i].mailbox_name);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].save_status));
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].drm_status));
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].inline_content_status));
	}

	EM_DEBUG_FUNC_END();
}



INTERNAL_FUNC char* em_convert_mailbox_to_byte_stream(emf_mailbox_t *input_mailbox_data, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_mailbox_data [%p], output_stream_size [%p]", input_mailbox_data, output_stream_size);
	
	char *result_stream = NULL;
	int   stream_size 	=  0;

	EM_IF_NULL_RETURN_VALUE(input_mailbox_data, NULL);
	EM_IF_NULL_RETURN_VALUE(output_stream_size, NULL);

	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->mailbox_id), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_mailbox_data->name);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->mailbox_type), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_mailbox_data->alias);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->unread_count), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->total_mail_count_on_local), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->total_mail_count_on_server), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->hold_connection), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->local), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->synchronous), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->account_id), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->user_data), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->mail_stream), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->has_archived_mails), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->mail_slot_size), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_mailbox_data->account_name);
	
	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END();

	return result_stream;
}


INTERNAL_FUNC void em_convert_byte_stream_to_mailbox(char *input_stream, emf_mailbox_t *output_mailbox_data)
{
	EM_DEBUG_FUNC_BEGIN("input_stream [%p], output_mailbox_data [%p]", input_stream, output_mailbox_data);
	int 		stream_offset 	= 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_mailbox_data);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->mailbox_id));
	fetch_string_from_stream(input_stream, &stream_offset, &output_mailbox_data->name);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->mailbox_type));
	fetch_string_from_stream(input_stream, &stream_offset, &output_mailbox_data->alias);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->unread_count));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->total_mail_count_on_local));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->total_mail_count_on_server));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->hold_connection));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->local));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->synchronous));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->account_id));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->user_data));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->mail_stream));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->has_archived_mails));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->mail_slot_size));
	fetch_string_from_stream(input_stream, &stream_offset, &output_mailbox_data->account_name);
	EM_DEBUG_FUNC_END();
}


INTERNAL_FUNC char* em_convert_option_to_byte_stream(emf_option_t* input_option, int* output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN();
	char *result_stream = NULL;
	int stream_size = 0;

	EM_IF_NULL_RETURN_VALUE(input_option, NULL);

	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->priority), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->keep_local_copy), sizeof(int)); 
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->req_delivery_receipt), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->req_read_receipt), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->download_limit), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->block_address), sizeof(int)); 
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->block_subject), sizeof(int)); 
	result_stream = append_string_to_stream(result_stream, &stream_size, input_option->display_name_from); 
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->reply_with_body), sizeof(int)); 
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->forward_with_files), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->add_myname_card), sizeof(int)); 
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->add_signature), sizeof(int)); 
	result_stream = append_string_to_stream(result_stream, &stream_size, input_option->signature); 
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_option->add_my_address_to_bcc), sizeof(int)); 
	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END();
	return result_stream;

}

INTERNAL_FUNC void em_convert_byte_stream_to_option(char *input_stream, emf_option_t *output_option)
{
	EM_DEBUG_FUNC_BEGIN();
	int stream_offset = 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->priority));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->keep_local_copy)); 
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->req_delivery_receipt));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->req_read_receipt));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->download_limit));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->block_address)); 
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->block_subject)); 
	fetch_string_from_stream(input_stream, &stream_offset, &output_option->display_name_from); 
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->reply_with_body)); 
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->forward_with_files));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->add_myname_card)); 
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->add_signature)); 
	fetch_string_from_stream(input_stream, &stream_offset, &output_option->signature); 
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_option->add_my_address_to_bcc)); 
	
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC char* em_convert_rule_to_byte_stream(emf_rule_t *input_rule, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN();
	char *result_stream = NULL;
	int stream_size = 0;

	EM_IF_NULL_RETURN_VALUE(input_rule, NULL);
	EM_IF_NULL_RETURN_VALUE(output_stream_size, NULL);

	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_rule->account_id), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_rule->filter_id), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_rule->type), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_rule->value);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_rule->faction), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_rule->mailbox);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_rule->flag1), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_rule->flag2), sizeof(int));

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END();
	return result_stream;
}

INTERNAL_FUNC void em_convert_byte_stream_to_rule(char *input_stream, emf_rule_t *output_rule)
{
	EM_DEBUG_FUNC_BEGIN();
	int stream_offset = 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_rule);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->account_id));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->filter_id));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->type));
	fetch_string_from_stream(input_stream, &stream_offset, &output_rule->value);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->faction));
	fetch_string_from_stream(input_stream, &stream_offset, &output_rule->mailbox);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->flag1));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->flag2));
	
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC char* em_convert_extra_flags_to_byte_stream(emf_extra_flag_t input_extra_flag, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN();
	char *result_stream = NULL;
	int   stream_size = 0;
	int   extra_flag_field = 0;

	EM_IF_NULL_RETURN_VALUE(output_stream_size, NULL);

	extra_flag_field = input_extra_flag.priority;
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&extra_flag_field, sizeof(int));
	extra_flag_field = input_extra_flag.status;
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&extra_flag_field, sizeof(int));
	extra_flag_field = input_extra_flag.noti;
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&extra_flag_field, sizeof(int));
	extra_flag_field = input_extra_flag.lock;
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&extra_flag_field, sizeof(int));
	extra_flag_field = input_extra_flag.report;
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&extra_flag_field, sizeof(int));
	extra_flag_field = input_extra_flag.drm;
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&extra_flag_field, sizeof(int));
	extra_flag_field = input_extra_flag.text_download_yn;
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&extra_flag_field, sizeof(int));

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END();

	return result_stream;
}

INTERNAL_FUNC void em_convert_byte_stream_to_extra_flags(char *input_stream, emf_extra_flag_t *output_extra_flag)
{
	EM_DEBUG_FUNC_BEGIN();

	int stream_offset = 0;
	int extra_flag_field = 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_extra_flag);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&extra_flag_field);
	output_extra_flag->priority = extra_flag_field;
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&extra_flag_field);
	output_extra_flag->status = extra_flag_field;
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&extra_flag_field);
	output_extra_flag->noti = extra_flag_field;
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&extra_flag_field);
	output_extra_flag->lock = extra_flag_field;
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&extra_flag_field);
	output_extra_flag->report = extra_flag_field;
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&extra_flag_field);
	output_extra_flag->drm = extra_flag_field;
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&extra_flag_field);
	output_extra_flag->text_download_yn = extra_flag_field;

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC char* em_convert_meeting_req_to_byte_stream(emf_meeting_request_t *input_meeting_req, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN();

	char *result_stream = NULL;
	int   stream_size        = 0;

	EM_IF_NULL_RETURN_VALUE(input_meeting_req, NULL);

	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_meeting_req->mail_id), sizeof(int)); 
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_meeting_req->meeting_response), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_meeting_req->start_time), sizeof(struct tm));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_meeting_req->end_time), sizeof(struct tm));
	result_stream = append_string_to_stream    (result_stream, &stream_size, input_meeting_req->location);
	result_stream = append_string_to_stream    (result_stream, &stream_size, input_meeting_req->global_object_id);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_meeting_req->time_zone.offset_from_GMT), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_meeting_req->time_zone.standard_name, 32);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_meeting_req->time_zone.standard_time_start_date), sizeof(struct tm));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_meeting_req->time_zone.standard_bias), sizeof(int)); 
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_meeting_req->time_zone.daylight_name, 32);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_meeting_req->time_zone.daylight_time_start_date), sizeof(struct tm));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_meeting_req->time_zone.daylight_bias), sizeof(int)); 

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END();
	return result_stream;
}


INTERNAL_FUNC void em_convert_byte_stream_to_meeting_req(char *input_stream,  emf_meeting_request_t *output_meeting_req)
{
	EM_DEBUG_FUNC_BEGIN();
	int stream_offset = 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_meeting_req);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_meeting_req->mail_id)); 
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_meeting_req->meeting_response));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(struct tm), (char*)&(output_meeting_req->start_time));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(struct tm), (char*)&(output_meeting_req->end_time));
	fetch_string_from_stream    (input_stream, &stream_offset, &output_meeting_req->location);
	fetch_string_from_stream    (input_stream, &stream_offset, &output_meeting_req->global_object_id);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_meeting_req->time_zone.offset_from_GMT));
	fetch_sized_data_from_stream(input_stream, &stream_offset, 32, output_meeting_req->time_zone.standard_name);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(struct tm), (char*)&(output_meeting_req->time_zone.standard_time_start_date));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_meeting_req->time_zone.standard_bias)); 
	fetch_sized_data_from_stream(input_stream, &stream_offset, 32, output_meeting_req->time_zone.daylight_name);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(struct tm), (char*)&(output_meeting_req->time_zone.daylight_time_start_date));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_meeting_req->time_zone.daylight_bias)); 

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC char* em_convert_search_filter_to_byte_stream(email_search_filter_t *input_search_filter_list, int input_search_filter_count, int *output_stream_size)
{
	EM_DEBUG_FUNC_BEGIN("input_search_filter_list [%p] input_search_filter_count [%d]", input_search_filter_list, input_search_filter_count);

	char *result_stream = NULL;
	int   stream_size   = 0;
	int   i = 0;

	EM_IF_NULL_RETURN_VALUE(input_search_filter_list, NULL);

	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_search_filter_count), sizeof(int));

	for( i = 0; i < input_search_filter_count; i++) {
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_search_filter_list->search_filter_type), sizeof(int));
		switch(input_search_filter_list->search_filter_type) {
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       :
			case EMAIL_SEARCH_FILTER_TYPE_UID              :
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      :
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       :
				result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_search_filter_list->search_filter_key_value.integer_type_key_value), sizeof(int));
				break;

			case EMAIL_SEARCH_FILTER_TYPE_BCC              :
			case EMAIL_SEARCH_FILTER_TYPE_CC               :
			case EMAIL_SEARCH_FILTER_TYPE_FROM             :
			case EMAIL_SEARCH_FILTER_TYPE_KEYWORD          :
			case EMAIL_SEARCH_FILTER_TYPE_SUBJECT          :
			case EMAIL_SEARCH_FILTER_TYPE_TO               :
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       :
				result_stream = append_string_to_stream(result_stream, &stream_size, input_search_filter_list->search_filter_key_value.string_type_key_value);
				break;

			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE :
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     :
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  :
				result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_search_filter_list->search_filter_key_value.time_type_key_value), sizeof(struct tm));
				break;
			default :
				EM_DEBUG_EXCEPTION("Invalid filter type [%d]", input_search_filter_list->search_filter_type);
				break;
		}
	}

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END();
	return result_stream;
}

INTERNAL_FUNC void em_convert_byte_stream_to_search_filter(char *input_stream, email_search_filter_t **output_search_filter_list, int *output_search_filter_count)
{
	EM_DEBUG_FUNC_BEGIN("input_stream [%p] output_search_filter_list [%p] output_search_filter_count [%p]", input_stream, output_search_filter_list, output_search_filter_count);

	int stream_offset = 0;
	int i = 0;
	int local_search_filter_count = 0;
	email_search_filter_t *local_search_filter = NULL;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_search_filter_list);
	EM_NULL_CHECK_FOR_VOID(output_search_filter_count);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(local_search_filter_count));

	if(local_search_filter_count == 0) {
		EM_DEBUG_EXCEPTION("local_search_filter_count is 0.");
		goto FINISH_OFF;
	}

	local_search_filter = em_malloc(sizeof(email_search_filter_t) * local_search_filter_count);

	if(local_search_filter == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc for local_search_filter failed");
		goto FINISH_OFF;
	}

	*output_search_filter_count = local_search_filter_count;

	for( i = 0; i < local_search_filter_count; i++) {
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(local_search_filter[i].search_filter_type));
		switch(local_search_filter[i].search_filter_type) {
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       :
			case EMAIL_SEARCH_FILTER_TYPE_UID              :
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      :
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       :
				fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(local_search_filter[i].search_filter_key_value.integer_type_key_value));
				break;

			case EMAIL_SEARCH_FILTER_TYPE_BCC              :
			case EMAIL_SEARCH_FILTER_TYPE_CC               :
			case EMAIL_SEARCH_FILTER_TYPE_FROM             :
			case EMAIL_SEARCH_FILTER_TYPE_KEYWORD          :
			case EMAIL_SEARCH_FILTER_TYPE_SUBJECT          :
			case EMAIL_SEARCH_FILTER_TYPE_TO               :
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       :
				fetch_string_from_stream(input_stream, &stream_offset, &(local_search_filter[i].search_filter_key_value.string_type_key_value));
				break;

			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE :
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     :
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  :
				fetch_sized_data_from_stream(input_stream, &stream_offset, 32, (char*)&(local_search_filter[i].search_filter_key_value.time_type_key_value));
				break;
			default :
				EM_DEBUG_EXCEPTION("Invalid filter type [%d]", local_search_filter[i].search_filter_type);
				break;
		}
	}

	*output_search_filter_list = local_search_filter;

FINISH_OFF:

	EM_DEBUG_FUNC_END();
}
