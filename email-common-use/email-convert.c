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
#include "tpl.h"

#define fSEEN 0x1
#define fDELETED 0x2
#define fFLAGGED 0x4
#define fANSWERED 0x8
#define fOLD 0x10
#define fDRAFT 0x20
#define fATTACHMENT 0x40
#define fFORWARD    0x80

INTERNAL_FUNC int em_convert_mail_tbl_to_mail_status(emstorage_mail_tbl_t *mail_tbl_data, int *result_mail_status, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_tbl_data[%p], result_mail_status [%p], err_code[%p]", mail_tbl_data, result_mail_status, err_code);
	int ret = false, error_code = EMAIL_ERROR_NONE;
	int attachment_count = 0;

	if(!mail_tbl_data || !result_mail_status) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	attachment_count = (mail_tbl_data->attachment_count > mail_tbl_data->inline_content_count) ? 1 : 0;

	*result_mail_status = (mail_tbl_data->flags_seen_field      ? fSEEN       : 0) |
	                      (mail_tbl_data->flags_deleted_field   ? fDELETED    : 0) |
	                      (mail_tbl_data->flags_flagged_field   ? fFLAGGED    : 0) |
	                      (mail_tbl_data->flags_answered_field  ? fANSWERED   : 0) |
	                      (mail_tbl_data->flags_recent_field    ? fOLD        : 0) |
	                      (mail_tbl_data->flags_draft_field     ? fDRAFT      : 0) |
	                      (attachment_count                       ? fATTACHMENT : 0) |
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
	int ret = false, error_code = EMAIL_ERROR_NONE;

	if(!result_mail_tbl_data) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error_code = EMAIL_ERROR_INVALID_PARAM;
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

INTERNAL_FUNC int em_convert_account_to_account_tbl(email_account_t *account, emstorage_account_tbl_t *account_tbl)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], account_tbl[%p]", account, account_tbl);
	int ret = 1;

	account_tbl->account_name                             = EM_SAFE_STRDUP(account->account_name);
	account_tbl->incoming_server_type                     = account->incoming_server_type;
	account_tbl->incoming_server_address                  = EM_SAFE_STRDUP(account->incoming_server_address);
	account_tbl->user_email_address                       = EM_SAFE_STRDUP(account->user_email_address);
	account_tbl->incoming_server_user_name                = EM_SAFE_STRDUP(account->incoming_server_user_name);
	account_tbl->incoming_server_password                 = EM_SAFE_STRDUP(account->incoming_server_password);
	account_tbl->retrieval_mode                           = account->retrieval_mode;
	account_tbl->incoming_server_port_number              = account->incoming_server_port_number;
	account_tbl->incoming_server_secure_connection        = account->incoming_server_secure_connection;
	account_tbl->outgoing_server_type                     = account->outgoing_server_type;
	account_tbl->outgoing_server_address                  = EM_SAFE_STRDUP(account->outgoing_server_address);
	account_tbl->outgoing_server_port_number              = account->outgoing_server_port_number;
	account_tbl->outgoing_server_need_authentication      = account->outgoing_server_need_authentication;
	account_tbl->outgoing_server_secure_connection        = account->outgoing_server_secure_connection;
	account_tbl->outgoing_server_user_name                = EM_SAFE_STRDUP(account->outgoing_server_user_name);
	account_tbl->outgoing_server_password                 = EM_SAFE_STRDUP(account->outgoing_server_password);
	account_tbl->user_display_name                        = EM_SAFE_STRDUP(account->user_display_name);
	account_tbl->reply_to_address                         = EM_SAFE_STRDUP(account->reply_to_address);
	account_tbl->return_address                           = EM_SAFE_STRDUP(account->return_address);
	account_tbl->account_id                               = account->account_id;
	account_tbl->keep_mails_on_pop_server_after_download  = account->keep_mails_on_pop_server_after_download;
	account_tbl->auto_download_size                       = account->auto_download_size;
	account_tbl->outgoing_server_use_same_authenticator   = account->outgoing_server_use_same_authenticator;
	account_tbl->pop_before_smtp                          = account->pop_before_smtp;
	account_tbl->incoming_server_requires_apop            = account->incoming_server_requires_apop;
	account_tbl->logo_icon_path                           = EM_SAFE_STRDUP(account->logo_icon_path);

	account_tbl->user_data                                = em_memdup(account->user_data, account->user_data_length);
	account_tbl->user_data_length                         = account->user_data_length;

	account_tbl->options.priority                         = account->options.priority;
	account_tbl->options.keep_local_copy                  = account->options.keep_local_copy;
	account_tbl->options.req_delivery_receipt             = account->options.req_delivery_receipt;
	account_tbl->options.req_read_receipt                 = account->options.req_read_receipt;
	account_tbl->options.download_limit                   = account->options.download_limit;
	account_tbl->options.block_address                    = account->options.block_address;
	account_tbl->options.block_subject                    = account->options.block_subject;
	account_tbl->options.display_name_from                = EM_SAFE_STRDUP(account->options.display_name_from);
	account_tbl->options.reply_with_body                  = account->options.reply_with_body;
	account_tbl->options.forward_with_files               = account->options.forward_with_files;
	account_tbl->options.add_myname_card                  = account->options.add_myname_card;
	account_tbl->options.add_signature                    = account->options.add_signature;
	account_tbl->options.signature                        = EM_SAFE_STRDUP(account->options.signature);
	account_tbl->options.add_my_address_to_bcc            = account->options.add_my_address_to_bcc;
	account_tbl->check_interval                           = account->check_interval;
	account_tbl->account_svc_id                           = account->account_svc_id;
	account_tbl->sync_status                              = account->sync_status;
	account_tbl->sync_disabled                            = account->sync_disabled;
	account_tbl->default_mail_slot_size                   = account->default_mail_slot_size;
	account_tbl->smime_type                               = account->smime_type;
	account_tbl->certificate_path                         = EM_SAFE_STRDUP(account->certificate_path);
	account_tbl->cipher_type                              = account->cipher_type;
	account_tbl->digest_type                              = account->digest_type;


	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int em_convert_account_tbl_to_account(emstorage_account_tbl_t *account_tbl, email_account_t *account)
{
	EM_DEBUG_FUNC_BEGIN("account_tbl[%p], account[%p]", account_tbl, account);
	int ret = 1;

	account->account_name                             = EM_SAFE_STRDUP(account_tbl->account_name);
	account->incoming_server_type                     = account_tbl->incoming_server_type;
	account->incoming_server_address                  = EM_SAFE_STRDUP(account_tbl->incoming_server_address);
	account->user_email_address                       = EM_SAFE_STRDUP(account_tbl->user_email_address);
	account->incoming_server_user_name                = EM_SAFE_STRDUP(account_tbl->incoming_server_user_name);
	account->incoming_server_password                 = EM_SAFE_STRDUP(account_tbl->incoming_server_password);
	account->retrieval_mode                           = account_tbl->retrieval_mode;
	account->incoming_server_port_number              = account_tbl->incoming_server_port_number;
	account->incoming_server_secure_connection        = account_tbl->incoming_server_secure_connection;
	account->outgoing_server_type                     = account_tbl->outgoing_server_type;
	account->outgoing_server_address                  = EM_SAFE_STRDUP(account_tbl->outgoing_server_address);
	account->outgoing_server_port_number              = account_tbl->outgoing_server_port_number;
	account->outgoing_server_need_authentication      = account_tbl->outgoing_server_need_authentication;
	account->outgoing_server_secure_connection        = account_tbl->outgoing_server_secure_connection;
	account->outgoing_server_user_name                = EM_SAFE_STRDUP(account_tbl->outgoing_server_user_name);
	account->outgoing_server_password                 = EM_SAFE_STRDUP(account_tbl->outgoing_server_password);
	account->user_display_name                        = EM_SAFE_STRDUP(account_tbl->user_display_name);
	account->reply_to_address                         = EM_SAFE_STRDUP(account_tbl->reply_to_address);
	account->return_address                           = EM_SAFE_STRDUP(account_tbl->return_address);
	account->account_id                               = account_tbl->account_id;
	account->keep_mails_on_pop_server_after_download  = account_tbl->keep_mails_on_pop_server_after_download;
	account->auto_download_size                       = account_tbl->auto_download_size;
	account->outgoing_server_use_same_authenticator   = account_tbl->outgoing_server_use_same_authenticator;
	account->pop_before_smtp                          = account_tbl->pop_before_smtp;
	account->incoming_server_requires_apop            = account_tbl->incoming_server_requires_apop;
	account->logo_icon_path                           = EM_SAFE_STRDUP(account_tbl->logo_icon_path);
	account->user_data                     = em_memdup(account_tbl->user_data, account_tbl->user_data_length);
	account->user_data_length              = account_tbl->user_data_length;
	account->options.priority                         = account_tbl->options.priority;
	account->options.keep_local_copy                  = account_tbl->options.keep_local_copy;
	account->options.req_delivery_receipt             = account_tbl->options.req_delivery_receipt;
	account->options.req_read_receipt                 = account_tbl->options.req_read_receipt;
	account->options.download_limit                   = account_tbl->options.download_limit;
	account->options.block_address                    = account_tbl->options.block_address;
	account->options.block_subject                    = account_tbl->options.block_subject;
	account->options.display_name_from                = EM_SAFE_STRDUP(account_tbl->options.display_name_from);
	account->options.reply_with_body                  = account_tbl->options.reply_with_body;
	account->options.forward_with_files               = account_tbl->options.forward_with_files;
	account->options.add_myname_card                  = account_tbl->options.add_myname_card;
	account->options.add_signature                    = account_tbl->options.add_signature;
	account->options.signature                        = EM_SAFE_STRDUP(account_tbl->options.signature);
	account->options.add_my_address_to_bcc            = account_tbl->options.add_my_address_to_bcc;
	account->check_interval                           = account_tbl->check_interval;
	account->account_svc_id                           = account_tbl->account_svc_id;
	account->sync_status                              = account_tbl->sync_status;
	account->sync_disabled                            = account_tbl->sync_disabled;
	account->default_mail_slot_size                   = account_tbl->default_mail_slot_size;
	account->smime_type                               = account_tbl->smime_type;
	account->certificate_path                         = EM_SAFE_STRDUP(account_tbl->certificate_path);
	account->cipher_type                              = account_tbl->cipher_type;
	account->digest_type                              = account_tbl->digest_type;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int em_convert_mailbox_to_mailbox_tbl(email_mailbox_t *mailbox, emstorage_mailbox_tbl_t *mailbox_tbl)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mailbox_tbl[%p]", mailbox, mailbox_tbl);
	int ret = 1;

	mailbox_tbl->account_id                 = mailbox->account_id;
	mailbox_tbl->mailbox_id                 = mailbox->mailbox_id;
	mailbox_tbl->mailbox_name               = EM_SAFE_STRDUP(mailbox->mailbox_name);
	mailbox_tbl->alias                      = EM_SAFE_STRDUP(mailbox->alias);
	mailbox_tbl->local_yn                   = mailbox->local;
	mailbox_tbl->mailbox_type               = mailbox->mailbox_type;
	mailbox_tbl->unread_count               = mailbox->unread_count;
	mailbox_tbl->total_mail_count_on_local  = mailbox->total_mail_count_on_local;
	mailbox_tbl->total_mail_count_on_server = mailbox->total_mail_count_on_server;
	mailbox_tbl->mail_slot_size             = mailbox->mail_slot_size;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int em_convert_mailbox_tbl_to_mailbox(emstorage_mailbox_tbl_t *mailbox_tbl, email_mailbox_t *mailbox)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_tbl[%p], mailbox[%p]", mailbox_tbl, mailbox);
	int ret = 1;

	mailbox->account_id                 = mailbox_tbl->account_id;
	mailbox->mailbox_id                 = mailbox_tbl->mailbox_id;
	mailbox->mailbox_name               = EM_SAFE_STRDUP(mailbox_tbl->mailbox_name);
	mailbox->alias                      = EM_SAFE_STRDUP(mailbox_tbl->alias);
	mailbox->local                      = mailbox_tbl->local_yn;
	mailbox->mailbox_type               = mailbox_tbl->mailbox_type;
	mailbox->unread_count               = mailbox_tbl->unread_count;
	mailbox->total_mail_count_on_local  = mailbox_tbl->total_mail_count_on_local;
	mailbox->total_mail_count_on_server = mailbox_tbl->total_mail_count_on_server;
	mailbox->mail_slot_size             = mailbox_tbl->mail_slot_size;
	mailbox->last_sync_time             = mailbox_tbl->last_sync_time;

	EM_DEBUG_FUNC_END();
	return ret;
}


