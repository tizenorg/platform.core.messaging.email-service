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


INTERNAL_FUNC int em_convert_account_to_account_tbl(email_account_t *account, emstorage_account_tbl_t *account_tbl)
{
	EM_DEBUG_FUNC_BEGIN("account[%p], account_tbl[%p]", account, account_tbl);
	int ret = 1;

	account_tbl->account_name                                  = EM_SAFE_STRDUP(account->account_name);
	account_tbl->incoming_server_type                          = account->incoming_server_type;
	account_tbl->incoming_server_address                       = EM_SAFE_STRDUP(account->incoming_server_address);
	account_tbl->user_email_address                            = EM_SAFE_STRDUP(account->user_email_address);
	account_tbl->incoming_server_user_name                     = EM_SAFE_STRDUP(account->incoming_server_user_name);
	account_tbl->incoming_server_password                      = EM_SAFE_STRDUP(account->incoming_server_password);
	account_tbl->retrieval_mode                                = account->retrieval_mode;
	account_tbl->incoming_server_port_number                   = account->incoming_server_port_number;
	account_tbl->incoming_server_secure_connection             = account->incoming_server_secure_connection;
	account_tbl->outgoing_server_type                          = account->outgoing_server_type;
	account_tbl->outgoing_server_address                       = EM_SAFE_STRDUP(account->outgoing_server_address);
	account_tbl->outgoing_server_port_number                   = account->outgoing_server_port_number;
	account_tbl->outgoing_server_need_authentication           = account->outgoing_server_need_authentication;
	account_tbl->outgoing_server_secure_connection             = account->outgoing_server_secure_connection;
	account_tbl->outgoing_server_user_name                     = EM_SAFE_STRDUP(account->outgoing_server_user_name);
	account_tbl->outgoing_server_password                      = EM_SAFE_STRDUP(account->outgoing_server_password);
	account_tbl->user_display_name                             = EM_SAFE_STRDUP(account->user_display_name);
	account_tbl->reply_to_address                              = EM_SAFE_STRDUP(account->reply_to_address);
	account_tbl->return_address                                = EM_SAFE_STRDUP(account->return_address);
	account_tbl->account_id                                    = account->account_id;
	account_tbl->keep_mails_on_pop_server_after_download       = account->keep_mails_on_pop_server_after_download;
	account_tbl->auto_download_size                            = account->auto_download_size;
	account_tbl->peak_interval                                 = account->peak_interval;
	account_tbl->peak_days                                     = account->peak_days;
	account_tbl->peak_start_time                               = account->peak_start_time;
	account_tbl->peak_end_time                                 = account->peak_end_time;


	account_tbl->outgoing_server_use_same_authenticator        = account->outgoing_server_use_same_authenticator;
	account_tbl->auto_resend_times                             = account->auto_resend_times;
	account_tbl->pop_before_smtp                               = account->pop_before_smtp;
	account_tbl->incoming_server_requires_apop                 = account->incoming_server_requires_apop;
	account_tbl->incoming_server_authentication_method         = account->incoming_server_authentication_method;
	account_tbl->logo_icon_path                                = EM_SAFE_STRDUP(account->logo_icon_path);

	account_tbl->roaming_option                                = account->roaming_option;
	account_tbl->color_label                                   = account->color_label;
	account_tbl->user_data                                     = em_memdup(account->user_data, account->user_data_length);
	account_tbl->user_data_length                              = account->user_data_length;

	account_tbl->options.priority                              = account->options.priority;
	account_tbl->options.keep_local_copy                       = account->options.keep_local_copy;
	account_tbl->options.req_delivery_receipt                  = account->options.req_delivery_receipt;
	account_tbl->options.req_read_receipt                      = account->options.req_read_receipt;
	account_tbl->options.download_limit                        = account->options.download_limit;
	account_tbl->options.block_address                         = account->options.block_address;
	account_tbl->options.block_subject                         = account->options.block_subject;
	account_tbl->options.display_name_from                     = EM_SAFE_STRDUP(account->options.display_name_from);
	account_tbl->options.reply_with_body                       = account->options.reply_with_body;
	account_tbl->options.forward_with_files                    = account->options.forward_with_files;
	account_tbl->options.add_myname_card                       = account->options.add_myname_card;
	account_tbl->options.add_signature                         = account->options.add_signature;
	account_tbl->options.signature                             = EM_SAFE_STRDUP(account->options.signature);
	account_tbl->options.add_my_address_to_bcc                 = account->options.add_my_address_to_bcc;
	account_tbl->options.notification_status                   = account->options.notification_status;
	account_tbl->options.vibrate_status                        = account->options.vibrate_status;
	account_tbl->options.display_content_status                = account->options.display_content_status;
	account_tbl->options.default_ringtone_status               = account->options.default_ringtone_status;
	account_tbl->options.alert_ringtone_path                   = EM_SAFE_STRDUP(account->options.alert_ringtone_path);

	account_tbl->check_interval                                = account->check_interval;
	account_tbl->account_svc_id                                = account->account_svc_id;
	account_tbl->sync_status                                   = account->sync_status;
	account_tbl->sync_disabled                                 = account->sync_disabled;
	account_tbl->default_mail_slot_size                        = account->default_mail_slot_size;
	account_tbl->smime_type                                    = account->smime_type;
	account_tbl->certificate_path                              = EM_SAFE_STRDUP(account->certificate_path);
	account_tbl->cipher_type                                   = account->cipher_type;
	account_tbl->digest_type                                   = account->digest_type;
	account_tbl->outgoing_server_size_limit                    = account->outgoing_server_size_limit;
	account_tbl->wifi_auto_download                            = account->wifi_auto_download;

    account_tbl->user_name                                     = EM_SAFE_STRDUP(account->user_name);
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int em_convert_account_tbl_to_account(emstorage_account_tbl_t *account_tbl, email_account_t *account)
{
	EM_DEBUG_FUNC_BEGIN("account_tbl[%p], account[%p]", account_tbl, account);
	int ret = 1;

	account->account_name                                      = EM_SAFE_STRDUP(account_tbl->account_name);
	account->incoming_server_type                              = account_tbl->incoming_server_type;
	account->incoming_server_address                           = EM_SAFE_STRDUP(account_tbl->incoming_server_address);
	account->user_email_address                                = EM_SAFE_STRDUP(account_tbl->user_email_address);
	account->incoming_server_user_name                         = EM_SAFE_STRDUP(account_tbl->incoming_server_user_name);
	account->incoming_server_password                          = EM_SAFE_STRDUP(account_tbl->incoming_server_password);
	account->retrieval_mode                                    = account_tbl->retrieval_mode;
	account->incoming_server_port_number                       = account_tbl->incoming_server_port_number;
	account->incoming_server_secure_connection                 = account_tbl->incoming_server_secure_connection;
	account->outgoing_server_type                              = account_tbl->outgoing_server_type;
	account->outgoing_server_address                           = EM_SAFE_STRDUP(account_tbl->outgoing_server_address);
	account->outgoing_server_port_number                       = account_tbl->outgoing_server_port_number;
	account->outgoing_server_need_authentication               = account_tbl->outgoing_server_need_authentication;
	account->outgoing_server_secure_connection                 = account_tbl->outgoing_server_secure_connection;
	account->outgoing_server_user_name                         = EM_SAFE_STRDUP(account_tbl->outgoing_server_user_name);
	account->outgoing_server_password                          = EM_SAFE_STRDUP(account_tbl->outgoing_server_password);
	account->user_display_name                                 = EM_SAFE_STRDUP(account_tbl->user_display_name);
	account->reply_to_address                                  = EM_SAFE_STRDUP(account_tbl->reply_to_address);
	account->return_address                                    = EM_SAFE_STRDUP(account_tbl->return_address);
	account->account_id                                        = account_tbl->account_id;
	account->keep_mails_on_pop_server_after_download           = account_tbl->keep_mails_on_pop_server_after_download;
	account->auto_download_size                                = account_tbl->auto_download_size;
	account->peak_interval                                     = account_tbl->peak_interval;
	account->peak_days                                         = account_tbl->peak_days;
	account->peak_start_time                                   = account_tbl->peak_start_time;
	account->peak_end_time                                     = account_tbl->peak_end_time;
	account->outgoing_server_use_same_authenticator            = account_tbl->outgoing_server_use_same_authenticator;
	account->pop_before_smtp                                   = account_tbl->pop_before_smtp;
	account->auto_resend_times                                 = account_tbl->auto_resend_times;
	account->incoming_server_requires_apop                     = account_tbl->incoming_server_requires_apop;
	account->incoming_server_authentication_method             = account_tbl->incoming_server_authentication_method;
	account->logo_icon_path                                    = EM_SAFE_STRDUP(account_tbl->logo_icon_path);
	account->user_data                                         = em_memdup(account_tbl->user_data, account_tbl->user_data_length);
	account->user_data_length                                  = account_tbl->user_data_length;
	account->roaming_option                                    = account_tbl->roaming_option;
	account->color_label                                       = account_tbl->color_label;
	account->options.priority                                  = account_tbl->options.priority;
	account->options.keep_local_copy                           = account_tbl->options.keep_local_copy;
	account->options.req_delivery_receipt                      = account_tbl->options.req_delivery_receipt;
	account->options.req_read_receipt                          = account_tbl->options.req_read_receipt;
	account->options.download_limit                            = account_tbl->options.download_limit;
	account->options.block_address                             = account_tbl->options.block_address;
	account->options.block_subject                             = account_tbl->options.block_subject;
	account->options.display_name_from                         = EM_SAFE_STRDUP(account_tbl->options.display_name_from);
	account->options.reply_with_body                           = account_tbl->options.reply_with_body;
	account->options.forward_with_files                        = account_tbl->options.forward_with_files;
	account->options.add_myname_card                           = account_tbl->options.add_myname_card;
	account->options.add_signature                             = account_tbl->options.add_signature;
	account->options.signature                                 = EM_SAFE_STRDUP(account_tbl->options.signature);
	account->options.add_my_address_to_bcc                     = account_tbl->options.add_my_address_to_bcc;
	account->options.notification_status                       = account_tbl->options.notification_status;
	account->options.vibrate_status                            = account_tbl->options.vibrate_status;
	account->options.display_content_status                    = account_tbl->options.display_content_status;
	account->options.default_ringtone_status                   = account_tbl->options.default_ringtone_status;
	account->options.alert_ringtone_path                       = EM_SAFE_STRDUP(account_tbl->options.alert_ringtone_path);
	account->check_interval                                    = account_tbl->check_interval;
	account->account_svc_id                                    = account_tbl->account_svc_id;
	account->sync_status                                       = account_tbl->sync_status;
	account->sync_disabled                                     = account_tbl->sync_disabled;
	account->default_mail_slot_size                            = account_tbl->default_mail_slot_size;
	account->smime_type                                        = account_tbl->smime_type;
	account->certificate_path                                  = EM_SAFE_STRDUP(account_tbl->certificate_path);
	account->cipher_type                                       = account_tbl->cipher_type;
	account->digest_type                                       = account_tbl->digest_type;
	account->outgoing_server_size_limit                        = account_tbl->outgoing_server_size_limit;
	account->wifi_auto_download                                = account_tbl->wifi_auto_download;
	account->user_name                                         = EM_SAFE_STRDUP(account_tbl->user_name);

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
	mailbox_tbl->no_select                  = mailbox->no_select;
	mailbox_tbl->last_sync_time             = mailbox->last_sync_time;
	mailbox_tbl->deleted_flag               = mailbox->deleted_flag;
	mailbox_tbl->eas_data_length            = mailbox->eas_data_length;

	if ((mailbox->eas_data_length>0) && mailbox->eas_data) {
		if ((mailbox_tbl->eas_data = em_malloc(mailbox->eas_data_length)) == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			return 0; /*prevent 53448*/
		}
		memcpy(mailbox_tbl->eas_data, mailbox->eas_data, mailbox->eas_data_length);
	}

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
	mailbox->no_select                  = mailbox_tbl->no_select;
	mailbox->last_sync_time             = mailbox_tbl->last_sync_time;
	mailbox->deleted_flag               = mailbox_tbl->deleted_flag;
	mailbox->eas_data_length            = mailbox_tbl->eas_data_length;

	if ((mailbox_tbl->eas_data_length >0) && mailbox_tbl->eas_data) {
		if ((mailbox->eas_data = em_malloc(mailbox_tbl->eas_data_length)) == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			return 0; /*prevent 53445*/
		}
		memcpy(mailbox->eas_data, mailbox_tbl->eas_data, mailbox_tbl->eas_data_length);
	}

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
		temp_mail_data[i].scheduled_sending_time  = mail_table_data[i].scheduled_sending_time;
		temp_mail_data[i].remaining_resend_times  = mail_table_data[i].remaining_resend_times;
		temp_mail_data[i].tag_id                  = mail_table_data[i].tag_id;
		temp_mail_data[i].replied_time            = mail_table_data[i].replied_time;
		temp_mail_data[i].forwarded_time          = mail_table_data[i].forwarded_time;
		temp_mail_data[i].eas_data_length         = mail_table_data[i].eas_data_length;
		if(mail_table_data[i].eas_data_length && mail_table_data[i].eas_data) {
			if((temp_mail_data[i].eas_data = em_malloc(mail_table_data[i].eas_data_length)) == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				err_code = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
		}
		memcpy(temp_mail_data[i].eas_data, mail_table_data[i].eas_data, mail_table_data[i].eas_data_length);
		temp_mail_data[i].pgp_password            = EM_SAFE_STRDUP(mail_table_data[i].pgp_password);
		temp_mail_data[i].user_name               = EM_SAFE_STRDUP(mail_table_data[i].user_name);
	}

	*mail_data = temp_mail_data;

	ret = true;
FINISH_OFF:

	if(ret == false && temp_mail_data) {
		emcore_free_mail_data_list(&temp_mail_data, item_count);
	}

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
		temp_mail_tbl[i].scheduled_sending_time  = mail_data[i].scheduled_sending_time;
		temp_mail_tbl[i].remaining_resend_times  = mail_data[i].remaining_resend_times;
		temp_mail_tbl[i].tag_id                  = mail_data[i].tag_id;
		temp_mail_tbl[i].replied_time            = mail_data[i].replied_time;
		temp_mail_tbl[i].forwarded_time          = mail_data[i].forwarded_time;
		temp_mail_tbl[i].eas_data_length         = mail_data[i].eas_data_length;
		if(mail_data[i].eas_data_length && mail_data[i].eas_data) {
			if((temp_mail_tbl[i].eas_data = em_malloc(mail_data[i].eas_data_length)) == NULL) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				err_code = EMAIL_ERROR_OUT_OF_MEMORY;
				/*prevent 44357*/
				if (i > 0)
					emstorage_free_mail(&temp_mail_tbl, i, NULL);
				else
					EM_SAFE_FREE(temp_mail_tbl);
				goto FINISH_OFF;
			}
		}
		memcpy(temp_mail_tbl[i].eas_data, mail_data[i].eas_data, mail_data[i].eas_data_length);
		temp_mail_tbl[i].pgp_password             = EM_SAFE_STRDUP(mail_data[i].pgp_password);
		temp_mail_tbl[i].user_name                = EM_SAFE_STRDUP(mail_data[i].user_name);
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

/* divide struct at binary field (void* user_data)*/
#define EMAIL_ACCOUNT_FMT	"S(" "isiii" "isiis" "sssis" "issii" "iiiii" "iiiis" "issii" "i"\
							"$(" "iiiii" "iisii" "iisii" "iiis" ")" "iiiii" "isiis" "i" ")" "B"

/* For converting fmt : to distinguish between 64bit or 32bit */
static char *convert_format(char *fmt)
{
	EM_DEBUG_FUNC_BEGIN();

	if (fmt == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return NULL;
	}

	int size = 0;
	int string_size = 0;
	int ret = false;
	int nested_structure = 0;
	char *c = NULL;
	char *converted_fmt = NULL;

	size = sizeof(time_t);

	string_size = strlen(fmt);
	if (size == 4) {
		converted_fmt = em_malloc(string_size + 1);
	} else if (size == 8) {
		/* 64bit nested */
		converted_fmt = em_malloc(string_size + 1 + 1);
	}

	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		return NULL;
	}

	c = fmt;
    while (*c != '\0') {
        switch (*c) {
            case 'c':
            case 'i':
            case 'u':
            case 'j':
            case 'v':
            case 'I':
            case 'U':
            case 'f':
            case 's':
            case '#':
            case 'B':
            case 'A':
            case 'S':
				strncat(converted_fmt, c, 1);
				break;
			case 't': /* special charater for time_t */
			case 'T':
				if (size == 4) {
					/* 32bit */
					strncat(converted_fmt, "i", 1);
				} else if (size == 8) {
					/* 64bit */
					strncat(converted_fmt, "I", 1);
				} else {
					EM_DEBUG_LOG("size : [%d]", size);
				}

				break;
            case '$': /* nested structure */
				/* if 64 bit inserted the dump integer type */
				strncat(converted_fmt, c, 1);
				nested_structure = 1;
				break;
            case ')':
				strncat(converted_fmt, c, 1);
				if (nested_structure)
					nested_structure = 0;
				break;
            case '(':
				strncat(converted_fmt, c, 1);
				if (nested_structure && size == 8)
					strncat(converted_fmt, "i", 1);
				break;
            default:
                EM_DEBUG_EXCEPTION("unsupported option %c\n", *c);
				goto FINISH_OFF;
        }
        c++;
    }

	EM_DEBUG_LOG("original fmt  : [%s]", fmt);
	EM_DEBUG_LOG("converted_fmt : [%s]", converted_fmt);

	ret = true;

FINISH_OFF:

	if (ret == false) {
		EM_SAFE_FREE(converted_fmt);
	}

	EM_DEBUG_FUNC_END();
	return converted_fmt;
}

