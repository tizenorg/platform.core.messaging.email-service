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



/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with Email Engine.
 * @file		Emf_Mapi_Types.c
 * @author	Kyuho Jo ( kyuho.jo@samsung.com )
 * @version	0.1
 * @brief 	This file contains interfaces for using data structures of email-service . 
 */

#include <Emf_Mapi.h>
#include <emf-types.h>


EXPORT_API int email_get_info_from_mail(emf_mail_t *mail, emf_mail_info_t **info)
{
	if(!mail || !info)
		return EMF_ERROR_INVALID_PARAM;
	
	*info = mail->info;

	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_info_to_mail(emf_mail_t *mail, emf_mail_info_t *info)
{
	if(!mail || !info)
		return EMF_ERROR_INVALID_PARAM;
	
	mail->info = info;

	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_head_from_mail(emf_mail_t *mail, emf_mail_head_t **head)
{
	if(!mail || !head)
		return EMF_ERROR_INVALID_PARAM;
	
	*head = mail->head;

	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_head_to_mail(emf_mail_t *mail, emf_mail_head_t *head)
{
	if(!mail || !head)
		return EMF_ERROR_INVALID_PARAM;
	
	mail->head = head;

	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_body_from_mail(emf_mail_t *mail, emf_mail_body_t **body)
{
	if(!mail || !body)
		return EMF_ERROR_INVALID_PARAM;
	
	*body = mail->body;

	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_body_to_mail(emf_mail_t *mail, emf_mail_body_t *body)
{
	if(!mail || !body)
		return EMF_ERROR_INVALID_PARAM;
	
	mail->body = body;

	return EMF_ERROR_NONE;
}


/*  emf_mail_info_t ------------------------------------------------------------ */

EXPORT_API int email_get_account_id_from_info(emf_mail_info_t *info, int *account_id)
{
	if(!info || !account_id)
		return EMF_ERROR_INVALID_PARAM;
		
	*account_id = info->account_id;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_account_id_to_info(emf_mail_info_t *info, int account_id)
{
	if(!info)
		return EMF_ERROR_INVALID_PARAM;
		
	info->account_id = account_id;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_uid_from_info(emf_mail_info_t *info, int *uid)
{
	if(!info || !uid)
		return EMF_ERROR_INVALID_PARAM;

	*uid = info->uid;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_uid_to_info(emf_mail_info_t *info, int uid)
{
	if(!info)
		return EMF_ERROR_INVALID_PARAM;

	info->uid = uid;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_rfc822_size_from_info(emf_mail_info_t *info, int *rfc822_size)
{
	if(!info || !rfc822_size)
		return EMF_ERROR_INVALID_PARAM;

	*rfc822_size = info->rfc822_size;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_rfc822_size_to_info(emf_mail_info_t *info, int rfc822_size)
{
	if(!info)
		return EMF_ERROR_INVALID_PARAM;

	info->rfc822_size = rfc822_size;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_body_downloaded_from_info(emf_mail_info_t *info, int *body_downloaded)
{
	if(!info || !body_downloaded)
		return EMF_ERROR_INVALID_PARAM;

	*body_downloaded = info->body_downloaded;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_body_downloaded_to_info(emf_mail_info_t *info, int body_downloaded)
{
	if(!info)
		return EMF_ERROR_INVALID_PARAM;

	info->body_downloaded = body_downloaded;
	
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_flags_from_info(emf_mail_info_t *info, emf_mail_flag_t *flags)
{
	if(!info || !flags)
		return EMF_ERROR_INVALID_PARAM;

	*flags = info->flags;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_flags_to_info(emf_mail_info_t *info, emf_mail_flag_t flags)
{
	if(!info)
		return EMF_ERROR_INVALID_PARAM;

	info->flags = flags;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_extra_flags_from_info(emf_mail_info_t *info, emf_extra_flag_t *extra_flags)
{
	if(!info || !extra_flags)
		return EMF_ERROR_INVALID_PARAM;

	*extra_flags = info->extra_flags;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_extra_flags_to_info(emf_mail_info_t *info, emf_extra_flag_t extra_flags)
{
	if(!info)
		return EMF_ERROR_INVALID_PARAM;

	info->extra_flags = extra_flags;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_sid_from_info(emf_mail_info_t *info, char **sid)
{
	if(!info || !sid)
		return EMF_ERROR_INVALID_PARAM;

	*sid = info->sid;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_sid_to_info(emf_mail_info_t *info, char *sid)
{
	if(!info)
		return EMF_ERROR_INVALID_PARAM;

	info->sid = sid;		
		
	return EMF_ERROR_NONE;
}

/*  emf_mail_head_t ------------------------------------------------------------ */

EXPORT_API int email_get_mid_from_head(emf_mail_head_t *head, char **mid)
{
	if(!head || !mid)
		return EMF_ERROR_INVALID_PARAM;

	*mid = head->mid;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_mid_to_head(emf_mail_head_t *head, char *mid)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->mid = mid;	
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_subject_from_head(emf_mail_head_t *head, char **subject)
{
	if(!head || !subject)
		return EMF_ERROR_INVALID_PARAM;

	*subject = head->subject;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_subject_to_head(emf_mail_head_t *head, char *subject)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->subject = subject;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_to_from_head(emf_mail_head_t *head, char **to)
{
	if(!head || !to)
		return EMF_ERROR_INVALID_PARAM;

	*to = head->to;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_to_to_head(emf_mail_head_t *head, char *to)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->to = to;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_from_from_head(emf_mail_head_t *head, char **from)
{
	if(!head || !from)
		return EMF_ERROR_INVALID_PARAM;

	*from = head->from;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_from_to_head(emf_mail_head_t *head, char *from)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->from = from;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_cc_from_head(emf_mail_head_t *head, char **cc)
{
	if(!head || !cc)
		return EMF_ERROR_INVALID_PARAM;

	*cc = head->cc;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_cc_to_head(emf_mail_head_t *head, char *cc)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->cc = cc;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_bcc_from_head(emf_mail_head_t *head, char **bcc)
{
	if(!head || !bcc)
		return EMF_ERROR_INVALID_PARAM;

	*bcc = head->bcc;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_bcc_to_head(emf_mail_head_t *head, char *bcc)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->bcc = bcc;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_reply_from_head(emf_mail_head_t *head, char **reply)
{
	if(!head || !reply)
		return EMF_ERROR_INVALID_PARAM;

	*reply = head->reply_to;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_reply_to_head(emf_mail_head_t *head, char *reply)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->reply_to = reply;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_return_path_from_head(emf_mail_head_t *head, char **return_path)
{
	if(!head || !return_path)
		return EMF_ERROR_INVALID_PARAM;

	*return_path = head->return_path;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_return_path_to_head(emf_mail_head_t *head, char *return_path)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->return_path = return_path;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_datetime_from_head(emf_mail_head_t *head, emf_datetime_t *datetime)
{
	if(!head || !datetime)
		return EMF_ERROR_INVALID_PARAM;

	*datetime = head->datetime;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_datetime_to_head(emf_mail_head_t *head, emf_datetime_t datetime)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->datetime = datetime;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_from_contact_name_from_head(emf_mail_head_t *head, char **from_contact_name)
{
	if(!head || !from_contact_name)
		return EMF_ERROR_INVALID_PARAM;

	*from_contact_name = head->from_contact_name;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_from_contact_name_to_head(emf_mail_head_t *head, char *from_contact_name)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->from_contact_name = from_contact_name;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_to_contact_name_from_head(emf_mail_head_t *head, char **to_contact_name)
{
	if(!head || !to_contact_name)
		return EMF_ERROR_INVALID_PARAM;
		
	*to_contact_name = head->to_contact_name;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_to_contact_name_to_head(emf_mail_head_t *head, char *to_contact_name)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->to_contact_name = to_contact_name;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_cc_contact_name_from_head(emf_mail_head_t *head, char **cc_contact_name)
{
	if(!head || !cc_contact_name)
		return EMF_ERROR_INVALID_PARAM;

	*cc_contact_name = head->cc_contact_name;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_cc_contact_name_to_head(emf_mail_head_t *head, char *cc_contact_name)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->cc_contact_name = cc_contact_name;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_bcc_contact_name_from_head(emf_mail_head_t *head, char **bcc_contact_name)
{
	if(!head || !bcc_contact_name)
		return EMF_ERROR_INVALID_PARAM;

	*bcc_contact_name = head->bcc_contact_name;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_bcc_contact_name_to_head(emf_mail_head_t *head, char *bcc_contact_name)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->bcc_contact_name = bcc_contact_name;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_preview_body_text_from_head(emf_mail_head_t *head, char **preview_body_text)
{
	if(!head || !preview_body_text)
		return EMF_ERROR_INVALID_PARAM;

	*preview_body_text = head->previewBodyText;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_preview_body_text_to_head(emf_mail_head_t *head, char *preview_body_text)
{
	if(!head)
		return EMF_ERROR_INVALID_PARAM;

	head->previewBodyText = preview_body_text;
		
	return EMF_ERROR_NONE;
}

/*  emf_mail_body_t ------------------------------------------------------------ */

EXPORT_API int email_get_plain_from_body(emf_mail_body_t *body, char **plain)
{
	if(!body || !plain)
		return EMF_ERROR_INVALID_PARAM;

	*plain = body->plain;
		
	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_plain_to_body(emf_mail_body_t *body, char *plain)
{
	if(!body)
		return EMF_ERROR_INVALID_PARAM;

	body->plain = plain;

	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_plain_charset_from_body(emf_mail_body_t *body, char **plain_charset)
{
	if(!body || !plain_charset)
		return EMF_ERROR_INVALID_PARAM;

	*plain_charset = body->plain_charset;

	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_plain_charset_to_body(emf_mail_body_t *body, char *plain_charset)
{
	if(!body)
		return EMF_ERROR_INVALID_PARAM;

	body->plain_charset = plain_charset;	

	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_html_from_body(emf_mail_body_t *body, char **html)
{
	if(!body || !html)
		return EMF_ERROR_INVALID_PARAM;

	*html = body->html;

	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_html_to_body(emf_mail_body_t *body, char *html)
{
	if(!body)
		return EMF_ERROR_INVALID_PARAM;

	body->html = html;	

	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_attachment_num_from_body(emf_mail_body_t *body, int *attachment_num)
{
	if(!body || !attachment_num)
		return EMF_ERROR_INVALID_PARAM;

	*attachment_num = body->attachment_num;

	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_attachment_num_to_body(emf_mail_body_t *body, int attachment_num)
{
	if(!body)
		return EMF_ERROR_INVALID_PARAM;

	body->attachment_num = attachment_num;		

	return EMF_ERROR_NONE;
}

EXPORT_API int email_get_attachment_from_body(emf_mail_body_t *body, emf_attachment_info_t **attachment)
{
	if(!body || !attachment)
		return EMF_ERROR_INVALID_PARAM;

	*attachment = body->attachment;

	return EMF_ERROR_NONE;
}

EXPORT_API int email_set_attachment_to_body(emf_mail_body_t *body, emf_attachment_info_t *attachment)
{
	if(!body)
		return EMF_ERROR_INVALID_PARAM;

	body->attachment = attachment;	

	return EMF_ERROR_NONE;
}

/*  emf_account_t -------------------------------------------------------------- */

int email_get_account_bind_type_from_account(emf_account_t *account, emf_account_bind_t account_bind_type)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account_bind_type = account->account_bind_type;

	return EMF_ERROR_NONE;
}

int email_set_account_bind_type_to_account(emf_account_t *account, emf_account_bind_t account_bind_type)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->account_bind_type = account_bind_type;	

	return EMF_ERROR_NONE;
}

int email_get_account_name_from_account(emf_account_t *account, char **account_name)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*account_name = account->account_name;


	return EMF_ERROR_NONE;
}

int email_set_account_name_to_account(emf_account_t *account, char *account_name)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->account_name = account_name;

	return EMF_ERROR_NONE;
}

int email_get_receiving_server_type_from_account(emf_account_t *account, emf_account_server_t *receiving_server_type)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*receiving_server_type = account->receiving_server_type;

	return EMF_ERROR_NONE;
}

int email_set_receiving_server_type_to_account(emf_account_t *account, emf_account_server_t receiving_server_type)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->receiving_server_type = receiving_server_type;


	return EMF_ERROR_NONE;
}

int email_get_receiving_server_addr_from_account(emf_account_t *account, char **receiving_server_addr)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*receiving_server_addr = account->receiving_server_addr;

	return EMF_ERROR_NONE;
}

int email_set_receiving_server_addr_to_account(emf_account_t *account, char *receiving_server_addr)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->receiving_server_addr = receiving_server_addr;

	return EMF_ERROR_NONE;
}

int email_get_email_addr_from_account(emf_account_t *account, char **email_addr)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*email_addr = account->email_addr;

	return EMF_ERROR_NONE;
}

int email_set_email_addr_to_account(emf_account_t *account, char *email_addr)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->email_addr = email_addr;

	return EMF_ERROR_NONE;
}

int email_get_user_name_from_account(emf_account_t *account, char **user_name)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*user_name = account->user_name;

	return EMF_ERROR_NONE;
}

int email_set_user_name_to_account(emf_account_t *account, char *user_name)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->user_name = user_name;

	return EMF_ERROR_NONE;
}

int email_get_password_from_account(emf_account_t *account, char **password)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*password = account->password;

	return EMF_ERROR_NONE;
}

int email_set_password_to_account(emf_account_t *account, char *password)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->password = password;

	return EMF_ERROR_NONE;
}

int email_get_retrieval_mode_from_account(emf_account_t *account, emf_imap4_retrieval_mode_t *retrieval_mode)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*retrieval_mode = account->retrieval_mode;

	return EMF_ERROR_NONE;
}

int email_set_retrieval_mode_to_account(emf_account_t *account, emf_imap4_retrieval_mode_t retrieval_mode)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->retrieval_mode = retrieval_mode;

	return EMF_ERROR_NONE;
}

int email_get_port_num_from_account(emf_account_t *account, int *port_num)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*port_num = account->port_num;

	return EMF_ERROR_NONE;
}


int email_set_port_num_to_account(emf_account_t *account, int port_num)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->port_num = port_num;

	return EMF_ERROR_NONE;
}

int email_get_use_security_from_account(emf_account_t *account, int *use_security)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*use_security = account->use_security;

	return EMF_ERROR_NONE;
}

int email_set_use_security_to_account(emf_account_t *account, int use_security)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->use_security = use_security;

	return EMF_ERROR_NONE;
}

int email_get_sending_server_type_from_account(emf_account_t *account, emf_account_server_t*sending_server_type)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*sending_server_type = account->sending_server_type;

	return EMF_ERROR_NONE;
}

int email_set_sending_server_type_to_account(emf_account_t *account, emf_account_server_t sending_server_type)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->sending_server_type = sending_server_type;


	return EMF_ERROR_NONE;
}

int email_get_sending_server_addr_from_account(emf_account_t *account, char **sending_server_addr)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*sending_server_addr = account->sending_server_addr;

	return EMF_ERROR_NONE;
}

int email_set_sending_server_addr_to_account(emf_account_t *account, char *sending_server_addr)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->sending_server_addr = sending_server_addr;

	return EMF_ERROR_NONE;
}

int email_get_sending_port_num_from_account(emf_account_t *account, int *sending_port_num)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*sending_port_num = account->sending_port_num;

	return EMF_ERROR_NONE;
}

int email_set_sending_port_num_to_account(emf_account_t *account, int sending_port_num)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->sending_port_num = sending_port_num;

	return EMF_ERROR_NONE;
}

int email_get_sending_auth_from_account(emf_account_t *account, int *sending_auth)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*sending_auth = account->sending_auth;

	return EMF_ERROR_NONE;
}

int email_set_sending_auth_to_account(emf_account_t *account, int sending_auth)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->sending_auth = sending_auth;

	return EMF_ERROR_NONE;
}

int email_get_sending_security_from_account(emf_account_t *account, int *sending_security)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*sending_security = account->sending_security;

	return EMF_ERROR_NONE;
}

int email_set_sending_security_to_account(emf_account_t *account, int sending_security)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->sending_security = sending_security;

	return EMF_ERROR_NONE;
}

int email_get_sending_user_from_account(emf_account_t *account, char **sending_user)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*sending_user = account->sending_user;

	return EMF_ERROR_NONE;
}

int email_set_sending_user_to_account(emf_account_t *account, char *sending_user)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->sending_user = sending_user;

	return EMF_ERROR_NONE;
}

int email_get_sending_password_from_account(emf_account_t *account, char **sending_password)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*sending_password = account->sending_password;

	return EMF_ERROR_NONE;
}

int email_set_sending_password_to_account(emf_account_t *account, char *sending_password)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->sending_password = sending_password;

	return EMF_ERROR_NONE;
}

int email_get_display_name_from_account(emf_account_t *account, char **display_name)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*display_name = account->display_name;

	return EMF_ERROR_NONE;
}

int email_set_display_name_to_account(emf_account_t *account, char *display_name)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->display_name = display_name;

	return EMF_ERROR_NONE;
}

int email_get_reply_to_addr_from_account(emf_account_t *account, char **reply_to_addr)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*reply_to_addr = account->reply_to_addr;

	return EMF_ERROR_NONE;
}

int email_set_reply_to_addr_to_account(emf_account_t *account, char *reply_to_addr)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->reply_to_addr = reply_to_addr;

	return EMF_ERROR_NONE;
}

int email_get_return_addr_from_account(emf_account_t *account, char **return_addr)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*return_addr = account->return_addr;

	return EMF_ERROR_NONE;
}