INTERNAL_FUNC int em_convert_mail_tbl_to_mail_data(emstorage_mail_tbl_t *mail_table_data, int item_count, email_mail_data_t **mail_data, int *error)
{
	EM_DEBUG_FUNC_BEGIN("mail_table_data[%p], item_count [%d], mail_data[%p]", mail_table_data, item_count, mail_data);
	int i, ret = false, err_code = EMAIL_ERROR_NONE;
	email_mail_data_t *temp_mail_data = NULL;

	if (!mail_table_data || !mail_data || !item_count) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	temp_mail_data = em_malloc(sizeof(email_mail_data_t) * item_count);

	if(!temp_mail_data) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err_code = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < item_count; i++) {
		temp_mail_data[i].mail_id                 = mail_table_data[i].mail_id;
		temp_mail_data[i].account_id              = mail_table_data[i].account_id;
		temp_mail_data[i].mailbox_id              = mail_table_data[i].mailbox_id;
		temp_mail_data[i].mailbox_type            = mail_table_data[i].mailbox_type;
		temp_mail_data[i].subject                 = EM_SAFE_STRDUP(mail_table_data[i].subject);
		temp_mail_data[i].date_time               = mail_table_data[i].date_time;
		temp_mail_data[i].server_mail_status      = mail_table_data[i].server_mail_status;
		temp_mail_data[i].server_mailbox_name     = EM_SAFE_STRDUP(mail_table_data[i].server_mailbox_name);
		temp_mail_data[i].server_mail_id          = EM_SAFE_STRDUP(mail_table_data[i].server_mail_id);
		temp_mail_data[i].message_id              = EM_SAFE_STRDUP(mail_table_data[i].message_id);
		temp_mail_data[i].reference_mail_id       = mail_table_data[i].reference_mail_id;
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
		temp_mail_data[i].file_path_mime_entity   = EM_SAFE_STRDUP(mail_table_data[i].file_path_mime_entity);
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
		temp_mail_data[i].message_class           = mail_table_data[i].message_class;
		temp_mail_data[i].digest_type             = mail_table_data[i].digest_type;
		temp_mail_data[i].smime_type              = mail_table_data[i].smime_type;
	}

	*mail_data = temp_mail_data;

	ret = true;