INTERNAL_FUNC char* em_convert_account_to_byte_stream(email_account_t* account, int *stream_len)
{
	EM_DEBUG_FUNC_END();
	EM_IF_NULL_RETURN_VALUE(account, NULL);

	tpl_node *tn = NULL;
	tpl_bin tb;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_ACCOUNT_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return NULL;
	}

	tn = tpl_map(converted_fmt, account, &tb, &(account->user_data_length));
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return NULL;
	}
	EM_SAFE_FREE(converted_fmt);

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
}

INTERNAL_FUNC void em_convert_byte_stream_to_account(char *stream, int stream_len, email_account_t *account)
{
	EM_DEBUG_FUNC_END();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(account);

	tpl_node *tn = NULL;
	tpl_bin tb;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_ACCOUNT_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return;
	}

	tn = tpl_map(converted_fmt, account, &tb, &(account->user_data_length));
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return;
	}
	EM_SAFE_FREE(converted_fmt);

	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);
	tpl_free(tn);

	/* tb will be destroyed at end of func, but tb.addr remains */
	account->user_data = tb.addr;

	EM_DEBUG_FUNC_END();
}

#define EMAIL_MAIL_DATA_FMT  "S(" "iiiis" "tisss" "issss" "sssss" "sisss"\
                            "icccc" "cccii" "iiiii" "iisii" "iitii" "ttsss" "si" ")B"