int email_set_return_addr_to_account(emf_account_t *account, char *return_addr)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->return_addr = return_addr;

	return EMF_ERROR_NONE;
}

int email_get_account_id_from_account(emf_account_t *account, int *account_id)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*account_id = account->account_id;

	return EMF_ERROR_NONE;
}

int email_set_account_id_to_account(emf_account_t *account, int account_id)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->account_id = account_id;

	return EMF_ERROR_NONE;
}

int email_get_keep_on_server_from_account(emf_account_t *account, int *keep_on_server)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*keep_on_server = account->keep_on_server;

	return EMF_ERROR_NONE;
}

int email_set_keep_on_server_to_account(emf_account_t *account, int keep_on_server)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->keep_on_server = keep_on_server;

	return EMF_ERROR_NONE;
}

int email_get_flag1_from_account(emf_account_t *account, int *flag1)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*flag1 = account->flag1;

	return EMF_ERROR_NONE;
}


int email_set_flag1_to_account(emf_account_t *account, int flag1)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->flag1 = flag1;

	return EMF_ERROR_NONE;
}

int email_get_flag2_from_account(emf_account_t *account, int *flag2)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*flag2 = account->flag2;

	return EMF_ERROR_NONE;
}


int email_set_flag2_to_account(emf_account_t *account, int flag2)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->flag2 = flag2;


	return EMF_ERROR_NONE;
}