FINISH_OFF:

	if(error)
		*error = err_code;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int   em_convert_mail_data_to_mail_tbl(email_mail_data_t *mail_data, int item_count, emstorage_mail_tbl_t **mail_table_data, int *error)
{
	EM_DEBUG_FUNC_BEGIN("mail_data[%p], item_count [%d], mail_table_data[%p]", mail_data, item_count, mail_table_data);
	int i, ret = false, err_code = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t *temp_mail_tbl = NULL;

	if (!mail_data || !mail_table_data || !item_count) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	temp_mail_tbl = em_malloc(sizeof(emstorage_mail_tbl_t) * item_count);

	if(!temp_mail_tbl) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err_code = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < item_count; i++) {
		temp_mail_tbl[i].mail_id                 = mail_data[i].mail_id;
		temp_mail_tbl[i].account_id              = mail_data[i].account_id;
		temp_mail_tbl[i].mailbox_id              = mail_data[i].mailbox_id;
		temp_mail_tbl[i].mailbox_type            = mail_data[i].mailbox_type;
		temp_mail_tbl[i].date_time               = mail_data[i].date_time;
		temp_mail_tbl[i].subject                 = EM_SAFE_STRDUP(mail_data[i].subject);
		temp_mail_tbl[i].server_mail_status      = mail_data[i].server_mail_status;
		temp_mail_tbl[i].server_mailbox_name     = EM_SAFE_STRDUP(mail_data[i].server_mailbox_name);
		temp_mail_tbl[i].server_mail_id          = EM_SAFE_STRDUP(mail_data[i].server_mail_id);
		temp_mail_tbl[i].message_id              = EM_SAFE_STRDUP(mail_data[i].message_id);
		temp_mail_tbl[i].reference_mail_id       = mail_data[i].reference_mail_id;
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
		temp_mail_tbl[i].file_path_mime_entity   = EM_SAFE_STRDUP(mail_data[i].file_path_mime_entity);
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
		temp_mail_tbl[i].message_class           = mail_data[i].message_class;
		temp_mail_tbl[i].digest_type             = mail_data[i].digest_type;
		temp_mail_tbl[i].smime_type              = mail_data[i].smime_type;
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
		return EMAIL_ERROR_INVALID_PARAM;
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

	EM_DEBUG_FUNC_END("err %d", EMAIL_ERROR_NONE);
	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC int em_convert_time_t_to_string(time_t *input_time, char **output_datetime_string)
{
	EM_DEBUG_FUNC_BEGIN("input_time[%p], output_datetime_string[%p]", input_time, output_datetime_string);
	char temp_buffer[20] = { 0, };
	struct tm *temp_time_info;

	if (!input_time || !output_datetime_string) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	temp_time_info = localtime(input_time);

	if(!temp_time_info) {
		EM_DEBUG_EXCEPTION("localtime failed.");
		return EMAIL_ERROR_SYSTEM_FAILURE;
	}
	SNPRINTF(temp_buffer, sizeof(temp_buffer), "%04d%02d%02d%02d%02d%02d",
		temp_time_info->tm_year + 1970, temp_time_info->tm_mon, temp_time_info->tm_mday, temp_time_info->tm_hour, temp_time_info->tm_min, temp_time_info->tm_sec);

	*output_datetime_string = EM_SAFE_STRDUP(temp_buffer);

	EM_DEBUG_FUNC_END("err %d", EMAIL_ERROR_NONE);
	return EMAIL_ERROR_NONE;
}

static char* append_sized_data_to_stream(char *stream, int *stream_len, char *src, int src_len)
{
	/* EM_DEBUG_FUNC_BEGIN("input_stream [%p], input_output_stream_length [%p], input_sized_data [%p], input_data_size [%d]", input_stream, input_output_stream_length, input_sized_data, input_data_size); */
	char *new_stream = NULL;

	if( !stream_len || src_len == 0 || src == NULL || (stream != NULL && *stream_len == 0) ||
			(stream == NULL && *stream_len != 0) ) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return NULL;
	}

	int old_stream_len = *stream_len;

	/*TODO: don't increase stream buffer incrementally when appending new data */
	new_stream = (char*)em_malloc(old_stream_len + src_len);

	if(!new_stream) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		return NULL;
	}

	if(stream != NULL)
		memcpy(new_stream, stream, old_stream_len);

	memcpy(new_stream + old_stream_len, src, src_len);

	*stream_len = old_stream_len + src_len;

	EM_SAFE_FREE(stream);
	/* EM_DEBUG_FUNC_END("*input_output_stream_length [%d]", *input_output_stream_length); */
	return new_stream;
}