INTERNAL_FUNC char* em_convert_mail_data_to_byte_stream(email_mail_data_t *mail_data, int *stream_len)
{
	EM_DEBUG_FUNC_BEGIN("mail_data[%p] stream_len[%p]", mail_data, stream_len);
	EM_IF_NULL_RETURN_VALUE(mail_data, NULL);
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);

	tpl_node *tn = NULL;
	tpl_bin tb;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_MAIL_DATA_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return NULL;
	}

	EM_DEBUG_LOG("eas_data_length[%d]", mail_data->eas_data_length); /*prevent 44369*/

	tn = tpl_map(converted_fmt, mail_data, &tb);
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return NULL;
	}
	EM_SAFE_FREE(converted_fmt);
	tb.sz   = mail_data->eas_data_length;
	tb.addr = mail_data->eas_data;
	tpl_pack(tn, 0);

	/* write account to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;

	EM_DEBUG_FUNC_END();
	return (char*) buf;
}

INTERNAL_FUNC void em_convert_byte_stream_to_mail_data(char *stream, int stream_len, email_mail_data_t *mail_data)
{
	EM_DEBUG_FUNC_BEGIN("stream[%p] stream_len[%d] mail_data[%p]", stream, stream_len, mail_data);
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(mail_data);

	tpl_node *tn = NULL;
	tpl_bin tb;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_MAIL_DATA_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return;
	}

	tn = tpl_map(converted_fmt, mail_data, &tb);
	if(!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return;
	}
	EM_SAFE_FREE(converted_fmt);

	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);

	if(mail_data->eas_data_length <= 0 || tb.addr == NULL) {
		EM_DEBUG_LOG("No eas data. eas_data_length[%d] addr[%p]", mail_data->eas_data_length, tb.addr);
	}
	else {
		EM_DEBUG_LOG("eas_data_length[%d] addr[%p]", mail_data->eas_data_length, tb.addr);
		mail_data->eas_data = tb.addr;
	}

	if(tn)
		tpl_free(tn);

	EM_DEBUG_FUNC_END();
}


#define EMAIL_ATTACHMENT_DATA_FMT "A(S(" "isssi" "iicii" "is" "))"

INTERNAL_FUNC char* em_convert_attachment_data_to_byte_stream(email_attachment_data_t *attachment, int attachment_count, int* stream_len)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);

	email_attachment_data_t cur = {0};
	tpl_node *tn = NULL;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_ATTACHMENT_DATA_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("convertinf failed");
		return NULL;
	}

	/* tpl_map adds value at 2nd param addr to packing buffer iterately */
	/* 2nd param value (not addr via pointer) should be modified at each iteration */
	tn = tpl_map(converted_fmt, &cur);
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return NULL;
	}
	EM_SAFE_FREE(converted_fmt);

	/* if attachment_count is zero, for loop is skipped */
	int i=0;
	for( ; (i < attachment_count) && (attachment+i) ; i++ ) {
		memcpy(&cur, attachment + i, sizeof(email_attachment_data_t));
		tpl_pack(tn, 1);
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
}

