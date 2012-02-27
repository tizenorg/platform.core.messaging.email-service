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
 * File: emf-mail.c
 * Desc: Mail Framework Mail Operation
 *
 * Auth: 
 *
 * History:
 *    2006.08.16 : created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include <unistd.h>
#include <malloc.h>

#include "emflib.h"
#include "emf-global.h"
#include "em-core-event.h"
#include "emf-account.h"
#include "emf-dbglog.h"
#include "em-storage.h"
#include "em-core-account.h"
#include "em-core-mesg.h"
#include "em-core-mailbox.h" 
#include "em-core-utils.h" 
#include "em-core-smtp.h" 
#include "em-core-timer.h" 

#ifdef __LOCAL_ACTIVITY__
extern int g_local_activity_run;
extern int g_save_local_activity_run;
#endif
static int emf_mail_check_mail_id(int mail_id, int* err_code);

EXPORT_API int emf_mail_send(emf_mailbox_t* mailbox, int mail_id, emf_option_t* sending_option, unsigned* handle, int* err_code)
{	
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], sending_option[%p], handle[%p], err_code[%p]", mailbox, mail_id, sending_option, handle, err_code);
	
	int ret = false, err = EMF_ERROR_NONE, err_2 = EMF_ERROR_NONE;
	int  result_handle = 0, account_id = 0;
	emf_mailbox_t dest_mbox;
	emf_option_t* option = NULL;
	emf_event_t event_data;
	char* mailbox_name = NULL;
	
	if (!mailbox || !mailbox->name || mailbox->account_id <= 0) {
		if (mailbox != NULL)
			EM_DEBUG_EXCEPTION(" mailbox->name[%s], mailbox->account_id[%d]", mailbox->name, mailbox->account_id);
		if (err_code)
			*err_code = EMF_ERROR_INVALID_MAILBOX;		
		return false;
	}
	
	account_id = mailbox->account_id;
	
	if (sending_option != NULL) {
		if (!(option = (emf_option_t*)em_core_malloc(sizeof(emf_option_t)))) {	
			EM_DEBUG_EXCEPTION("em_core_malloc for sending_option failed...");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		memcpy(option, sending_option, sizeof(emf_option_t));
		option->display_name_from = EM_SAFE_STRDUP(sending_option->display_name_from);
	}
	
	emf_account_t* ref_account = emf_get_account_reference(account_id);

	if (!ref_account) {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

#ifdef __FEATURE_MOVE_TO_OUTBOX_FIRST__
	if (!em_storage_get_mailboxname_by_mailbox_type(account_id,EMF_MAILBOX_TYPE_OUTBOX,&mailbox_name, false, &err)) {
		EM_DEBUG_EXCEPTION(" em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	if (strcmp(mailbox->name, mailbox_name)) {	
		dest_mbox.name = mailbox_name;
		dest_mbox.account_id = account_id;
		
		/*  mail is moved to 'OUTBOX' first of all. */
		if (!em_core_mail_move(&mail_id, 1, dest_mbox.name, EMF_MOVED_AFTER_SENDING, 0, &err)) {
			EM_DEBUG_EXCEPTION("em_core_mail_move falied [%d]", err);
			goto FINISH_OFF;
		}
	}
#endif /* __FEATURE_MOVE_TO_OUTBOX_FIRST__ */

	if(!em_storage_notify_network_event(NOTI_SEND_START, account_id, NULL, mail_id, 0))
		EM_DEBUG_EXCEPTION(" em_storage_notify_network_event [ NOTI_SEND_START] Failed >>>> ");
	
	/* set EMF_MAIL_STATUS_SEND_WAIT status */

	if(!em_storage_set_field_of_mails_with_integer_value(account_id, &mail_id, 1, "save_status", EMF_MAIL_STATUS_SEND_WAIT, true, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_set_field_of_mails_with_integer_value [%d]",err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	switch (ref_account->account_bind_type) {
		case EMF_BIND_TYPE_EM_CORE:
			memset(&event_data, 0x00, sizeof(emf_event_t));
			event_data.type               = EMF_EVENT_SEND_MAIL;
			event_data.account_id         = account_id;
			event_data.event_param_data_1 = (char*)option;
			event_data.event_param_data_3 = EM_SAFE_STRDUP(mailbox->name);
			event_data.event_param_data_4 = mail_id;
			
			if (!em_core_insert_send_event(&event_data, &result_handle, &err)) {
				EM_DEBUG_EXCEPTION(" em_core_insert_event failed [%d]", err);
				goto FINISH_OFF;
			}
#ifdef __LOCAL_ACTIVITY__
			EM_DEBUG_LOG("Setting g_save_local_activity_run ");
			g_save_local_activity_run = 1;
#endif
			break;
		
		default:
			EM_DEBUG_EXCEPTION("unsupported account binding type...");
			err = EMF_ERROR_NOT_SUPPORTED;
			goto FINISH_OFF;
	}
	
	if ( handle )
		*handle = result_handle;

	ret = true;
	
FINISH_OFF:
	if (ret == false) {	
		EM_DEBUG_EXCEPTION("emf_mail_send failed [%d]", err);			

		if(!em_storage_set_field_of_mails_with_integer_value(account_id, &mail_id, 1, "save_status", EMF_MAIL_STATUS_SAVED, true, &err))
			EM_DEBUG_EXCEPTION("em_storage_set_field_of_mails_with_integer_value [%d]",err);

		EM_SAFE_FREE(event_data.event_param_data_3);
		
		if(option != NULL) {	
			EM_SAFE_FREE(option->display_name_from);
			EM_SAFE_FREE(option);
		}
	}

	if(!em_core_add_transaction_info(mail_id , result_handle , &err_2))
		EM_DEBUG_EXCEPTION("em_core_add_transaction_info failed [%d]", err_2);

	EM_SAFE_FREE(mailbox_name);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_mail_send_saved(int account_id, emf_option_t* sending_option,  unsigned* handle, int* err_code)
{	
	EM_DEBUG_FUNC_BEGIN("account_id[%d], sending_option[%p], handle[%p], err_code[%p]", account_id, sending_option, handle, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	emf_option_t* option = NULL;
	emf_event_t event_data;
	char *mailbox_name = NULL;	
	
	memset(&event_data, 0x00, sizeof(emf_event_t));
	
	if (account_id <= 0)  {
		EM_DEBUG_EXCEPTION("account_id = %d", account_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (sending_option)  {
		option = (emf_option_t*)em_core_malloc(sizeof(emf_option_t));
		if (!option)  {
			EM_DEBUG_EXCEPTION("em_core_malloc failed...");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memcpy(option, sending_option, sizeof(emf_option_t));
	}
	
	emf_account_t* ref_account = emf_get_account_reference(account_id);

	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	
	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE:
			event_data.type = EMF_EVENT_SEND_MAIL_SAVED;
						
			if (!em_storage_get_mailboxname_by_mailbox_type(account_id,EMF_MAILBOX_TYPE_OUTBOX,&mailbox_name, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
							
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
			event_data.event_param_data_3 = EM_SAFE_STRDUP(mailbox_name);		
			event_data.account_id  = account_id;
			event_data.event_param_data_1 = (char*)option;
		
			if (!em_core_insert_event(&event_data, (int*)handle, &err))  {
				EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
				goto FINISH_OFF;
			}
			break;
			
		default :
			EM_DEBUG_EXCEPTION("unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (ret == false)  {
		EM_SAFE_FREE(event_data.event_param_data_3);
		EM_SAFE_FREE(option);
	}

	EM_SAFE_FREE(mailbox_name);
	
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_mail_send_report(emf_mail_t* mail,     unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail[%p], handle[%p], err_code[%p]", mail, handle, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;

	emf_mail_t* tmail = NULL;
	char* mailbox = NULL;
	char *mailbox_name = NULL;
	
	if (!mail || !mail->info || mail->info->account_id <= 0 || !mail->head || 
		!mail->head->from || !mail->head->mid)  {
		if (mail != NULL)  {
			if (mail->info != NULL)  {
				if (mail->head != NULL)
					EM_DEBUG_LOG("mail->info->account_id[%d], mail->head->from[%p], mail->head->mid[%p]", mail->info->account_id, mail->head->from, mail->head->mid);
				else
					EM_DEBUG_LOG("mail->info->account_id[%d], mail->head[%p]", mail->info->account_id, mail->head);		
			}
			else {
				if (mail->head != NULL)
					EM_DEBUG_LOG("mail->info[%p],  mail->head->from[%p], mail->head->mid[%p]", mail->info, mail->head->from, mail->head->mid);
				else
					EM_DEBUG_LOG("mail->info[%p],  mail->head[%p]", mail->info, mail->head);
			}
		}
		else
			EM_DEBUG_LOG("mail[%p]", mail);
		
		err = EMF_ERROR_INVALID_MAIL; /* EMF_ERROR_INVALID_PARAM */
		goto FINISH_OFF;
	}

	emf_account_t* ref_account = emf_get_account_reference(mail->info->account_id);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", mail->info->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE:
			tmail = em_core_malloc(sizeof(emf_mail_t));
			tmail->head = em_core_malloc(sizeof(emf_mail_head_t));
			
			if (!tmail || !tmail->head)  {
				EM_DEBUG_EXCEPTION("mailloc failed...");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			memset(tmail, 0x00, sizeof(emf_mail_t));
			memset(tmail->head, 0x00, sizeof(emf_mail_head_t));
			
			tmail->head->to = EM_SAFE_STRDUP(mail->head->from);
			tmail->head->mid = EM_SAFE_STRDUP(mail->head->mid);
			
			if (mail->head->subject)  {
				tmail->head->subject = malloc(strlen(mail->head->subject) + 7);
				if (!(tmail->head->subject))  {	
					EM_DEBUG_EXCEPTION("mailloc failed...");
					err = EMF_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}
				
				SNPRINTF(tmail->head->subject, strlen(mail->head->subject) + 7 - 1,  "Read: %s", mail->head->subject);
			}
			
			tmail->info = em_core_malloc(sizeof(emf_mail_info_t));
			if (!(tmail->info))  {	
				EM_DEBUG_EXCEPTION("mailloc failed...");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			
			memset(tmail->info, 0x00, sizeof(emf_mail_info_t));
			
			tmail->info->account_id = mail->info->account_id;
			tmail->info->flags.draft = 1;
			tmail->info->extra_flags.status = (unsigned char)EMF_MAIL_STATUS_SENDING;/* 4; */
			tmail->info->extra_flags.report = (unsigned char)EMF_MAIL_REPORT_MDN;
						
		if (!em_storage_get_mailboxname_by_mailbox_type(mail->info->account_id,EMF_MAILBOX_TYPE_OUTBOX,&mailbox_name, false, &err))  {
			EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
			goto FINISH_OFF;
		}
		mailbox = EM_SAFE_STRDUP(mailbox_name);
			
			
			if (!em_core_mail_save(tmail->info->account_id, mailbox, tmail, NULL, 1, &err)) {
				EM_DEBUG_EXCEPTION("em_core_mail_save failed [%d]", err);
				goto FINISH_OFF;
			}
			else  {
				emf_event_t event_data = { 0 };
				event_data.type = EMF_EVENT_SEND_MAIL;
				event_data.account_id = tmail->info->account_id;
				event_data.event_param_data_4 = tmail->info->uid;
				event_data.event_param_data_3 = mailbox;
				event_data.event_param_data_1 = NULL;
				
				if (!em_core_insert_event(&event_data,(int*) handle, &err))  { 
					EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
					goto FINISH_OFF;
				}
#ifdef __LOCAL_ACTIVITY__
				EM_DEBUG_LOG("Setting g_save_local_activity_run ");
				g_save_local_activity_run = 1;
#endif

			}
			break;
			
		default:
			EM_DEBUG_EXCEPTION("unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;

	}
	
	ret = true;

FINISH_OFF:
	if (tmail != NULL)
		em_core_mail_free(&tmail, 1, NULL);	
	if (ret == false)
		EM_SAFE_FREE(mailbox);

	EM_SAFE_FREE(mailbox_name);
	
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_add_mail(emf_mail_data_t *input_mail_data, emf_attachment_data_t *input_attachment_data_list, int input_attachment_count, emf_meeting_request_t *input_meeting_request, int input_sync_server)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count [%d], input_meeting_req [%p], input_sync_server[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_sync_server);

	int         err = EMF_ERROR_NONE;
	int         handle = 0;
	emf_event_t event_data = { 0 };
	
	if (!input_mail_data || input_mail_data->account_id <= 0 ||
		(input_mail_data->report_status == EMF_MAIL_REPORT_MDN && !input_mail_data->full_address_to))  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM"); 
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	emf_account_t* ref_account = emf_get_account_reference(input_mail_data->account_id);
	if (!ref_account)  {
		EM_DEBUG_LOG(" emf_get_account_reference failed [%d]", input_mail_data->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE : 
			if ((err = em_core_add_mail(input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_sync_server)) != EMF_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("em_core_add_mail failed [%d]", err);
				goto FINISH_OFF;
			}

#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
			if ( input_sync_server ) {	
				event_data.type               = EMF_EVENT_SAVE_MAIL;
				event_data.account_id         = input_mail_data->account_id;
				event_data.event_param_data_4 = input_mail_data->mail_id;

				if (!em_core_insert_send_event(&event_data, &handle, &err))  {
					EM_DEBUG_EXCEPTION("em_core_insert_send_event failed [%d]", err);
					goto FINISH_OFF;
				}
			}
#endif			
		break;

		default:
			EM_DEBUG_EXCEPTION("unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
	}
FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
	
EXPORT_API int emf_mail_save_to_mailbox(emf_mail_t* mail, emf_mailbox_t* mailbox, emf_meeting_request_t *meeting_req, int from_composer, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail[%p], mailbox[%p], err_code[%p]", mail, mailbox, err_code);

	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	struct stat st_buf;
	emf_event_t event_data;
	int  handle;
	
	if (!mail || !mail->head || !mail->body || !mailbox || mailbox->account_id <= 0 ||
		(mail->info && mail->info->extra_flags.report == EMF_MAIL_REPORT_MDN && !mail->head->to))  {
		EM_DEBUG_EXCEPTION("mail[%p], head[%p], body[%p], mailbox[%p], account_id[%d], info[%p], report[%d], to[%p]",
		mail, mail ? mail->head : 0, mail ? mail->body : 0, mailbox, mailbox ? mailbox->account_id : 0, 
		mail ? mail->info : 0, mail ? (mail->info?mail->info->extra_flags.report : 0) : 0, mail ? (mail->head ? mail->head->to : 0) : 0); 
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
#ifdef __LOCAL_ACTIVITY__	
	char *mailbox_name = NULL;
	if( mailbox->name ) {
		mailbox_name =  EM_SAFE_STRDUP(mailbox->name) ;  
	}

#endif

	if(from_composer) {
		if (mail->body->plain)  {
			if (stat(mail->body->plain, &st_buf) < 0)  {
				EM_DEBUG_EXCEPTION("mail->body->plain, stat(\"%s\") failed...", mail->body->plain);
				err = EMF_ERROR_INVALID_MAIL;
				goto FINISH_OFF;
			}
		}
		
		if (mail->body->html)  {
			if (stat(mail->body->html, &st_buf) < 0)  {
				EM_DEBUG_EXCEPTION("mail->body->html, stat(\"%s\") failed...", mail->body->html);
				
				err = EMF_ERROR_INVALID_MAIL;
				goto FINISH_OFF;
			}

		}
		
		if (mail->body->attachment)  {
			emf_attachment_info_t* p = mail->body->attachment;
			while (p)  {
				if(p->downloaded) {
					if (!p->savename || stat(p->savename, &st_buf) < 0)  {
						EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", p->savename);
						err = EMF_ERROR_INVALID_ATTACHMENT;		
						goto FINISH_OFF;
					}
				}
				p = p->next;
			}
		}
	}
	
	emf_account_t* ref_account = emf_get_account_reference(mailbox->account_id);
	if (!ref_account)  {
		EM_DEBUG_LOG(" emf_get_account_reference failed [%d]", mailbox->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;		/* EMF_ERROR_ACCOUNT_NOT_FOUND; */
		goto FINISH_OFF;
	}
	
	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE : 
			if (!em_core_mail_save(mailbox->account_id, mailbox->name, mail, meeting_req, from_composer, &err)) {
				EM_DEBUG_EXCEPTION("em_core_mail_save failed [%d]", err);
				goto FINISH_OFF;
			}


#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__	/*  Disable temporarily */
/* 			if ( from_composer && ref_account->receiving_server_type == EMF_SERVER_TYPE_IMAP4 ) */
			if ( from_composer && !mailbox->local) /* Not to be considered for Outbox mails which is local mailbox */ {	
				memset(&event_data, 0x00, sizeof(emf_event_t));

				EM_DEBUG_LOG(">>>>> Save to mailbox on a server: MAIL ID [ %d ] >>>>", mail->info->uid);

				event_data.type = EMF_EVENT_SAVE_MAIL;
				event_data.account_id = mailbox->account_id;
				event_data.event_param_data_4 = mail->info->uid;

#ifdef __LOCAL_ACTIVITY__
				emf_activity_tbl_t new_activity;
				memset( &new_activity, 0x00, sizeof(emf_activity_tbl_t));
				new_activity.activity_type = ACTIVITY_SAVEMAIL;
				new_activity.account_id    = event_data.account_id;
				new_activity.mail_id	   = event_data.event_param_data_4;
	
				if( mailbox_name ) {
					new_activity.dest_mbox =  mailbox_name ;  
				}

				if (false == em_core_get_next_activity_id(&new_activity.activity_id,&err)) {
					EM_DEBUG_EXCEPTION(" em_core_get_next_activity_id Failed - %d \n", err);
				}
	
				if (!em_core_activity_add(&new_activity, &err)) {
					EM_DEBUG_EXCEPTION(" em_core_activity_add Failed - %d \n", err);
				}
#endif /* __LOCAL_ACTIVITY__ */

				if (!em_core_insert_send_event(&event_data, &handle, &err))  {
					EM_DEBUG_EXCEPTION("em_core_insert_send_event failed [%d]", err);
					err = EMF_ERROR_NONE;
					ret = true;
					goto FINISH_OFF;
				}
#ifdef __LOCAL_ACTIVITY__
				EM_DEBUG_LOG("Setting g_save_local_activity_run ");
				g_save_local_activity_run = 1;
#endif				
			}
#endif			
		break;

		default:
			EM_DEBUG_EXCEPTION("unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
	}/*  if with_server */

	ret = true;
	
FINISH_OFF:

#ifdef __LOCAL_ACTIVITY__
	EM_SAFE_FREE(mailbox_name);
#endif
	
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_mail_add_meeting_request(int account_id, char *mailbox_name, emf_meeting_request_t *meeting_req, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%s], meeting_req[%p], err_code[%p]", account_id, mailbox_name, meeting_req, err_code);

	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if ( account_id <= 0 || !meeting_req || meeting_req->mail_id <= 0 )  {
		if(meeting_req)
			EM_DEBUG_EXCEPTION("mail_id[%d]", meeting_req->mail_id);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!em_core_mail_add_meeting_request(account_id, mailbox_name, meeting_req, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_mail_save_meeting_request failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

/*  unused API */
int emf_mail_get_size(emf_mailbox_t* mailbox, int mail_id, int* size, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], size[%p], err_code[%p]", mailbox, mail_id, size, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!size)  {
		EM_DEBUG_EXCEPTION("mailbox[%p], mail_id[%d], size[%p]", mailbox, mail_id, size);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	
	if (!em_core_mail_get_size(/*mailbox,*/ mail_id, size, &err))  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_size failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

int emf_mail_get_mail(emf_mailbox_t* mailbox, int mail_id, emf_mail_t** mail, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], mail[%p], err_code[%p]", mailbox, mail_id, mail, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!mail)  {
		EM_DEBUG_EXCEPTION("mailbox[%p], mail_id[%d], mail[%p]", mailbox, mail_id, mail);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!em_core_mail_get_mail(/*mailbox,*/ mail_id, mail, &err) || !mail)  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_mail failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_mail_get_info(emf_mailbox_t* mailbox, int mail_id, emf_mail_info_t** info, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], info[%p], err_code[%p]", mailbox, mail_id, info, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!info)  {
		EM_DEBUG_EXCEPTION("mailbox[%p], mail_id[%d], info[%p]", mailbox, mail_id, info);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!em_core_mail_get_info(mail_id, info, &err) || !info)  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_info failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

int emf_mail_get_head(emf_mailbox_t* mailbox, int mail_id, emf_mail_head_t** head, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], head[%p], err_code[%p]", mailbox, mail_id, head, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!head)  {
		EM_DEBUG_EXCEPTION("mailbox[%p], mail_id[%d], head[%p]", mailbox, mail_id, head);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!em_core_mail_get_header(/*mailbox,*/ mail_id, head, &err) || !head)  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_header failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	
	return ret;
}

int emf_mail_get_body(emf_mailbox_t* mailbox, int mail_id, emf_mail_body_t** body, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], body[%p], err_code[%p]", mailbox, mail_id, body, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!body)  {
		EM_DEBUG_EXCEPTION("mailbox[%p], mail_id[%d], body[%p]", mailbox, mail_id, body);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!em_core_mail_get_body(/*mailbox,*/ mail_id, body, &err) || !body)  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_body failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_mail_download_body(emf_mailbox_t* mailbox, int mail_id, int verbose, int with_attachment,  unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], verbose[%d], with_attachment[%d], handle[%p], err_code[%p]", mailbox, mail_id, verbose, with_attachment,  handle, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
/* 	mpNewMail_StopAlertSound(); */
	
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	emf_event_t event_data;
	
	memset(&event_data, 0x00, sizeof(emf_event_t));
	
	event_data.type = EMF_EVENT_DOWNLOAD_BODY;
	event_data.event_param_data_1 = NULL;
	event_data.event_param_data_4 = mail_id;
	event_data.account_id = mailbox->account_id;
	event_data.event_param_data_3 = GINT_TO_POINTER(verbose << 1 | with_attachment);
	
	if (!em_core_insert_event(&event_data, (int*)handle, &err))  {
		EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
		err = EMF_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}
#ifdef __LOCAL_ACTIVITY__	
	EM_DEBUG_LOG("Setting g_local_activity_run ");
	g_local_activity_run = 1;	
#endif
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

int emf_mail_get_attachment(emf_mailbox_t* mailbox, int mail_id,  char* attachment_id, emf_attachment_info_t** attachment, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], attachment_id[%s], attachment[%p], err_code[%p]", mailbox, mail_id, attachment_id, attachment, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!attachment_id || !attachment)  {
		EM_DEBUG_EXCEPTION("mailbox[%p], mail_id[%d], attachment_id[%p], attachment[%p]\n", mailbox, mail_id, attachment_id, attachment);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!em_core_mail_get_attachment(/*mailbox,*/ mail_id, attachment_id, attachment, &err) || !attachment)  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_attachment failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

int emf_mail_add_attachment(emf_mailbox_t* mailbox, int mail_id, emf_attachment_info_t* attachment, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], attachment[%p], err_code[%p]", mailbox, mail_id, attachment, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;

	if (!attachment)  {
		EM_DEBUG_EXCEPTION(" mailbox[%p], mail_id[%d], attachment[%p]", mailbox, mail_id, attachment);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION(" emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!em_core_mail_add_attachment(/*mailbox,*/ mail_id, attachment, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_mail_add_attachment failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

int emf_mail_delete_attachment(emf_mailbox_t* mailbox, int mail_id,  char* attachment_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], attachment_id[%s], err_code[%p]", mailbox, mail_id, attachment_id, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!attachment_id)  {
		EM_DEBUG_EXCEPTION(" mailbox[%p], mail_id[%d], attachment_id[%p]", mailbox, mail_id, attachment_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION(" emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!em_core_mail_delete_attachment(mail_id, attachment_id, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_mail_delete_attachment failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_mail_download_attachment(emf_mailbox_t* mailbox, int mail_id, char* attachment, unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], attachment[%p], handle[%p], err_code[%p]", mailbox, mail_id, attachment, handle, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!attachment)  {
		EM_DEBUG_EXCEPTION("attachment[%p] is invalid", attachment);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	emf_event_t event_data;
	
	memset(&event_data, 0x00, sizeof(emf_event_t));
	
	event_data.type = EMF_EVENT_DOWNLOAD_ATTACHMENT;
	event_data.event_param_data_1 = NULL;
	event_data.event_param_data_4 = mail_id;
	event_data.account_id = mailbox->account_id;	
	event_data.event_param_data_3 = EM_SAFE_STRDUP(attachment);
	
	if (!em_core_insert_event(&event_data, (int*)handle, &err))  {
		EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

#ifdef __LOCAL_ACTIVITY__	
	EM_DEBUG_LOG("Setting g_local_activity_run ");
	g_local_activity_run = 1;	
#endif

	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	
	return ret;
}

int emf_mail_info_free(emf_mail_info_t** mail_info_list, int count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	return em_core_mail_info_free(mail_info_list, count, err_code);
}

int emf_mail_head_free(emf_mail_head_t** mail_head_list, int count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	return em_core_mail_head_free(mail_head_list, count, err_code);
}

int emf_mail_body_free(emf_mail_body_t**  mail_body_list,  int count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	return em_core_mail_body_free(mail_body_list, count, err_code);
}

int emf_mail_free(emf_mail_t** mail_list, int count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	return em_core_mail_free(mail_list, count, err_code);
}

EXPORT_API int emf_mail_attachment_info_free(emf_attachment_info_t** atch_info, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	return em_core_mail_attachment_info_free(atch_info, err_code);
}

void* thread_func_to_delete_mail(void *thread_argument)
{
	EM_DEBUG_FUNC_BEGIN();
	int *mail_id_list = NULL, mail_id_count, account_id, err;
	unsigned handle = 0;
	emf_event_t *event_data = (emf_event_t*)thread_argument;

	account_id         = event_data->account_id;
	mail_id_list       = (int*)event_data->event_param_data_3;
	mail_id_count      = event_data->event_param_data_4;

	if (!em_core_mail_delete(account_id, mail_id_list, mail_id_count, EMF_DELETE_LOCALLY, EMF_DELETED_BY_COMMAND, false, &err)) {
		EM_DEBUG_EXCEPTION(" em_core_mail_delete falied [%d]", err);
		goto FINISH_OFF;
	}
	
	if (!em_core_insert_event(event_data, (int*)handle, &err)) {
		EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:	
	/* em_core_free_event(event_data); */ /* all of members will be freed after using in each event handler */
	EM_SAFE_FREE(event_data);

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

EXPORT_API int emf_mail_delete(int account_id, emf_mailbox_t* mailbox, int mail_ids[], int mail_ids_count, int from_server, unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_ids[%p], mail_ids_count[%d], from_server[%d], handle[%p], err_code[%p]", mailbox, mail_ids, mail_ids_count, from_server, handle, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	int* p = NULL, thread_error;
	emf_event_t *event_data = NULL;
	emf_account_t *account = NULL;
	thread_t delete_thread;

	/* mailbox can be NULL for deleting thread mail. */
	if (mail_ids_count <= 0) {
		EM_DEBUG_EXCEPTION("mail_ids_count [%d]", mail_ids_count);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!(account = em_core_get_account_reference(account_id))) {
		EM_DEBUG_EXCEPTION("em_core_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if ((p = em_core_malloc(sizeof(int) * mail_ids_count)) == NULL) {
		EM_DEBUG_EXCEPTION("em_core_malloc for p failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	memcpy(p, mail_ids, sizeof(int) * mail_ids_count);

	if ((event_data = em_core_malloc(sizeof(emf_event_t)) ) == NULL) {
		EM_DEBUG_EXCEPTION("em_core_malloc for event_data failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	event_data->type                   = EMF_EVENT_DELETE_MAIL;
	event_data->account_id             = account_id;
	if(mailbox)
		event_data->event_param_data_1 = mailbox->name;
	event_data->event_param_data_3     = (char*)p;
	event_data->event_param_data_4     = mail_ids_count;

	THREAD_CREATE(delete_thread, thread_func_to_delete_mail, (void*)event_data, thread_error);
	THREAD_DETACH(delete_thread); /* free resources used for new thread */
	ret = true;

FINISH_OFF:
	if (ret == false)
		EM_SAFE_FREE(p);
	
	if (err_code)
		*err_code = err;
	
	return ret;
}

int emf_mail_delete_all(emf_mailbox_t* mailbox, int with_server, unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], with_server[%d], handle[%p], err_code[%p]", mailbox, with_server, handle, err_code);
	
	int            ret = false;
	int            err = EMF_ERROR_NONE;
	emf_account_t *ref_account = NULL;
	emf_event_t    event_data = { 0 };
	
	if (!mailbox || mailbox->account_id <= 0)  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	ref_account = emf_get_account_reference(mailbox->account_id);

	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed account_id [%d]", mailbox->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
	event_data.type               = EMF_EVENT_DELETE_MAIL_ALL;
	event_data.account_id         = mailbox->account_id;
	event_data.event_param_data_1 = EM_SAFE_STRDUP(mailbox->name);
	event_data.event_param_data_3 = NULL;
	event_data.event_param_data_4 = with_server;

	if (!em_core_insert_event(&event_data, (int*)handle, &err))  {
		EM_DEBUG_EXCEPTION("em_core_insert_event falied [%d]", err);
		EM_SAFE_FREE(event_data.event_param_data_1);
		goto FINISH_OFF;
	}
	
#ifdef __LOCAL_ACTIVITY__
	int i, total = 0 , search_handle = 0;
	int *mail_ids = NULL;
	emf_activity_tbl_t new_activity;
	int activityid = 0;
	
	if (false == em_core_get_next_activity_id(&activityid,&err)) {
		EM_DEBUG_EXCEPTION(" em_core_get_next_activity_id Failed - %d ", err);
	}
	
	if (!em_storage_mail_search_start(NULL, mailbox->account_id, mailbox->name, 0, &search_handle, &total, true, &err)) {
		EM_DEBUG_EXCEPTION(" em_storage_mail_search_start failed [%d]", err);
		
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	mail_ids = em_core_malloc(sizeof(int) * total);
	if (mail_ids == NULL)  {
		EM_DEBUG_EXCEPTION(" mailloc failed...");
		
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	for (i = 0; i < total; i++)  {
		if (!em_storage_mail_search_result(search_handle, RETRIEVE_ID, (void**)&mail_ids[i], true, &err))  {
			EM_DEBUG_EXCEPTION(" em_storage_mail_search_result failed [%d]", err);
			
			err = em_storage_get_emf_error_from_em_storage_error(err);
			EM_SAFE_FREE(mail_ids);
			goto FINISH_OFF;
		}
			
		new_activity.activity_id	= activityid;
		new_activity.activity_type	= ACTIVITY_DELETEMAIL;
		new_activity.mail_id		= mail_ids[i];
		new_activity.server_mailid	= NULL;
		new_activity.src_mbox		= mailbox->name;
		new_activity.dest_mbox		= NULL;
		new_activity.account_id 	= mailbox->account_id;
						
		if (! em_core_activity_add(&new_activity, &err))
			EM_DEBUG_EXCEPTION(" em_core_activity_add  Failed  - %d ", err);
		
	}

	EM_SAFE_FREE(mail_ids);

	EM_DEBUG_LOG("Setting g_local_activity_run ");
	g_local_activity_run = 1;	
#endif
	
#endif 	/*  __FEATURE_SYNC_CLIENT_TO_SERVER__ */
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;

	return ret;
}

	

EXPORT_API int emf_mail_move_all_mails(emf_mailbox_t* src_mailbox, emf_mailbox_t* dst_mailbox, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("src_mailbox[%p], dst_mailbox[%p], err_code[%p]",  src_mailbox, dst_mailbox, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_account_t *ref_account = NULL;
	emf_mail_list_item_t *mail_list = NULL;
	int select_num = 0;
	int *mails = NULL;	
	int i=0;
	int num =0;
	
	if (!dst_mailbox || dst_mailbox->account_id <= 0 || !src_mailbox || src_mailbox->account_id <= 0)  {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	ref_account = emf_get_account_reference(dst_mailbox->account_id);

	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", dst_mailbox->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if(!em_storage_get_mail_list(src_mailbox->account_id, src_mailbox->name, NULL, EMF_LIST_TYPE_NORMAL, -1, -1, 0, NULL, EMF_SORT_DATETIME_HIGH, false, &mail_list, &select_num, &err)) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_list failed");
		goto FINISH_OFF;
	}

	mails = malloc(sizeof(int) * select_num);

	if( !mails ) {
		EM_DEBUG_EXCEPTION("Malloc failed...!");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memset(mails, 0x00, sizeof(int) * select_num);
		
	for(i = 0 ; i < select_num ; i++) {
		if( mail_list[i].save_status != EMF_MAIL_STATUS_SENDING ) {
			mails[num] = mail_list[i].mail_id;
			num++;
		}
	}

	if( num <= 0) {
		EM_DEBUG_EXCEPTION("can't find avalable mails. num = %d", num);
		err = EMF_ERROR_MAIL_NOT_FOUND;
		goto FINISH_OFF;	
	}
	
	if(!em_core_mail_move(mails, num, dst_mailbox->name, EMF_MOVED_BY_COMMAND, 0, &err)) {
		EM_DEBUG_EXCEPTION("em_core_mail_move falied [%d]", err);
		goto FINISH_OFF;
	}

		
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;

	if(mail_list)
		EM_SAFE_FREE(mail_list);

	if(mails != NULL )
		EM_SAFE_FREE(mails);
	
	return ret;
}

void* thread_func_to_move_mail(void *thread_argument)
{
	EM_DEBUG_FUNC_BEGIN();
	int *mail_ids = NULL, mail_ids_count, noti_param_1, noti_param_2, err;
	unsigned handle = 0;
	emf_event_t *event_data = (emf_event_t*)thread_argument;
	char *dst_mailbox_name = NULL;

	dst_mailbox_name   = (char*)event_data->event_param_data_1;
	mail_ids           = (int*)event_data->event_param_data_3;
	mail_ids_count     = event_data->event_param_data_4;
	noti_param_1       = event_data->event_param_data_6;
	noti_param_2       = event_data->event_param_data_7;

	if (!em_core_mail_move(mail_ids, mail_ids_count, dst_mailbox_name, noti_param_1, noti_param_2, &err)) {
		EM_DEBUG_EXCEPTION("em_core_mail_move failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!em_core_insert_event(event_data, (int*)&handle, &err)) {
		EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:	
	/* em_core_free_event(event_data); */ /* all of members will be freed after using in each event handler */
	EM_SAFE_FREE(event_data);

	EM_DEBUG_FUNC_END();
	return SUCCESS;
}

EXPORT_API int emf_mail_move(int mail_ids[], int num, emf_mailbox_t* dst_mailbox, int noti_param_1, int noti_param_2, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], num[%d], dst_mailbox[%p], err_code[%p]", mail_ids, num, dst_mailbox, err_code);
	
	/*  default variable */
	int mail_id = 0, *p = NULL, thread_error;
	int ret = false, err = EMF_ERROR_NONE;
	char *src_mailbox_name = NULL;		
	emf_mail_tbl_t* mail_table_data = NULL;
	emf_account_t* ref_account = NULL;
	emf_event_t *event_data = NULL;
	thread_t move_thread;

	if (num <= 0 || !dst_mailbox || dst_mailbox->account_id <= 0) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	ref_account = emf_get_account_reference(dst_mailbox->account_id);

	if (!ref_account) {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", dst_mailbox->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	/* Getting source mailbox name */
	mail_id = mail_ids[0];

	if (!em_storage_get_mail_field_by_id(mail_id, RETRIEVE_SUMMARY, &mail_table_data, true, &err) || !mail_table_data) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_field_by_id failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	if (src_mailbox_name ==  NULL)
		src_mailbox_name = EM_SAFE_STRDUP(mail_table_data->mailbox_name);		

	em_storage_free_mail(&mail_table_data, 1, NULL);

	if ((event_data = em_core_malloc(sizeof(emf_event_t)) ) == NULL) {
		EM_DEBUG_EXCEPTION("em_core_malloc for event_data failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if ((p = em_core_malloc(sizeof(int) * num)) == NULL) {
		EM_DEBUG_EXCEPTION("em_core_malloc for p failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	memcpy(p, mail_ids, sizeof(int) * num);
	
	event_data->account_id        = dst_mailbox->account_id;
	event_data->type               = EMF_EVENT_MOVE_MAIL;
	event_data->event_param_data_1 = EM_SAFE_STRDUP(dst_mailbox->name);
	event_data->event_param_data_2 = EM_SAFE_STRDUP(src_mailbox_name);
	event_data->event_param_data_3 = (char*)p;
	event_data->event_param_data_4 = num;
	event_data->event_param_data_6 = noti_param_1;
	event_data->event_param_data_7 = noti_param_2;

#ifdef __LOCAL_ACTIVITY__
	int i = 0, activityid = 0;
	
	if (false == em_core_get_next_activity_id(&activityid,&err))
		EM_DEBUG_EXCEPTION(" em_core_get_next_activity_id Failed - %d ", err);
	
	for (i =0; i < event_data.event_param_data_4; i++) {
		emf_activity_tbl_t	new_activity;
		new_activity.activity_id = activityid;
		new_activity.activity_type = ACTIVITY_MOVEMAIL;
		new_activity.account_id    = event_data.account_id;
		new_activity.mail_id	   = mail_ids[i];
		new_activity.dest_mbox	   = event_data.event_param_data_1;
		new_activity.server_mailid = NULL;
		new_activity.src_mbox	   = event_data.event_param_data_2;

		if (!em_core_activity_add(&new_activity, &err))
			EM_DEBUG_EXCEPTION(" em_core_activity_add Failed - %d ", err);
	}
#endif /* __LOCAL_ACTIVITY__ */
	THREAD_CREATE(move_thread, thread_func_to_move_mail, (void*)event_data, thread_error);
	THREAD_DETACH(move_thread); /* free resources used for new thread */
	ret = true;
	
FINISH_OFF:

#ifdef __LOCAL_ACTIVITY__	
	EM_DEBUG_LOG("Setting g_local_activity_run ");
	g_local_activity_run = 1;	
#endif /* __LOCAL_ACTIVITY__ */

	if (err_code)
		*err_code = err;

	EM_SAFE_FREE(src_mailbox_name);
	
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}

EXPORT_API int emf_mail_modify_flag(int mail_id, emf_mail_flag_t new_flag, int onserver, int sticky_flag, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], new_flag[%d], onserver[%d], sticky_flag[%d], err_code[%p]", mail_id, new_flag, onserver, sticky_flag, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!em_core_mail_modify_flag(mail_id, new_flag, sticky_flag, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_mail_modify_flag falled [%d]", err);
		goto FINISH_OFF;
	}
	
#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
	if( onserver == 1)  {
       emf_event_t event_data;
       unsigned handle = 0;
       memset(&event_data, 0x00, sizeof(emf_event_t));
       
       event_data.type = EMF_EVENT_SYNC_MAIL_FLAG_TO_SERVER;
       event_data.event_param_data_1 = NULL;
       event_data.event_param_data_4 = mail_id;
       event_data.account_id = 0;      
#ifdef __LOCAL_ACTIVITY__
		emf_mail_tbl_t *mail_table_data = NULL;
		if (!em_storage_get_mail_field_by_id(mail_id, RETRIEVE_SUMMARY, &mail_table_data, false, &err) || !mail_table_data)  {
			EM_DEBUG_LOG(" em_storage_get_mail_field_by_id failed [%d]", err);
			goto FINISH_OFF;
		}
		event_data.account_id = mail_table_data->account_id;
		em_storage_free_mail(&mail_table_data,1,&err);

		emf_activity_tbl_t	new_activity;
		new_activity.activity_type = ACTIVITY_MODIFYFLAG;
		new_activity.account_id    = event_data.account_id;
		new_activity.mail_id	   = event_data.event_param_data_4;
		new_activity.dest_mbox	   = NULL;
		new_activity.server_mailid = NULL;
		new_activity.src_mbox	   = NULL;

		if (false == em_core_get_next_activity_id(&new_activity.activity_id,&err)) {
			EM_DEBUG_EXCEPTION(" em_core_get_next_activity_id Failed - %d \n", err);
		}

		if (!em_core_activity_add(&new_activity, &err)) {
			EM_DEBUG_EXCEPTION(" em_core_activity_add [ACTIVITY_MODIFYFLAG] Failed - %d \n", err);
		}
		
#endif /*  __LOCAL_ACTIVITY__ */
       if (!em_core_insert_event(&event_data, (int*)&handle, &err)) {
			EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
			goto FINISH_OFF;
       }

#ifdef __LOCAL_ACTIVITY__	
		EM_DEBUG_LOG("Setting g_local_activity_run ");
		g_local_activity_run = 1;	
#endif /*  __LOCAL_ACTIVITY__ */
	}
#endif /*  __FEATURE_SYNC_CLIENT_TO_SERVER__ */

	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_mail_modify_extra_flag(int mail_id, emf_extra_flag_t new_flag, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], err_code[%p]", mail_id, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
		
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
			goto FINISH_OFF;
		}

	if (!em_core_mail_modify_extra_flag(mail_id, new_flag, &err))  {
		EM_DEBUG_EXCEPTION("engine_mail_modify_extra_flag failed [%d]", err);
			goto FINISH_OFF;
		}
		
	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
		}

EXPORT_API int emf_mail_set_flags_field(int account_id, int mail_ids[], int num, emf_flags_field_type field_type, int value, int onserver, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%d], num[%d], field_type [%d], value[%d], err_code[%p]", mail_ids[0], num, field_type, value, err_code);
		
	int ret = false, err = EMF_ERROR_NONE;

	if(account_id <= 0 || !mail_ids || num <= 0) {
		err = EMF_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
			goto FINISH_OFF;
		}

	if (!em_core_mail_set_flags_field(account_id, mail_ids, num, field_type, value, &err))  {
		EM_DEBUG_EXCEPTION("em_core_mail_set_flags_field falled [%d]", err);
		goto FINISH_OFF;
	}

	if( onserver )  {
		int *mail_id_array = NULL;
		emf_event_t event_data = {0};
		unsigned handle = 0;

		mail_id_array = em_core_malloc(sizeof(int) * num);
	       
		if (mail_id_array == NULL)  {
			EM_DEBUG_EXCEPTION("em_core_malloc failed...");
	       err = EMF_ERROR_OUT_OF_MEMORY;
	       goto FINISH_OFF;
		}

		memcpy(mail_id_array, mail_ids, sizeof(int) * num);
		
		event_data.type               = EMF_EVENT_SYNC_FLAGS_FIELD_TO_SERVER;
		event_data.account_id         = account_id;
		event_data.event_param_data_1 = NULL;
		event_data.event_param_data_3 = (char*)mail_id_array;
		event_data.event_param_data_4  = num;
		event_data.event_param_data_5 = field_type;
		event_data.event_param_data_6 = value;

		if (!em_core_insert_event(&event_data, (int*)&handle, &err))  {
			EM_DEBUG_LOG("em_core_insert_event failed [%d]", err);
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

	

EXPORT_API int emf_mail_update(int mail_id, emf_mail_t* mail, emf_meeting_request_t *meeting_req, int with_server, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], mail[%p], with_server[%d], err_code[%p]", mail_id, mail, with_server, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_mail_tbl_t* mail_table_data = NULL;	
	int handle= 0;
	emf_event_t event_data;	
	char* p_mail = NULL;	
	
	if (!mail || !mail->info)  {
		EM_DEBUG_EXCEPTION("mail_id[%d], mali[%p]", mail_id, mail);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if (mail->body && mail->body->attachment)  {
		emf_attachment_info_t* p = mail->body->attachment;
		struct stat st_buf;
		
		while (p)  {
			if(p->downloaded) {
				if (!p->savename || stat(p->savename, &st_buf) < 0)  {
					EM_DEBUG_EXCEPTION("stat(\"%s\" failed...", p->savename);
					err = EMF_ERROR_INVALID_PARAM;
					goto FINISH_OFF;
				}
			}
			p = p->next;
		}
	}
	
	emf_account_t* ref_account = emf_get_account_reference(mail->info->account_id);
	
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", mail->info->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;	
		goto FINISH_OFF;
	}

	if( with_server && ref_account->receiving_server_type == EMF_SERVER_TYPE_IMAP4) {
		if (!em_storage_get_mail_field_by_id(mail_id, RETRIEVE_SUMMARY, &mail_table_data, false, &err)) 
			EM_DEBUG_LOG("em_storage_get_mail_field_by_id failed [%d]", err);

		p_mail= malloc(sizeof(int));
 		if (p_mail == NULL)  {
			EM_DEBUG_EXCEPTION("malloc failed...");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
 		memcpy(p_mail, &mail_id, sizeof(int));

 		memset(&event_data, 0x00, sizeof(emf_event_t));
 		event_data.type = EMF_EVENT_DELETE_MAIL;
 		event_data.account_id  = mail->info->account_id;
		event_data.event_param_data_3  = (char*)p_mail;
 		event_data.event_param_data_4  = 1;
		event_data.event_param_data_1 = EM_SAFE_STRDUP(mail_table_data->mailbox_name);

#ifdef __LOCAL_ACTIVITY__
		emf_activity_tbl_t new_activity;	
		EM_DEBUG_LOG(" mail_id - %d\n", mail_id);
		new_activity.activity_type	= ACTIVITY_DELETEMAIL_SEND;			
		new_activity.account_id    = event_data.account_id;			
		new_activity.mail_id		= mail_id;
		new_activity.server_mailid  = NULL;
		new_activity.src_mbox		= EM_SAFE_STRDUP(mail_table_data->mailbox);
		new_activity.dest_mbox		= NULL;
						
		if (false == em_core_get_next_activity_id(&new_activity.activity_id,&err)) {
			EM_DEBUG_EXCEPTION(" em_core_get_next_activity_id Failed - %d \n", err);
		}
		
		if (! em_core_activity_add(&new_activity, &err))
			EM_DEBUG_EXCEPTION(" em_core_activity_add  Failed  - %d \n", err);
		else
			EM_SAFE_FREE(new_activity.src_mbox); /* Memory Leak Fix */

#endif /* __LOCAL_ACTIVITY__ */		
		if (!em_core_insert_send_event(&event_data, &handle, &err))  {
			EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
			goto FINISH_OFF;
		}

#ifdef __LOCAL_ACTIVITY__
		EM_DEBUG_LOG("Setting g_save_local_activity_run ");
		g_save_local_activity_run = 1;
#endif
	}

	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE: 
	 		memset(&event_data, 0x00, sizeof(emf_event_t));
	 		event_data.type = EMF_EVENT_UPDATE_MAIL_OLD;
	 		event_data.account_id  = mail->info->account_id;
			event_data.event_param_data_3  = NULL;
	 		event_data.event_param_data_4  = mail_id;
			event_data.event_param_data_1 = (char*)mail;				/*  will be freed in event handler */
			event_data.event_param_data_2 = (char*)meeting_req;		/*  will be freed in event handler */

#ifdef __LOCAL_ACTIVITY__
			if( with_server ) {
				emf_activity_tbl_t new_activity;	

				new_activity.activity_type = ACTIVITY_SAVEMAIL;
				new_activity.account_id    = event_data.account_id;
				new_activity.mail_id	   = event_data.event_param_data_4;
				new_activity.dest_mbox	   = NULL;
				new_activity.server_mailid = NULL;
				new_activity.src_mbox	   = NULL;

				if (false == em_core_get_next_activity_id(&new_activity.activity_id,&err)) {
					EM_DEBUG_EXCEPTION(" em_core_get_next_activity_id Failed - %d \n", err);
				}

				if (!em_core_activity_add(&new_activity, &err)) {
					EM_DEBUG_EXCEPTION(" em_core_activity_add Failed - %d \n", err);
				}
			}
#endif /* __LOCAL_ACTIVITY__ */

			if (!em_core_insert_event(&event_data, &handle, &err))  {
				EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
				goto FINISH_OFF;
			}

			#ifdef __LOCAL_ACTIVITY__
			EM_DEBUG_LOG("Setting g_save_local_activity_run ");
			g_save_local_activity_run = 1;
			#endif
			
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

EXPORT_API int emf_update_mail(emf_mail_data_t *input_mail_data, emf_attachment_data_t *input_attachment_data_list, int input_attachment_count, emf_meeting_request_t *input_meeting_request, int input_sync_server)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count [%d], input_meeting_req [%p], input_sync_server[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_sync_server);

	int            err = EMF_ERROR_NONE;
	int            handle = 0;
	emf_event_t    event_data = { 0 };
	emf_account_t *ref_account = NULL;
	
	if (!input_mail_data || input_mail_data->account_id <= 0 || input_mail_data->mail_id == 0 ||
		(input_mail_data->report_status == EMF_MAIL_REPORT_MDN && !input_mail_data->full_address_to))  {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM"); 
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	ref_account = emf_get_account_reference(input_mail_data->account_id);
	if (!ref_account)  {
		EM_DEBUG_LOG(" emf_get_account_reference failed [%d]", input_mail_data->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE : 
			if ( (err = em_core_update_mail(input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_sync_server)) != EMF_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("em_core_update_mail failed [%d]", err);
				goto FINISH_OFF;
			}

#ifdef __FEATURE_SYNC_CLIENT_TO_SERVER__
			if ( input_sync_server ) {	
				event_data.type               = EMF_EVENT_UPDATE_MAIL;
				event_data.account_id         = input_mail_data->account_id;
				event_data.event_param_data_1 = (char*)input_mail_data;
				event_data.event_param_data_2 = (char*)input_attachment_data_list;
				event_data.event_param_data_3 = (char*)input_meeting_request;
				event_data.event_param_data_4 = input_attachment_count;
				event_data.event_param_data_5 = input_sync_server;

				if (!em_core_insert_send_event(&event_data, &handle, &err))  {
					EM_DEBUG_EXCEPTION("em_core_insert_send_event failed [%d]", err);
					err = EMF_ERROR_NONE;
					goto FINISH_OFF;
				}
			}
#endif			
		break;

		default:
			EM_DEBUG_EXCEPTION("unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
	}
	
FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


int emf_mail_check_mail_id(int mail_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], err_code[%p]", mail_id, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	emf_mail_tbl_t* mail = NULL;
	
	if (!em_storage_get_mail_field_by_id(mail_id, RETRIEVE_SUMMARY, &mail, true, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_field_by_id failed [%d]", err);
		
		if (err == EM_STORAGE_ERROR_MAIL_NOT_FOUND)
			err = EMF_ERROR_MAIL_NOT_FOUND;
		else if (err == EM_STORAGE_ERROR_OUT_OF_MEMORY)
			err = EMF_ERROR_OUT_OF_MEMORY;
		else if (err == EM_STORAGE_ERROR_DB_FAILURE)
			err = EMF_ERROR_DB_FAILURE;
		else if (err == EM_STORAGE_ERROR_INVALID_PARAM)
			err = EMF_ERROR_INVALID_PARAM;
		else
			err = EMF_ERROR_UNKNOWN;
		
		goto FINISH_OFF;
	}
	
	ret = true;

FINISH_OFF:
	if (mail != NULL)
		em_storage_free_mail(&mail, 1, NULL);
	
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


EXPORT_API int emf_mail_send_retry( int mail_id,  int timeout_in_sec, int* err_code)
{
	int ret = false;
	int err = EMF_ERROR_NONE;
	long nTimerValue = 0;
	emf_mailbox_t mailbox;
	emf_mail_t*     mail = NULL;
	emf_mail_tbl_t* mail_table_data = NULL;
	char mailId[10];
	emf_option_t opt;
	
	memset( &opt, 0x00, sizeof(emf_option_t));
	memset( &mailbox, 0x00, sizeof(emf_mailbox_t));
	memset(mailId, 0x00, 10);

	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}	

	if( ! em_storage_get_mail_by_id(mail_id, &mail_table_data, false, &err) && !mail_table_data)  {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;		
	}
	
	if (!em_core_mail_get_mail(mail_id, &mail, &err) && !mail) {
		EM_DEBUG_EXCEPTION("em_core_mail_get_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	
	mailbox.name = EM_SAFE_STRDUP(mail_table_data->mailbox_name);
	memcpy(&opt, em_core_get_option(&err), sizeof(emf_mail_t));
	if( mail && mail->info ) {
		mailbox.account_id = mail->info->account_id;
		opt.priority = mail->info->extra_flags.priority;
	}
	
	if ( timeout_in_sec == 0 ) {
		if(!emf_mail_send(&mailbox, mail_id, &opt, NULL, &err)) {
			EM_DEBUG_EXCEPTION("emf_mail_send failed [%d]", err);
			goto FINISH_OFF;
		}			
	}
	else if ( timeout_in_sec > 0 ) {
		sprintf(mailId,"%d",mail_id);
		nTimerValue = timeout_in_sec * 1000;
		if ( em_core_set_timer_ex(nTimerValue, (EMF_TIMER_CALLBACK) _OnMailSendRetryTimerCB, (void*)mailId) <= 0) {
			EM_DEBUG_EXCEPTION("emf_mail_send_retry - Failed to start timer");
			goto FINISH_OFF;
		}
	}
	ret = true;
FINISH_OFF:
	if (err_code)
		*err_code = err;
	if (mail)  em_core_mail_free(&mail, 1, NULL);
	if(mail_table_data) em_storage_free_mail(&mail_table_data, 1, NULL);
	EM_SAFE_FREE(mailbox.name);
	
	return ret;
}


EXPORT_API 
void _OnMailSendRetryTimerCB( void* data )
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	emf_mailbox_t mailbox;
	emf_mail_t*     mail = NULL;
	emf_mail_tbl_t* mail_table_data = NULL;	
	int mail_id = 0 ;
	emf_option_t opt;

	memset( &opt, 0x00, sizeof(emf_option_t));
	memset( &mailbox, 0x00, sizeof(emf_mailbox_t));

	if( !data ) {
		EM_DEBUG_LOG("Invalid param");
		goto FINISH_OFF;
	}

	mail_id = atoi((char*)data);
	

	if (!emf_mail_check_mail_id(mail_id, &err))  {
		EM_DEBUG_EXCEPTION("emf_mail_check_mail_id failed [%d]", err);
		goto FINISH_OFF;
	}	

	if (!em_core_mail_get_mail(mail_id, &mail, &err) && !mail) {
		EM_DEBUG_EXCEPTION("em_core_mail_get_mail failed [%d]", err);
		goto FINISH_OFF;
	}
	
	if( ! em_storage_get_mail_by_id(mail_id, &mail_table_data, false, &err) && !mail_table_data) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;		
	}

	mailbox.name = EM_SAFE_STRDUP(mail_table_data->mailbox_name);
	memcpy(&opt, em_core_get_option(&err), sizeof(emf_mail_t));

	if( mail && mail->info ) {
		opt.priority = mail->info->extra_flags.priority;
		mailbox.account_id = mail->info->account_id;
	}
	
	if(!emf_mail_send(&mailbox, mail_id, &opt, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emf_mail_send failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	if (mail)  em_core_mail_free(&mail, 1, NULL);
	if(mail_table_data) em_storage_free_mail(&mail_table_data, 1, NULL);	
	EM_SAFE_FREE(mailbox.name);
	
	EM_DEBUG_FUNC_END();
	return;
}

EXPORT_API int emf_mail_move_thread_to_mailbox(int thread_id, char *target_mailbox_name, int move_always_flag, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("thread_id [%d], target_mailbox_name [%p], move_always_flag [%d], err_code [%p]", thread_id, target_mailbox_name, move_always_flag, err_code);
	int ret = false;
	int err = EMF_ERROR_NONE;
	int *mail_id_list = NULL, result_count = 0, i, mailbox_count = 0, account_id;
	emf_mail_list_item_t *mail_list = NULL;
	emf_mailbox_t *target_mailbox_list = NULL, *target_mailbox = NULL;

	if (!target_mailbox_name) {
		EM_DEBUG_EXCEPTION("target_mailbox [%p]", target_mailbox_name);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!em_storage_get_mail_list(0, NULL, NULL, thread_id, -1, -1, 0, NULL, EMF_SORT_DATETIME_HIGH, true, &mail_list, &result_count, &err) || !mail_list || !result_count) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_list failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	mail_id_list = em_core_malloc(sizeof(int) * result_count);
	
	if (mail_id_list == NULL) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < result_count; i++) {
		mail_id_list[i] = mail_list[i].mail_id;
	}
	account_id = mail_list[0].account_id;


	if (!em_core_mailbox_get_list(account_id, &target_mailbox_list, &mailbox_count, &err)) {
		EM_DEBUG_EXCEPTION("em_core_mailbox_get_list failed [%d]", err);
		goto FINISH_OFF;
	}

	for(i = 0; i < mailbox_count; i++) {
		EM_DEBUG_LOG("%s %s", target_mailbox_list[i].name, target_mailbox_name);
		if(strcmp(target_mailbox_list[i].name, target_mailbox_name) == 0) {
			target_mailbox = (target_mailbox_list + i);
			break;
		}
	}

	if(!target_mailbox) {
		EM_DEBUG_EXCEPTION("couldn't find proper target mailbox.");
		goto FINISH_OFF;
	}

	if (!emf_mail_move(mail_id_list, result_count, target_mailbox, EMF_MOVED_BY_MOVING_THREAD, move_always_flag, &err)) {
		EM_DEBUG_EXCEPTION("emf_mail_move failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!em_storage_notify_storage_event(NOTI_THREAD_MOVE, account_id, thread_id, target_mailbox_name, move_always_flag)) 
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event Failed [NOTI_MAIL_MOVE] >>>> ");

	ret = true;
	
FINISH_OFF:
	if (!em_core_mailbox_free(&target_mailbox_list, mailbox_count, &err)) 
		EM_DEBUG_EXCEPTION("target_mailbox_list failed [%d]", err);
	EM_SAFE_FREE(mail_list);
	EM_SAFE_FREE(mail_id_list);

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_mail_delete_thread(int thread_id, int delete_always_flag, unsigned* handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("thread_id [%d], delete_always_flag [%d], err_code [%p]", thread_id, delete_always_flag, err_code);
	int ret = false;
	int err = EMF_ERROR_NONE;
	int account_id, *mail_id_list = NULL, result_count = 0, i;
	emf_mail_list_item_t *mail_list = NULL;

	if (!em_storage_get_mail_list(0, NULL, NULL, thread_id, -1, -1, 0, NULL, EMF_SORT_MAILBOX_NAME_HIGH, true, &mail_list, &result_count, &err) || !mail_list || !result_count) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_list failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	mail_id_list = em_core_malloc(sizeof(int) * result_count);
	
	if (mail_id_list == NULL) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < result_count; i++) {
		mail_id_list[i] = mail_list[i].mail_id;
	}

	account_id = mail_list[0].account_id;

	// should remove requiring of mailbox information from this function. 
	// email-service should find mailboxes itself by its mail id.
	if (!emf_mail_delete(account_id, NULL, mail_id_list, result_count, false, handle, &err)) {
		EM_DEBUG_EXCEPTION("emf_mail_delete failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!em_storage_notify_storage_event(NOTI_THREAD_DELETE, account_id, thread_id, NULL, delete_always_flag)) 
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event Failed [NOTI_MAIL_MOVE] >>>> ");

	ret = true;
	
FINISH_OFF:
	EM_SAFE_FREE(mail_list);
	EM_SAFE_FREE(mail_id_list);

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_mail_modify_seen_flag_of_thread(int thread_id, int seen_flag, int on_server, unsigned* handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("thread_id [%d], seen_flag [%d], on_server [%d], handle [%p], err_code [%p]", thread_id, seen_flag, on_server, handle, err_code);
	int ret = false;
	int err = EMF_ERROR_NONE;
	int account_id, *mail_id_list = NULL, result_count = 0, i;
	emf_mail_list_item_t *mail_list = NULL;

	if (!em_storage_get_mail_list(0, NULL, NULL, thread_id, -1, -1, 0, NULL, EMF_SORT_MAILBOX_NAME_HIGH, true, &mail_list, &result_count, &err) || !mail_list || !result_count) {
		EM_DEBUG_EXCEPTION("em_storage_get_mail_list failed [%d]", err);
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}
	
	mail_id_list = em_core_malloc(sizeof(int) * result_count);
	
	if (mail_id_list == NULL) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for(i = 0; i < result_count; i++) {
		mail_id_list[i] = mail_list[i].mail_id;
	}

	account_id = mail_list[0].account_id;

	if (!emf_mail_set_flags_field(account_id, mail_id_list, result_count, EMF_FLAGS_SEEN_FIELD, seen_flag, on_server, &err)) {
		EM_DEBUG_EXCEPTION("emf_mail_set_flags_field failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!em_storage_notify_storage_event(NOTI_THREAD_MODIFY_SEEN_FLAG, account_id, thread_id, NULL, seen_flag)) 
		EM_DEBUG_EXCEPTION(" em_storage_notify_storage_event Failed [NOTI_MAIL_MOVE] >>>> ");

	ret = true;
	
FINISH_OFF:
	EM_SAFE_FREE(mail_list);
	EM_SAFE_FREE(mail_id_list);

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}