static char* append_string_to_stream(char *input_stream, int *input_output_stream_length, char *input_source_string)
{
	EM_DEBUG_FUNC_BEGIN("input_stream [%p], input_output_stream_length [%p] input_source_string[%p]", input_stream, input_output_stream_length, input_source_string);
	char *new_stream = NULL;
	int   data_length = 0;

	if( !input_output_stream_length || (input_stream != NULL && *input_output_stream_length == 0) ||
		(input_stream == NULL && *input_output_stream_length != 0) ) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return NULL;
	}

	int source_stream_length = *input_output_stream_length;

	data_length = EM_SAFE_STRLEN(input_source_string);

	new_stream = (char*)em_malloc(source_stream_length + data_length + sizeof(int));

	if(!new_stream) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		return NULL;
	}

	if(input_stream != NULL)
		memcpy(new_stream, input_stream, source_stream_length);

	/* write string length */
	memcpy(new_stream + source_stream_length, (char*)&data_length, sizeof(int));

	/* write string */
	if(input_source_string)
		memcpy(new_stream + source_stream_length + sizeof(int), input_source_string, data_length);

	/* for example, "abc" is written to stream buffer with "3abc" */
	*input_output_stream_length = source_stream_length + sizeof(int) + data_length;

	EM_SAFE_FREE(input_stream);
	EM_DEBUG_FUNC_END("*input_output_stream_length [%d]", *input_output_stream_length);
	return new_stream;
}

#if 0
static char* append_binary_to_stream(char *stream, int *stream_length, char *src, int src_size)
{
	/* EM_DEBUG_FUNC_BEGIN("input_stream [%p], input_output_stream_length [%p], input_sized_data [%p], input_data_size [%d]", input_stream, input_output_stream_length, input_sized_data, input_data_size); */
	char *new_stream = NULL;

	if( !stream_length || (stream && *stream_length == 0) || (!stream && *stream_length != 0) ||
		src_size < 0 ) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return NULL;
	}

	/*TODO: don't increase stream buffer incrementally when appending new data */
	new_stream = (char*)em_malloc(*stream_length + sizeof(int) + src_size);

	if(!new_stream) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		return NULL;
	}


	if(stream != NULL)
		memcpy(new_stream, stream, *stream_length);

	memcpy(new_stream + *stream_length, &src_size, sizeof(int));

	if( src_size > 0 )
		memcpy(new_stream + *stream_length + sizeof(int), src, src_size);

	*stream_length = *stream_length + sizeof(int) + src_size;

	EM_SAFE_FREE(stream);
	/* EM_DEBUG_FUNC_END("*input_output_stream_length [%d]", *input_output_stream_length); */

	return new_stream;
}
#endif

static int fetch_sized_data_from_stream(char *input_stream, int *input_output_stream_offset, int input_data_size, char *output_data)
{
	/* EM_DEBUG_FUNC_BEGIN("input_stream [%p], input_output_stream_offset [%p] input_data_size [%d], output_data[%p]", input_stream, input_output_stream_offset, input_data_size, output_data); */
	int stream_offset = 0;

	if( !input_stream || !input_output_stream_offset || !input_data_size || !output_data) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	stream_offset = *input_output_stream_offset;

	memcpy(output_data, input_stream + stream_offset, input_data_size);
	stream_offset += input_data_size;

	*input_output_stream_offset = stream_offset;

	/* EM_DEBUG_FUNC_END("stream_offset [%d]", stream_offset); */
	return EMAIL_ERROR_NONE;
}