INTERNAL_FUNC void em_convert_byte_stream_to_attachment_data(char *stream, int stream_len, email_attachment_data_t **attachment_data, int *attachment_count)
{
	EM_DEBUG_FUNC_BEGIN();

	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(attachment_data);
	EM_NULL_CHECK_FOR_VOID(attachment_count);

	email_attachment_data_t cur = {0};
	tpl_node *tn = NULL;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_ATTACHMENT_DATA_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return;
	}

	tn = tpl_map(converted_fmt, &cur);
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return;
	}
	EM_SAFE_FREE(converted_fmt);
	tpl_load(tn, TPL_MEM, stream, stream_len);

	/* tpl does not return the size of variable-length array, but we need variable-length array */
	/* so, make list and get list count in the first phase, */
	/* and then copy list to var array after allocating memory */

	int num_element = tpl_Alen (tn, 1);
	email_attachment_data_t *attached = NULL;
	if (num_element > 0)
		attached = (email_attachment_data_t*) em_malloc(sizeof(email_attachment_data_t)*num_element);

	int i = 0;
	while (tpl_unpack(tn, 1) > 0) {
		if (!(attached+i)) {
			EM_DEBUG_EXCEPTION ("num element mismatched [%d] vs [%d]", num_element, i);
			num_element = i;
			break;
		}
		email_attachment_data_t* pdata = attached+i;
		memcpy(pdata, &cur, sizeof(email_attachment_data_t)); /* copy unpacked data to list item */
		pdata->attachment_name 		= EM_SAFE_STRDUP(cur.attachment_name);
		pdata->attachment_path 		= EM_SAFE_STRDUP(cur.attachment_path);
		pdata->content_id               = EM_SAFE_STRDUP(cur.content_id);
		pdata->attachment_mime_type 	= EM_SAFE_STRDUP(cur.attachment_mime_type);
		EM_SAFE_FREE(cur.attachment_name);
		EM_SAFE_FREE(cur.attachment_path);
		EM_SAFE_FREE(cur.content_id);
		EM_SAFE_FREE(cur.attachment_mime_type);
		memset(&cur, 0, sizeof(email_attachment_data_t));    /* initialize variable, used for unpacking */
		i++;
	}
	tpl_free(tn);

	*attachment_count = num_element;
	*attachment_data = attached;

	EM_DEBUG_FUNC_END();
}