int email_get_pop_before_smtp_from_account(emf_account_t *account, int *pop_before_smtp)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*pop_before_smtp = account->pop_before_smtp;

	return EMF_ERROR_NONE;
}


int email_set_pop_before_smtp_to_account(emf_account_t *account, int pop_before_smtp)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->pop_before_smtp = pop_before_smtp;

	return EMF_ERROR_NONE;
}

int email_get_apop_from_account(emf_account_t *account, int *apop)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*apop = account->apop;

	return EMF_ERROR_NONE;
}

int email_set_apop_to_account(emf_account_t *account, int apop)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->apop = apop;

	return EMF_ERROR_NONE;
}

int email_get_logo_icon_path_from_account(emf_account_t *account, char **logo_icon_path)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*logo_icon_path = account->logo_icon_path;
	
	return EMF_ERROR_NONE;
}


int email_set_logo_icon_path_to_account(emf_account_t *account, char *logo_icon_path)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->logo_icon_path = logo_icon_path;

	return EMF_ERROR_NONE;
}

int email_get_preset_account_from_account(emf_account_t *account, int *preset_account)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*preset_account = account->preset_account;

	return EMF_ERROR_NONE;
}

int email_set_preset_account_to_account(emf_account_t *account, int preset_account)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->preset_account = preset_account;

	return EMF_ERROR_NONE;
}