static int fetch_string_from_stream(char *input_stream, int *input_output_stream_offset, char **output_string)
{
	/* EM_DEBUG_FUNC_BEGIN("input_stream [%p], input_output_stream_offset [%p] output_string[%p]", input_stream, input_output_stream_offset, output_string); */
	int string_length = 0;
	int stream_offset = 0;
	char *result_string = NULL;

	if( !input_stream || !input_output_stream_offset || !output_string) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	stream_offset = *input_output_stream_offset;
	/*	EM_DEBUG_LOG("stream_offset [%d]", stream_offset);	*/

	memcpy(&string_length, input_stream + stream_offset, sizeof(int));
	stream_offset +=  sizeof(int);

	/*	EM_DEBUG_LOG("string_length [%d]", string_length);	*/

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

	/* EM_DEBUG_FUNC_END("stream_offset [%d]", stream_offset); */
	return EMAIL_ERROR_NONE;
}

#if 0
static int fetch_binary_from_stream(char *stream, int *stream_offset, void **dest)
{
	/* EM_DEBUG_FUNC_BEGIN("input_stream [%p], input_output_stream_offset [%p] output_string[%p]", input_stream, input_output_stream_offset, output_string); */
	int length = 0;

	if( !stream || !stream_offset || !dest) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int offset = *stream_offset;

	memcpy((void*)&length, (void*) stream + offset, sizeof(int));
	offset +=  sizeof(int);

	*dest = NULL;
	if(length > 0) {
		*dest = (void*)em_malloc(length);
		if(!*dest) {
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
			return EMAIL_ERROR_OUT_OF_MEMORY;
		}

		memcpy(*dest, (void*) stream + offset, length);
		offset += length;
	}

	*stream_offset = offset;

	return EMAIL_ERROR_NONE;
}
#endif
                                    /* divide struct at binary field (void* user_data)*/
#define EMAIL_ACCOUNT_FMT   "S(" "isiii" "is" ")" "B" "S(" "issss"  "isiss" "iiiii" "isiss" "iii"\
                                 "$(" "iiiii" "iisii" "iisi" ")" "iiisii" ")"


INTERNAL_FUNC char* em_convert_account_to_byte_stream(email_account_t* account, int *stream_len)
{
	EM_DEBUG_FUNC_END();
	EM_IF_NULL_RETURN_VALUE(account, NULL);

	tpl_node *tn = NULL;
	tpl_bin tb;

	tn = tpl_map(EMAIL_ACCOUNT_FMT, account, &tb, &(account->user_data_length));
	tb.sz = account->user_data_length;
	tb.addr = account->user_data;
	tpl_pack(tn, 0);

	/* write account to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;
	EM_DEBUG_FUNC_END();
	return (char*) buf;

#if 0
	EM_DEBUG_FUNC_BEGIN("input_account [%p], output_stream_size [%p]", input_account, output_stream_size);
	char *result_stream = NULL;
	int stream_size = 0;

	EM_IF_NULL_RETURN_VALUE(input_account, NULL);

	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->account_name);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->incoming_server_type), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->incoming_server_address);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->user_email_address);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->incoming_server_user_name);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->incoming_server_password);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->retrieval_mode), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->incoming_server_port_number), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->incoming_server_secure_connection), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->outgoing_server_type), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->outgoing_server_address);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->outgoing_server_port_number), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->outgoing_server_need_authentication), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->outgoing_server_secure_connection), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->outgoing_server_user_name);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->outgoing_server_password);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->user_display_name);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->reply_to_address);
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->return_address);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->account_id), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->keep_mails_on_pop_server_after_download), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->auto_download_size), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->outgoing_server_use_same_authenticator), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->pop_before_smtp), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->incoming_server_requires_apop), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->logo_icon_path);
	result_stream = append_binary_to_stream(result_stream, &stream_size, input_account->user_data, input_account->user_data_length);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*) &input_account->user_data_length, sizeof(int));
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
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->check_interval), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->account_svc_id), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->sync_status), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->sync_disabled), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->default_mail_slot_size), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->smime_type), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_account->certificate_path);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->cipher_type), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_account->digest_type), sizeof(int));

	*output_stream_size = stream_size;


	EM_DEBUG_FUNC_END("stream_size [%d]", stream_size);
	return result_stream;
#endif
}


INTERNAL_FUNC void em_convert_byte_stream_to_account(char *stream, int stream_len, email_account_t *account)
{
	EM_DEBUG_FUNC_END();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(account);

	tpl_node *tn = NULL;
	tpl_bin tb;

	tn = tpl_map(EMAIL_ACCOUNT_FMT, account, &tb, &(account->user_data_length));
	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);
	tpl_free(tn);

	/* tb will be destroyed at end of func, but tb.addr remains */
	account->user_data = tb.addr;

	EM_DEBUG_FUNC_END();
#if 0
	EM_DEBUG_FUNC_BEGIN();
	int stream_offset = 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_account);

	fetch_string_from_stream(input_stream, &stream_offset, &output_account->account_name);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->incoming_server_type);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->incoming_server_address);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->user_email_address);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->incoming_server_user_name);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->incoming_server_password);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->retrieval_mode);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->incoming_server_port_number);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->incoming_server_secure_connection);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->outgoing_server_type);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->outgoing_server_address);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->outgoing_server_port_number);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->outgoing_server_need_authentication);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->outgoing_server_secure_connection);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->outgoing_server_user_name);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->outgoing_server_password);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->user_display_name);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->reply_to_address);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->return_address);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->account_id);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->keep_mails_on_pop_server_after_download);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->auto_download_size);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->outgoing_server_use_same_authenticator);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->pop_before_smtp);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->incoming_server_requires_apop);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->logo_icon_path);
	fetch_binary_from_stream(input_stream, &stream_offset, &output_account->user_data);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->user_data_length);
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
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->check_interval);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->account_svc_id);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->sync_status);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->sync_disabled);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->default_mail_slot_size);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->smime_type);
	fetch_string_from_stream(input_stream, &stream_offset, &output_account->certificate_path);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->cipher_type);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&output_account->digest_type);

	EM_DEBUG_FUNC_END();