#define EMAIL_MAILBOX_FMT  "S(" "isisi" "iiiii" "itii" ")B"

INTERNAL_FUNC char* em_convert_mailbox_to_byte_stream(email_mailbox_t *mailbox_data, int *stream_len)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_IF_NULL_RETURN_VALUE(mailbox_data, NULL);
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);

	tpl_node *tn = NULL;
	tpl_bin tb;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_MAILBOX_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return NULL;
	}

	EM_DEBUG_LOG("eas_data_length[%d]", mailbox_data->eas_data_length);

	tn = tpl_map(converted_fmt, mailbox_data, &tb);
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return NULL;
	}
	EM_SAFE_FREE(converted_fmt);
	tb.sz   = mailbox_data->eas_data_length;
	tb.addr = mailbox_data->eas_data;
	tpl_pack(tn, 0);

	/* write account to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);

	if(tn)
		tpl_free(tn);

	*stream_len = len;

	EM_DEBUG_FUNC_END("serialized len: %d", len);
	return (char*) buf;
}


INTERNAL_FUNC void em_convert_byte_stream_to_mailbox(char *stream, int stream_len, email_mailbox_t *mailbox_data)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(mailbox_data);

	tpl_node *tn = NULL;
	tpl_bin tb;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_MAILBOX_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return;
	}

	tn = tpl_map(converted_fmt, mailbox_data, &tb);
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return;
	}
	EM_SAFE_FREE(converted_fmt);
	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);

	if(mailbox_data->eas_data_length <= 0 || tb.addr == NULL) {
		EM_DEBUG_LOG("No eas data. eas_data_length[%d] addr[%p]", mailbox_data->eas_data_length, tb.addr);
	}
	else {
		EM_DEBUG_LOG("eas_data_length[%d] addr[%p]", mailbox_data->eas_data_length, tb.addr);
		mailbox_data->eas_data = tb.addr;
	}

	if(tn)
		tpl_free(tn);

	EM_DEBUG_FUNC_END("deserialized len %d", stream_len);
}

#define EMAIL_OPTION_FMT "S(" "iiiii" "iisii" "iisi" ")"

INTERNAL_FUNC char* em_convert_option_to_byte_stream(email_option_t* option, int* stream_len)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_IF_NULL_RETURN_VALUE(option, NULL);
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);

	tpl_node *tn = NULL;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_OPTION_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return NULL;
	}

	tn = tpl_map(converted_fmt, option);
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return NULL;
	}
	EM_SAFE_FREE(converted_fmt);
	tpl_pack(tn, 0);

	/* write account to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;

	EM_DEBUG_FUNC_END("serialized len: %d", len);
	return (char*) buf;
}

INTERNAL_FUNC void em_convert_byte_stream_to_option(char *stream, int stream_len, email_option_t *option)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(option);

	tpl_node *tn = NULL;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_OPTION_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return;
	}

	tn = tpl_map(converted_fmt, option);
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return;
	}
	EM_SAFE_FREE(converted_fmt);
	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);
	tpl_free(tn);

	EM_DEBUG_FUNC_END("deserialized len %d", stream_len);
}


#define EMAIL_RULE_FMT "S(" "iisis" "siiii" ")"

INTERNAL_FUNC char* em_convert_rule_to_byte_stream(email_rule_t *rule, int *stream_len)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_IF_NULL_RETURN_VALUE(rule, NULL);
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);

	tpl_node *tn = NULL;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_RULE_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return NULL;
	}

	tn = tpl_map(converted_fmt, rule);
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return NULL;
	}
	EM_SAFE_FREE(converted_fmt);
	tpl_pack(tn, 0);

	/* write account to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;

	EM_DEBUG_FUNC_END("serialized len: %d", len);
	return (char*) buf;
}

INTERNAL_FUNC void em_convert_byte_stream_to_rule(char *stream, int stream_len, email_rule_t *rule)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(rule);

	tpl_node *tn = NULL;
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_RULE_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return;
	}

	tn = tpl_map(converted_fmt, rule);
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return;
	}
	EM_SAFE_FREE(converted_fmt);
	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);
	tpl_free(tn);

	EM_DEBUG_FUNC_END("deserialized len %d", stream_len);
}

#define EMAIL_MEETING_REQUEST_FMT   "iiBBs" "sic#Bi" "c#Bi"

INTERNAL_FUNC char* em_convert_meeting_req_to_byte_stream(email_meeting_request_t *meeting_req, int *stream_len)
{

	EM_DEBUG_FUNC_END();
	EM_IF_NULL_RETURN_VALUE(meeting_req, NULL);

	tpl_node *tn = NULL;
	tpl_bin tb[4];
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_MEETING_REQUEST_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return NULL;
	}

	tn = tpl_map(converted_fmt,
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
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return NULL;
	}
	EM_SAFE_FREE(converted_fmt);
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
}


INTERNAL_FUNC void em_convert_byte_stream_to_meeting_req(char *stream, int stream_len, email_meeting_request_t *meeting_req)
{
	EM_DEBUG_FUNC_END();
	EM_NULL_CHECK_FOR_VOID(stream);
	EM_NULL_CHECK_FOR_VOID(meeting_req);

	tpl_node *tn = NULL;
	tpl_bin tb[4];
	char *converted_fmt = NULL;

	converted_fmt = convert_format(EMAIL_MEETING_REQUEST_FMT);
	if (converted_fmt == NULL) {
		EM_DEBUG_EXCEPTION("converting failed");
		return;
	}

	tn = tpl_map(converted_fmt,
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
	if (!tn) {
		EM_SAFE_FREE(converted_fmt);
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return;
	}
	tpl_load(tn, TPL_MEM, stream, stream_len);
	tpl_unpack(tn, 0);
	tpl_free(tn);
	EM_SAFE_FREE(converted_fmt);

	/* tb will be destroyed at end of func, but tb.addr remains */
	memcpy(&meeting_req->start_time, tb[0].addr, sizeof(struct tm));
	memcpy(&meeting_req->end_time, tb[1].addr, sizeof(struct tm));
	memcpy(&meeting_req->time_zone.standard_time_start_date, tb[2].addr, sizeof(struct tm));
	memcpy(&meeting_req->time_zone.daylight_time_start_date, tb[3].addr, sizeof(struct tm));

	int i=0;
	for(i=0; i< 4 ; i++)
		EM_SAFE_FREE(tb[i].addr);

	EM_DEBUG_FUNC_END();
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
		result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_search_filter_list[i].search_filter_type), sizeof(int));
		switch(input_search_filter_list[i].search_filter_type) {
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       :
			case EMAIL_SEARCH_FILTER_TYPE_UID              :
			case EMAIL_SEARCH_FILTER_TYPE_ALL              :
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      :
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_NEW        :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_OLD        :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       :
			case EMAIL_SEARCH_FILTER_TYPE_HEADER_PRIORITY  :
				result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_search_filter_list[i].search_filter_key_value.integer_type_key_value), sizeof(int));
				break;

			case EMAIL_SEARCH_FILTER_TYPE_BCC              :
			case EMAIL_SEARCH_FILTER_TYPE_BODY             :
			case EMAIL_SEARCH_FILTER_TYPE_CC               :
			case EMAIL_SEARCH_FILTER_TYPE_FROM             :
			case EMAIL_SEARCH_FILTER_TYPE_KEYWORD          :
			case EMAIL_SEARCH_FILTER_TYPE_TEXT             :
			case EMAIL_SEARCH_FILTER_TYPE_SUBJECT          :
			case EMAIL_SEARCH_FILTER_TYPE_TO               :
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       :
			case EMAIL_SEARCH_FILTER_TYPE_ATTACHMENT_NAME  :
			case EMAIL_SEARCH_FILTER_TYPE_CHARSET          :
			case EMAIL_SEARCH_FILTER_TYPE_USER_DEFINED     :
				result_stream = append_string_to_stream(result_stream, &stream_size, input_search_filter_list[i].search_filter_key_value.string_type_key_value);
				break;

			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE :
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     :
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  :
				result_stream = append_sized_data_to_stream(result_stream, &stream_size, (char*)&(input_search_filter_list[i].search_filter_key_value.time_type_key_value), sizeof(time_t));
				break;
			default :
				EM_DEBUG_EXCEPTION("Invalid filter type [%d]", input_search_filter_list[i].search_filter_type);
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
			case EMAIL_SEARCH_FILTER_TYPE_ALL              :
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      :
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_NEW        :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_OLD        :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     :
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       :
			case EMAIL_SEARCH_FILTER_TYPE_HEADER_PRIORITY  :
				fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(int), (char*)&(local_search_filter[i].search_filter_key_value.integer_type_key_value));
				break;

			case EMAIL_SEARCH_FILTER_TYPE_BCC              :
			case EMAIL_SEARCH_FILTER_TYPE_BODY             :
			case EMAIL_SEARCH_FILTER_TYPE_CC               :
			case EMAIL_SEARCH_FILTER_TYPE_FROM             :
			case EMAIL_SEARCH_FILTER_TYPE_KEYWORD          :
			case EMAIL_SEARCH_FILTER_TYPE_TEXT             :
			case EMAIL_SEARCH_FILTER_TYPE_SUBJECT          :
			case EMAIL_SEARCH_FILTER_TYPE_TO               :
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       :
			case EMAIL_SEARCH_FILTER_TYPE_ATTACHMENT_NAME  :
			case EMAIL_SEARCH_FILTER_TYPE_CHARSET          :
			case EMAIL_SEARCH_FILTER_TYPE_USER_DEFINED     :
				fetch_string_from_stream(input_stream, &stream_offset, &(local_search_filter[i].search_filter_key_value.string_type_key_value));
				break;

			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE :
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     :
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  :
				fetch_sized_data_from_stream(input_stream, &stream_offset, sizeof(time_t), (char*)&(local_search_filter[i].search_filter_key_value.time_type_key_value));
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