int email_get_options_from_account(emf_account_t *account, emf_option_t **options)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*options = &account->options;

	return EMF_ERROR_NONE;
}

int email_set_options_to_account(emf_account_t *account, emf_option_t *options)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->options = *options;

	return EMF_ERROR_NONE;
}

int email_get_target_storage_from_account(emf_account_t *account, int *target_storage)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*target_storage = account->target_storage;

	return EMF_ERROR_NONE;
}

int email_set_target_storage_to_account(emf_account_t *account, int target_storage)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->target_storage = target_storage;

	return EMF_ERROR_NONE;
}

int email_get_check_interval_from_account(emf_account_t *account, int *check_interval)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	*check_interval = account->check_interval;

	return EMF_ERROR_NONE;
}

int email_set_check_interval_to_account(emf_account_t *account, int check_interval)
{
	if(!account)
		return EMF_ERROR_INVALID_PARAM;
	
	account->check_interval = check_interval;

	return EMF_ERROR_NONE;
}

/*  emf_mailbox_t--------------------------------------------------------------- */

int email_get_name_from_mailbox(emf_mailbox_t *mailbox, char **name)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*name = mailbox->name;

	return EMF_ERROR_NONE;
}

int email_set_name_to_mailbox(emf_mailbox_t *mailbox, char *name)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->name = name;

	return EMF_ERROR_NONE;
}
	