#endif
}

#define EMAIL_MAIL_DATA_FMT  "S(" "iiiis" "iisss" "issss" "sssss" "sisss"\
                            "icccc" "cccii" "iiiii" "iisii" "ii" ")"

INTERNAL_FUNC char* em_convert_mail_data_to_byte_stream(email_mail_data_t *mail_data, int *stream_len)
{
	EM_DEBUG_FUNC_END();
	EM_IF_NULL_RETURN_VALUE(mail_data, NULL);
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);

	tpl_node *tn = NULL;

	tn = tpl_map(EMAIL_MAIL_DATA_FMT, mail_data);
	tpl_pack(tn, 0);

	/* write account to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;
	EM_DEBUG_FUNC_END();
	return (char*) buf;

#if 0
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
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].mailbox_id), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].mailbox_type), sizeof(int));
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].subject);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].date_time), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].server_mail_status), sizeof(int));
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].server_mailbox_name);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].server_mail_id);
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].message_id);
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].reference_mail_id), sizeof(int));
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
		result_stream = append_string_to_stream(result_stream, &stream_size, input_mail_data[i].file_path_mime_entity);
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
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].message_class), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].digest_type), sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mail_data[i].smime_type), sizeof(int));
	}

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END("stream_size [%d]", stream_size);
	return result_stream;
#endif
}

INTERNAL_FUNC void em_convert_byte_stream_to_mail_data(char *stream, int stream_len, email_mail_data_t *mail_data)
{
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(mail_data);

	tpl_node *tn = NULL;

	tn = tpl_map(EMAIL_MAIL_DATA_FMT, mail_data);
	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);
	tpl_free(tn);

	EM_DEBUG_FUNC_END();
#if 0
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

	*output_mail_data = (email_mail_data_t*)em_malloc(sizeof(email_mail_data_t) * (*output_mail_data_count));

	if(!*output_mail_data) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return;
	}

	for(i = 0; i < *output_mail_data_count; i++) {
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].mail_id);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].account_id);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].mailbox_id);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].mailbox_type);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].subject);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].date_time);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].server_mail_status);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].server_mailbox_name);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].server_mail_id);
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].message_id);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].reference_mail_id);
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
		fetch_string_from_stream(input_stream, &stream_offset, &(*output_mail_data)[i].file_path_mime_entity);
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
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].message_class);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].digest_type);
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(*output_mail_data)[i].smime_type);
	}

	EM_DEBUG_FUNC_END();
#endif
}


#define EMAIL_ATTACHMENT_DATA_FMT "A(S(" "issii" "iciii" "s" "))"

INTERNAL_FUNC char* em_convert_attachment_data_to_byte_stream(email_attachment_data_t *attachment, int attachment_count, int* stream_len)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);
	if(!attachment) {
		EM_DEBUG_LOG("no attachment to be included");
		*stream_len = 0;
		return NULL;
	}


	email_attachment_data_t cur = {0};
	tpl_node *tn = NULL;

	/* tpl_map adds value at 2nd param addr to packing buffer iterately */
	/* 2nd param value (not addr via pointer) should be modified at each iteration */
	tn = tpl_map(EMAIL_ATTACHMENT_DATA_FMT, &cur);
	int i=0;
	for( ; i < attachment_count ; i++ ) {
		memcpy(&cur, attachment+i, sizeof(cur)); /* copy data to cur : swallow copy */
		tpl_pack(tn, 1);                        /* pack data at &cur: deep copy */
	}

	/* write data to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;

	EM_DEBUG_LOG("stream_len: %d", len);

	EM_DEBUG_FUNC_END();
	return (char*) buf;

#if 0
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
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].mailbox_id, sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].save_status, sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].drm_status, sizeof(int));
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&input_attachment_data[i].inline_content_status,sizeof(int));
	}

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END("stream_size [%d]", stream_size);
	return result_stream;
#endif
}

INTERNAL_FUNC void em_convert_byte_stream_to_attachment_data(char *stream, int stream_len, email_attachment_data_t **attachment_data, int *attachment_count)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(attachment_data);
	EM_NULL_CHECK_FOR_VOID(attachment_count);


	email_attachment_data_t cur = {0};
	tpl_node *tn = NULL;
	tn = tpl_map(EMAIL_ATTACHMENT_DATA_FMT, &cur);
	tpl_load(tn, TPL_MEM, stream, stream_len);

	/* tpl does not return the size of variable-length array, but we need variable-length array */
	/* so, make list and get list count in the first phase, */
	/* and then copy list to var array after allocating memory */
	GList *head = NULL;
	int count = 0;
	while( tpl_unpack(tn, 1) > 0) {
		email_attachment_data_t* pdata = (email_attachment_data_t*) em_malloc(sizeof(email_attachment_data_t));
		memcpy(pdata, &cur, sizeof(email_attachment_data_t)); /* copy unpacked data to list item */
		head = g_list_prepend(head, pdata);                   /* add it to list */
		memset(&cur, 0, sizeof(email_attachment_data_t));     /* initialize variable, used for unpacking */
		count++;
	}
	tpl_free(tn);

	/*finally we get the list count and allocate var length array */
	email_attachment_data_t *attached = (email_attachment_data_t*) em_malloc(sizeof(email_attachment_data_t)*count);

	/*write glist item into variable array*/
	head = g_list_reverse(head);
	GList *p = g_list_first(head);
	int i=0;
	for( ; p ; p = g_list_next(p), i++ ) {
		email_attachment_data_t* pdata = (email_attachment_data_t*) g_list_nth_data(p, 0);
		memcpy( attached+i, pdata, sizeof(email_attachment_data_t));
		EM_SAFE_FREE(pdata);    /*now, list item is useless */
	}

	g_list_free(head);

	*attachment_count = count;
	*attachment_data = attached;
	EM_DEBUG_FUNC_END();