#define EMAIL_JOB_INFORMATION_FMT   "A(S(iiii))"

INTERNAL_FUNC char* em_convert_task_information_to_byte_stream(email_task_information_t *input_task_information, int input_task_information_count, int *stream_len)
{
	EM_DEBUG_FUNC_BEGIN("input_task_information[%p] input_task_information_count[%d] stream_len[%p]", input_task_information, input_task_information_count, stream_len);
	EM_IF_NULL_RETURN_VALUE(input_task_information, NULL);
	EM_IF_NULL_RETURN_VALUE(stream_len, NULL);

	email_task_information_t cur = {0};
	tpl_node *tn = NULL;
	int i = 0;

	tn = tpl_map(EMAIL_JOB_INFORMATION_FMT, &cur);
	if (!tn) {
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return NULL;
	}

	for( ; i < input_task_information_count ; i++ ) {
		memcpy(&cur, input_task_information + i, sizeof(email_task_information_t));
		tpl_pack(tn, 1);
	}

	/* write data to buffer */
	void *buf = NULL;
	size_t len = 0;
	tpl_dump(tn, TPL_MEM, &buf, &len);
	tpl_free(tn);

	*stream_len = len;

	EM_DEBUG_FUNC_END("serialized len: %d", len);
	return (char*) buf;
}