int email_get_mailbox_type_from_mailbox(emf_mailbox_t *mailbox, emf_mailbox_type_e mailbox_type)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox_type = mailbox->mailbox_type;

	return EMF_ERROR_NONE;
}

int email_set_mailbox_type_to_mailbox(emf_mailbox_t *mailbox, emf_mailbox_type_e mailbox_type)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->mailbox_type = mailbox_type;

	return EMF_ERROR_NONE;
}
	
int email_get_alias_from_mailbox(emf_mailbox_t *mailbox, char **alias)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*alias = mailbox->alias;

	return EMF_ERROR_NONE;
}

int email_set_alias_to_mailbox(emf_mailbox_t *mailbox, char *alias)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->alias = alias;

	return EMF_ERROR_NONE;
}
	
int email_get_unread_count_from_mailbox(emf_mailbox_t *mailbox, int *unread_count)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*unread_count = mailbox->unread_count;

	return EMF_ERROR_NONE;
}

int email_set_unread_count_to_mailbox(emf_mailbox_t *mailbox, int unread_count)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->unread_count = unread_count;

	return EMF_ERROR_NONE;
}
	
int email_get_hold_connection_from_mailbox(emf_mailbox_t *mailbox, int *hold_connection)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*hold_connection = mailbox->hold_connection;

	return EMF_ERROR_NONE;
}