#if 0
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

	*output_attachment_data = (email_attachment_data_t*)em_malloc(sizeof(email_attachment_data_t) * (*output_attachment_count));

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
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].mailbox_id));
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].save_status));
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].drm_status));
		fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&((*output_attachment_data)[i].inline_content_status));
	}

	EM_DEBUG_FUNC_END();
#endif
}


#define EMAIL_MAILBOX_FMT  "S(" "isisi" "iiiii" "i" ")"

INTERNAL_FUNC char* em_convert_mailbox_to_byte_stream(email_mailbox_t *mailbox_data, int *stream_len)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_IF_NULL_RETURN_VALUE(mailbox_data, NULL);
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);

	tpl_node *tn = NULL;

	tn = tpl_map(EMAIL_MAILBOX_FMT, mailbox_data);
	tpl_pack(tn, 0);

	/* write account to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;
	EM_DEBUG_FUNC_END("serialized len: %d", len);
	return (char*) buf;

#if 0
	EM_DEBUG_FUNC_BEGIN("input_mailbox_data [%p], output_stream_size [%p]", input_mailbox_data, output_stream_size);
	
	char *result_stream = NULL;
	int   stream_size 	=  0;

	EM_IF_NULL_RETURN_VALUE(input_mailbox_data, NULL);
	EM_IF_NULL_RETURN_VALUE(output_stream_size, NULL);

	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->mailbox_id), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_mailbox_data->mailbox_name);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->mailbox_type), sizeof(int));
	result_stream = append_string_to_stream(result_stream, &stream_size, input_mailbox_data->alias);
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->unread_count), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->total_mail_count_on_local), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->total_mail_count_on_server), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->local), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->account_id), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_mailbox_data->mail_slot_size), sizeof(int));
	
	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END();

	return result_stream;
#endif
}


INTERNAL_FUNC void em_convert_byte_stream_to_mailbox(char *stream, int stream_len, email_mailbox_t *mailbox_data)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(mailbox_data);

	tpl_node *tn = NULL;

	tn = tpl_map(EMAIL_MAILBOX_FMT, mailbox_data);
	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);
	tpl_free(tn);

	EM_DEBUG_FUNC_END("deserialized len %d", stream_len);

/*	EM_DEBUG_FUNC_BEGIN("input_stream [%p], output_mailbox_data [%p]", input_stream, output_mailbox_data);
	int 		stream_offset 	= 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_mailbox_data);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->mailbox_id));
	fetch_string_from_stream(input_stream, &stream_offset, &output_mailbox_data->mailbox_name);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->mailbox_type));
	fetch_string_from_stream(input_stream, &stream_offset, &output_mailbox_data->alias);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->unread_count));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->total_mail_count_on_local));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->total_mail_count_on_server));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->local));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->account_id));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_mailbox_data->mail_slot_size));
	EM_DEBUG_FUNC_END();
*/
}

#define EMAIL_OPTION_FMT "S(" "iiiii" "iisii" "iisi" ")"

INTERNAL_FUNC char* em_convert_option_to_byte_stream(email_option_t* option, int* stream_len)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_IF_NULL_RETURN_VALUE(option, NULL);
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);

	tpl_node *tn = NULL;

	tn = tpl_map(EMAIL_OPTION_FMT, option);
	tpl_pack(tn, 0);

	/* write account to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;
	EM_DEBUG_FUNC_END("serialized len: %d", len);
	return (char*) buf;


#if 0
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
#endif
}

INTERNAL_FUNC void em_convert_byte_stream_to_option(char *stream, int stream_len, email_option_t *option)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(option);

	tpl_node *tn = NULL;

	tn = tpl_map(EMAIL_OPTION_FMT, option);
	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);
	tpl_free(tn);

	EM_DEBUG_FUNC_END("deserialized len %d", stream_len);

#if 0
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
#endif
}


#define EMAIL_RULE_FMT "S(" "iiisi" "iii" ")"

INTERNAL_FUNC char* em_convert_rule_to_byte_stream(email_rule_t *rule, int *stream_len)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_IF_NULL_RETURN_VALUE(rule, NULL);
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);

	tpl_node *tn = NULL;

	tn = tpl_map(EMAIL_RULE_FMT, rule);
	tpl_pack(tn, 0);

	/* write account to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;
	EM_DEBUG_FUNC_END("serialized len: %d", len);
	return (char*) buf;


#if 0
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
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_rule->target_mailbox_id), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_rule->flag1), sizeof(int));
	result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_rule->flag2), sizeof(int));

	*output_stream_size = stream_size;

	EM_DEBUG_FUNC_END();
	return result_stream;
#endif
}

INTERNAL_FUNC void em_convert_byte_stream_to_rule(char *stream, int stream_len, email_rule_t *rule)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(rule);

	tpl_node *tn = NULL;

	tn = tpl_map(EMAIL_RULE_FMT, rule);
	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);
	tpl_free(tn);

	EM_DEBUG_FUNC_END("deserialized len %d", stream_len);



#if 0
	EM_DEBUG_FUNC_BEGIN();
	int stream_offset = 0;

	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_rule);

	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->account_id));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->filter_id));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->type));
	fetch_string_from_stream(input_stream, &stream_offset, &output_rule->value);
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->faction));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->target_mailbox_id));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->flag1));
	fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(output_rule->flag2));

	EM_DEBUG_FUNC_END();
#endif
}

#define EMAIL_MEETING_REQUEST_FMT   "iiBBs" "sic#Bi" "c#Bi"

INTERNAL_FUNC char* em_convert_meeting_req_to_byte_stream(email_meeting_request_t *meeting_req, int *stream_len)
{

	EM_DEBUG_FUNC_END();
	EM_IF_NULL_RETURN_VALUE(meeting_req, NULL);

	tpl_node *tn = NULL;
	tpl_bin tb[4];

	tn = tpl_map(EMAIL_MEETING_REQUEST_FMT,
						&meeting_req->mail_id,
						&meeting_req->meeting_response,
						&tb[0],
						&tb[1],
						&meeting_req->location,
						&meeting_req->global_object_id,
						&meeting_req->time_zone.offset_from_GMT,
						meeting_req->time_zone.standard_name, 32,
						&tb[2],
						&meeting_req->time_zone.standard_bias,
						meeting_req->time_zone.daylight_name, 32,
						&tb[3],
						&meeting_req->time_zone.daylight_bias
				);
	tb[0].sz = tb[1].sz = tb[2].sz = tb[3].sz = sizeof(struct tm);
	tb[0].addr = &meeting_req->start_time;
	tb[1].addr = &meeting_req->end_time;
	tb[2].addr = &meeting_req->time_zone.standard_time_start_date;
	tb[3].addr = &meeting_req->time_zone.daylight_time_start_date;


	tpl_pack(tn, 0);

	/* write account to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;
	EM_DEBUG_FUNC_END();
	return (char*) buf;


#if 0
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
#endif
}


INTERNAL_FUNC void em_convert_byte_stream_to_meeting_req(char *stream, int stream_len, email_meeting_request_t *meeting_req)
{
	EM_DEBUG_FUNC_END();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(meeting_req);

	tpl_node *tn = NULL;
	tpl_bin tb[4];

	tn = tpl_map(EMAIL_MEETING_REQUEST_FMT,
						&meeting_req->mail_id,
						&meeting_req->meeting_response,
						&tb[0],
						&tb[1],
						&meeting_req->location,
						&meeting_req->global_object_id,
						&meeting_req->time_zone.offset_from_GMT,
						meeting_req->time_zone.standard_name, 32,
						&tb[2],
						&meeting_req->time_zone.standard_bias,
						meeting_req->time_zone.daylight_name, 32,
						&tb[3],
						&meeting_req->time_zone.daylight_bias
				);
	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);
	tpl_free(tn);

	/* tb will be destroyed at end of func, but tb.addr remains */
	memcpy(&meeting_req->start_time, tb[0].addr, sizeof(struct tm));
	memcpy(&meeting_req->end_time, tb[1].addr, sizeof(struct tm));
	memcpy(&meeting_req->time_zone.standard_time_start_date, tb[2].addr, sizeof(struct tm));
	memcpy(&meeting_req->time_zone.daylight_time_start_date, tb[3].addr, sizeof(struct tm));

	int i=0;
	for(i=0; i< 4 ; i++)
		EM_SAFE_FREE(tb[i].addr);

	EM_DEBUG_FUNC_END();