INTERNAL_FUNC void em_convert_byte_stream_to_task_information(char *input_stream, int input_stream_len, email_task_information_t **output_task_information, int *output_task_information_count)
{
	EM_DEBUG_FUNC_BEGIN("input_stream[%p] input_stream_len[%d] output_task_information[%p] output_task_information_count[%p]", input_stream, input_stream_len, output_task_information, output_task_information_count);
	EM_NULL_CHECK_FOR_VOID(input_stream);
	EM_NULL_CHECK_FOR_VOID(output_task_information);

	email_task_information_t cur = {0};
	tpl_node *tn = NULL;
	int i = 0;

	tn = tpl_map(EMAIL_JOB_INFORMATION_FMT, &cur);
	if (!tn) {
		EM_DEBUG_EXCEPTION("tpl_map failed");
		return;
	}
	tpl_load(tn, TPL_MEM, input_stream, input_stream_len);
	int num_element = tpl_Alen (tn, 1);
	email_task_information_t* deserialized = NULL;
	if (num_element > 0)
		deserialized = (email_task_information_t*) em_malloc(sizeof(email_task_information_t)*num_element);

	while( tpl_unpack(tn, 1) > 0) {
		if (!(deserialized+i)) {
			EM_DEBUG_EXCEPTION ("num element mismatched [%d] vs [%d]", num_element, i);
			num_element = i;
			break;
		}
		email_task_information_t* pdata = deserialized+i;
		memcpy(pdata, &cur, sizeof(email_task_information_t));
		memset(&cur, 0, sizeof(email_task_information_t));
		i++;
	}
	tpl_free(tn);


	*output_task_information_count = num_element;
	*output_task_information       = deserialized;

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int em_convert_certificate_tbl_to_certificate(emstorage_certificate_tbl_t *certificate_tbl, email_certificate_t **certificate, int *error)
{
	EM_DEBUG_FUNC_BEGIN("certficate_tbl[%p], certificate[%p]", certificate_tbl, certificate);

	int err_code = EMAIL_ERROR_NONE;

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