int email_set_hold_connection_to_mailbox(emf_mailbox_t *mailbox, int hold_connection)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->hold_connection = hold_connection;

	return EMF_ERROR_NONE;
}
	
int email_get_local_from_mailbox(emf_mailbox_t *mailbox, int *local)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*local = mailbox->local;

	return EMF_ERROR_NONE;
}

int email_set_local_to_mailbox(emf_mailbox_t *mailbox, int local)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->local = local;

	return EMF_ERROR_NONE;
}
	
int email_get_synchronous_from_mailbox(emf_mailbox_t *mailbox, int *synchronous)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*synchronous = mailbox->synchronous;

	return EMF_ERROR_NONE;
}

int email_set_synchronous_to_mailbox(emf_mailbox_t *mailbox, int synchronous)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->synchronous = synchronous;

	return EMF_ERROR_NONE;
}
	
int email_get_account_id_from_mailbox(emf_mailbox_t *mailbox, int *account_id)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*account_id = mailbox->account_id;

	return EMF_ERROR_NONE;
}

int email_set_account_id_to_mailbox(emf_mailbox_t *mailbox, int account_id)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->account_id = account_id;

	return EMF_ERROR_NONE;
}
	
int email_get_user_data_from_mailbox(emf_mailbox_t *mailbox, void **user_data)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*user_data = mailbox->user_data;

	return EMF_ERROR_NONE;
}

int email_set_user_data_to_mailbox(emf_mailbox_t *mailbox, void *user_data)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->user_data = user_data;

	return EMF_ERROR_NONE;
}
	
int email_get_mail_stream_from_mailbox(emf_mailbox_t *mailbox, void **mail_stream)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*mail_stream = mailbox->mail_stream;

	return EMF_ERROR_NONE;
}

int email_set_mail_stream_to_mailbox(emf_mailbox_t *mailbox, void *mail_stream)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->mail_stream = mail_stream;

	return EMF_ERROR_NONE;
}
	
int email_get_has_archived_mails_from_mailbox(emf_mailbox_t *mailbox, int *has_archived_mails)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*has_archived_mails = mailbox->has_archived_mails;

	return EMF_ERROR_NONE;
}

int email_set_has_archived_mails_to_mailbox(emf_mailbox_t *mailbox, int has_archived_mails)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->has_archived_mails = has_archived_mails;

	return EMF_ERROR_NONE;
}
	
int email_get_account_name_from_mailbox(emf_mailbox_t *mailbox, char **account_name)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*account_name = mailbox->account_name;

	return EMF_ERROR_NONE;
}

int email_set_account_name_to_mailbox(emf_mailbox_t *mailbox, char *account_name)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->account_name= account_name;

	return EMF_ERROR_NONE;
}
	
int email_get_next_from_mailbox(emf_mailbox_t *mailbox, emf_mailbox_t **next)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	*next = mailbox->next;

	return EMF_ERROR_NONE;
}

int email_set_next_to_mailbox(emf_mailbox_t *mailbox, emf_mailbox_t *next)
{
	if(!mailbox)
		return EMF_ERROR_INVALID_PARAM;

	mailbox->next= next;

	return EMF_ERROR_NONE;
}


/*  emf_attachment_info_t------------------------------------------------------- */
	
int email_get_inline_content_from_attachment_info(emf_attachment_info_t *attachment_info, int *inline_content)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	*inline_content = attachment_info->inline_content;

	return EMF_ERROR_NONE;
}

int email_set_inline_content_to_attachment_info(emf_attachment_info_t *attachment_info, int inline_content)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	attachment_info->inline_content= inline_content;

	return EMF_ERROR_NONE;
}
	
int email_get_attachment_id_from_attachment_info(emf_attachment_info_t *attachment_info, int *attachment_id)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	*attachment_id = attachment_info->attachment_id;

	return EMF_ERROR_NONE;
}

int email_set_attachment_id_to_attachment_info(emf_attachment_info_t *attachment_info, int attachment_id)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	attachment_info->attachment_id= attachment_id;

	return EMF_ERROR_NONE;
}
	