#if 0
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
#endif
}

INTERNAL_FUNC char* em_convert_search_filter_to_byte_stream(email_search_filter_t *input_search_filter_list,
									int input_search_filter_count, int *output_stream_size)
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

INTERNAL_FUNC void em_convert_byte_stream_to_search_filter(char *input_stream,
				email_search_filter_t **output_search_filter_list, int *output_search_filter_count)
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

INTERNAL_FUNC int em_convert_certificate_tbl_to_certificate(emstorage_certificate_tbl_t *certificate_tbl, email_certificate_t **certificate, int *error)
{
	EM_DEBUG_FUNC_BEGIN("certficate_tbl[%p], certificate[%p]", certificate_tbl, certificate);

	int err_code = EMAIL_ERROR_NONE;
	int ret = false;
	email_certificate_t *temp_certificate = NULL;

	if (!certificate_tbl || !certificate)  {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	temp_certificate = em_malloc(sizeof(email_certificate_t)) ;
	if (!temp_certificate) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err_code = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	temp_certificate->certificate_id = certificate_tbl->certificate_id;
	temp_certificate->issue_year = certificate_tbl->issue_year;
	temp_certificate->issue_month = certificate_tbl->issue_month;
	temp_certificate->issue_day = certificate_tbl->issue_day;
	temp_certificate->expiration_year = certificate_tbl->expiration_year;
	temp_certificate->expiration_month = certificate_tbl->expiration_month;
	temp_certificate->expiration_day = certificate_tbl->expiration_day;
	temp_certificate->issue_organization_name = EM_SAFE_STRDUP(certificate_tbl->issue_organization_name);
	temp_certificate->email_address = EM_SAFE_STRDUP(certificate_tbl->email_address);
	temp_certificate->subject_str = EM_SAFE_STRDUP(certificate_tbl->subject_str);
	temp_certificate->filepath = EM_SAFE_STRDUP(certificate_tbl->filepath);

	*certificate = temp_certificate;

	ret = true;
FINISH_OFF:
	if (error)
		*error = err_code;

	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int em_convert_certificate_to_certificate_tbl(email_certificate_t *certificate, emstorage_certificate_tbl_t *certificate_tbl)
{
	EM_DEBUG_FUNC_BEGIN("certficate[%p], certificate_tbl[%p]", certificate, certificate_tbl);

	certificate_tbl->certificate_id = certificate->certificate_id;
	certificate_tbl->issue_year = certificate->issue_year;
	certificate_tbl->issue_month = certificate->issue_month;
	certificate_tbl->issue_day = certificate->issue_day;
	certificate_tbl->expiration_year = certificate->expiration_year;
	certificate_tbl->expiration_month = certificate->expiration_month;
	certificate_tbl->expiration_day = certificate->expiration_day;
	certificate_tbl->issue_organization_name = EM_SAFE_STRDUP(certificate->issue_organization_name);
	certificate_tbl->email_address = EM_SAFE_STRDUP(certificate->email_address);
	certificate_tbl->subject_str = EM_SAFE_STRDUP(certificate->subject_str);
	certificate_tbl->filepath = EM_SAFE_STRDUP(certificate->filepath);

	EM_DEBUG_FUNC_END();
	return true;
}