int email_get_name_from_attachment_info(emf_attachment_info_t *attachment_info, char **name)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	*name = attachment_info->name;

	return EMF_ERROR_NONE;
}

int email_set_name_to_attachment_info(emf_attachment_info_t *attachment_info, char *name)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	attachment_info->name= name;

	return EMF_ERROR_NONE;
}
	
int email_get_size_from_attachment_info(emf_attachment_info_t *attachment_info, int *size)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	*size = attachment_info->size;

	return EMF_ERROR_NONE;
}

int email_set_size_to_attachment_info(emf_attachment_info_t *attachment_info, int size)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	attachment_info->size= size;

	return EMF_ERROR_NONE;
}
	
int email_get_downloaded_from_attachment_info(emf_attachment_info_t *attachment_info, int *downloaded)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	*downloaded = attachment_info->downloaded;

	return EMF_ERROR_NONE;
}

int email_set_downloaded_to_attachment_info(emf_attachment_info_t *attachment_info, int downloaded)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	attachment_info->downloaded= downloaded;

	return EMF_ERROR_NONE;
}
	
int email_get_savename_from_attachment_info(emf_attachment_info_t *attachment_info, char **savename)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	*savename = attachment_info->savename;

	return EMF_ERROR_NONE;
}

int email_set_savename_to_attachment_info(emf_attachment_info_t *attachment_info, char *savename)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	attachment_info->savename= savename;

	return EMF_ERROR_NONE;
}
	
int email_get_drm_from_attachment_info(emf_attachment_info_t *attachment_info, int *drm)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	*drm = attachment_info->drm;

	return EMF_ERROR_NONE;
}

int email_set_drm_to_attachment_info(emf_attachment_info_t *attachment_info, int drm)
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	attachment_info->drm= drm;

	return EMF_ERROR_NONE;
}
	
int email_get_next_from_attachment_info(emf_attachment_info_t *attachment_info, emf_attachment_info_t **next) 
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	*next = attachment_info->next;

	return EMF_ERROR_NONE;
}

int email_set_next_to_attachment_info(emf_attachment_info_t *attachment_info, emf_attachment_info_t *next) 
{
	if(!attachment_info)
		return EMF_ERROR_INVALID_PARAM;

	attachment_info->next= next;

	return EMF_ERROR_NONE;
}

/*  emf_option_t---------------------------------------------------------------- */
	
int email_get_priority_from_option(emf_option_t *option, int *priority)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*priority = option->priority;

	return EMF_ERROR_NONE;
}

int email_set_priority_to_option(emf_option_t *option, int priority)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->priority= priority;

	return EMF_ERROR_NONE;
}
	
int email_get_keep_local_copy_from_option(emf_option_t *option, int *keep_local_copy)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*keep_local_copy = option->keep_local_copy;


	return EMF_ERROR_NONE;
}

int email_set_keep_local_copy_to_option(emf_option_t *option, int keep_local_copy)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->keep_local_copy= keep_local_copy;

	return EMF_ERROR_NONE;
}
	
int email_get_req_delivery_receipt_from_option(emf_option_t *option, int *req_delivery_receipt)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*req_delivery_receipt = option->req_delivery_receipt;

	return EMF_ERROR_NONE;
}

int email_set_req_delivery_receipt_to_option(emf_option_t *option, int req_delivery_receipt)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->req_delivery_receipt= req_delivery_receipt;

	return EMF_ERROR_NONE;
}
	
int email_get_req_read_receipt_from_option(emf_option_t *option, int *req_read_receipt)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*req_read_receipt = option->req_read_receipt;

	return EMF_ERROR_NONE;
}

int email_set_req_read_receipt_to_option(emf_option_t *option, int req_read_receipt)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->req_read_receipt = req_read_receipt;

	return EMF_ERROR_NONE;
}
	
int email_get_download_limit_from_option(emf_option_t *option, int *download_limit)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*download_limit = option->download_limit;

	return EMF_ERROR_NONE;
}

int email_set_download_limit_to_option(emf_option_t *option, int download_limit)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->download_limit = download_limit;


	return EMF_ERROR_NONE;
}
	
int email_get_block_address_from_option(emf_option_t *option, int *block_address)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*block_address = option->block_address;

	return EMF_ERROR_NONE;
}

int email_set_block_address_to_option(emf_option_t *option, int block_address)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->block_address = block_address;

	return EMF_ERROR_NONE;
}
	
int email_get_block_subject_from_option(emf_option_t *option, int *block_subject)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*block_subject = option->block_subject;

	return EMF_ERROR_NONE;
}

int email_set_block_subject_to_option(emf_option_t *option, int block_subject)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->block_subject = block_subject;

	return EMF_ERROR_NONE;
}
	
int email_get_display_name_from_from_option(emf_option_t *option, char **display_name_from)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*display_name_from = option->display_name_from;

	return EMF_ERROR_NONE;
}

int email_set_display_name_from_to_option(emf_option_t *option, char *display_name_from)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->display_name_from = display_name_from;

	return EMF_ERROR_NONE;
}
	
int email_get_reply_with_body_from_option(emf_option_t *option, int *reply_with_body)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*reply_with_body = option->reply_with_body;

	return EMF_ERROR_NONE;
}

int email_set_reply_with_body_to_option(emf_option_t *option, int reply_with_body)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->reply_with_body = reply_with_body;

	return EMF_ERROR_NONE;
}
	
int email_get_forward_with_files_from_option(emf_option_t *option, int *forward_with_files)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*forward_with_files = option->forward_with_files;

	return EMF_ERROR_NONE;
}

int email_set_forward_with_files_to_option(emf_option_t *option, int forward_with_files)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->forward_with_files = forward_with_files;

	return EMF_ERROR_NONE;
}
	
int email_get_add_myname_card_from_option(emf_option_t *option, int *add_myname_card)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*add_myname_card = option->add_myname_card;

	return EMF_ERROR_NONE;
}

int email_set_add_myname_card_to_option(emf_option_t *option, int add_myname_card)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->add_myname_card = add_myname_card;

	return EMF_ERROR_NONE;
}
	
int email_get_add_signature_from_option(emf_option_t *option, int *add_signature)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*add_signature = option->add_signature;

	return EMF_ERROR_NONE;
}

int email_set_add_signature_to_option(emf_option_t *option, int add_signature)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->add_signature = add_signature;

	return EMF_ERROR_NONE;
}
	
int email_get_signature_from_option(emf_option_t *option, char **signature)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	*signature = option->signature;

	return EMF_ERROR_NONE;
}

int email_set_signature_to_option(emf_option_t *option, char *signature)
{
	if(!option)
		return EMF_ERROR_INVALID_PARAM;

	option->signature = signature;

	return EMF_ERROR_NONE;
}

/*  emf_rule_t------------------------------------------------------------------- */
int email_get_account_id_from_rule(emf_rule_t *rule, int *account_id)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	*account_id = rule->account_id;

	return EMF_ERROR_NONE;
}

int email_set_account_id_to_rule(emf_rule_t *rule, int account_id)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	rule->account_id = account_id;

	return EMF_ERROR_NONE;
}
	
int email_get_filter_id_from_rule(emf_rule_t *rule, int *filter_id)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	*filter_id = rule->filter_id;

	return EMF_ERROR_NONE;
}

int email_set_filter_id_to_rule(emf_rule_t *rule, int filter_id)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	rule->filter_id = filter_id;

	return EMF_ERROR_NONE;
}
	
int email_get_type_from_rule(emf_rule_t *rule, emf_rule_type_t type)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	type = rule->type;

	return EMF_ERROR_NONE;
}

int email_set_type_to_rule(emf_rule_t *rule, emf_rule_type_t type)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	rule->type = type;

	return EMF_ERROR_NONE;
}
	
int email_get_value_from_rule(emf_rule_t *rule, char **value)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	*value = rule->value;

	return EMF_ERROR_NONE;
}

int email_set_value_to_rule(emf_rule_t *rule, char *value)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	rule->value = value;

	return EMF_ERROR_NONE;
}
	
int email_get_action_from_rule(emf_rule_t *rule, emf_rule_action_t action)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	action = rule->faction;

	return EMF_ERROR_NONE;
}

int email_set_action_to_rule(emf_rule_t *rule, emf_rule_action_t action)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	rule->faction = action;

	return EMF_ERROR_NONE;
}
	
int email_get_flag1_from_rule(emf_rule_t *rule, int *flag1)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	*flag1 = rule->flag1;

	return EMF_ERROR_NONE;
}

int email_set_flag1_to_rule(emf_rule_t *rule, int flag1)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	rule->flag1 = flag1;

	return EMF_ERROR_NONE;
}
	
int email_get_flag2_from_rule(emf_rule_t *rule, int *flag2)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	*flag2 = rule->flag2;

	return EMF_ERROR_NONE;
}

int email_set_flag2_to_rule(emf_rule_t *rule, int flag2)
{
	if(!rule)
		return EMF_ERROR_INVALID_PARAM;

	rule->flag2 = flag2;

	return EMF_ERROR_NONE;
}



