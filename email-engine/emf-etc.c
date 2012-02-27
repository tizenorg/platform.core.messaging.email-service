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
 * File: emf-etc.c
 * Desc: Mail Framework Etc Implementations
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
#include "emflib.h"
#include "emf-account.h"
#include "emf-dbglog.h"
#include "emf-global.h"
#include "em-core-types.h"
#include "em-core-event.h"
#include "em-core-utils.h"
#include "em-storage.h"


int emf_register_event_callback(emf_action_t action, emf_event_callback callback, void* event_data)
{
	return em_core_register_event_callback(action, callback, event_data);
}

int emf_unregister_event_callback(emf_action_t action, emf_event_callback callback)
{
	return em_core_unregister_event_callback(action, callback);
}


EXPORT_API void emf_get_event_queue_status(int* on_sending, int* on_receiving)
{
	em_core_get_event_queue_status(on_sending, on_receiving);
}

EXPORT_API int emf_get_pending_job(emf_action_t action, int account_id, int mail_id, emf_event_status_type_t* status)
{
	EM_DEBUG_FUNC_BEGIN("action[%d], account_id[%d], mail_id[%d]", action, account_id, mail_id);

	return em_core_get_pending_event(action, account_id, mail_id, status);
}

EXPORT_API int emf_cancel_job(int account_id, int handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], handle[%d], err_code[%p]", account_id, handle, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	
	if (!em_core_cancel_thread(handle, NULL, &err))  {
		EM_DEBUG_EXCEPTION("em_core_cancel_thread failed [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}


EXPORT_API int emf_mail_send_mail_cancel_job(int account_id, int mail_id, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mail_id[%d], err_code[%p]", account_id, mail_id, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;	
	int handle = 0;
	emf_account_t* ref_account = NULL;
	
	if (account_id <= 0)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mail_id[%d]", account_id, mail_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

#ifdef __FEATURE_PROGRESS_IN_OUTBOX__

	/*	h.gahlaut@samsung.com: Moved this code from email_cancel_send_mail API to email-service engine
		since this code has update DB operation which is failing in context of email application process 
		with an sqlite error -> sqlite3_step fail:8 */
		
	/* 	which means #define SQLITE_READONLY   8 */  /* Attempt to write a readonly database */ 
	emf_mail_t* mail = NULL;

	if (!em_core_mail_get_mail(mail_id, &mail, &err))  {
		EM_DEBUG_EXCEPTION("em_core_mail_get_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	if (mail && mail->info) {
		if (mail->info->extra_flags.status == EMF_MAIL_STATUS_SEND_CANCELED) {
			EM_DEBUG_EXCEPTION(">>>> EMF_MAIL_STATUS_SEND_CANCELED Already set for Mail ID [ %d ]", mail_id);
			goto FINISH_OFF;
		}
		else {			
			mail->info->extra_flags.status = EMF_MAIL_STATUS_SEND_CANCELED;

			if(!em_core_mail_modify_extra_flag(mail_id, mail->info->extra_flags, &err)) {
				EM_DEBUG_EXCEPTION("Failed to modify extra flag [%d]",err);
				goto FINISH_OFF;
			}
		}
	}

#endif

	if(!em_core_get_handle_by_mailId_from_transaction_info(mail_id , &handle )) {
		EM_DEBUG_EXCEPTION("em_core_get_handle_by_mailId_from_transaction_info failed for mail_id[%d]", mail_id);
		err = EMF_ERROR_HANDLE_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!(ref_account = emf_get_account_reference(account_id)))  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE :
			if (!em_core_cancel_send_mail_thread(handle, NULL, &err))  {
				EM_DEBUG_EXCEPTION("em_core_cancel_send_mail_thread failed [%d]", err);
				goto FINISH_OFF;
			}
			break;

		default:
			EM_DEBUG_EXCEPTION("unknown account bind type...");
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
	}	
	
	if(!em_core_delete_transaction_info_by_mailId(mail_id))
		EM_DEBUG_EXCEPTION("em_core_delete_transaction_info_by_mailId failed for mail_id[%d]", mail_id);
	
	ret = true;
	
FINISH_OFF:
	if(err_code != NULL)
		*err_code = err;
	
#ifdef __FEATURE_PROGRESS_IN_OUTBOX__
	if(!em_core_mail_free(&mail, 1, &err))
		EM_DEBUG_EXCEPTION("em_core_mail_free Failed [%d ]", err);	

#endif
	EM_DEBUG_FUNC_END();
	return ret;
}	


EXPORT_API int
emf_clear_mail_data(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int error = EMF_ERROR_NONE;
	
	if (emf_init(&error)) {
		if (!em_storage_clear_mail_data(true, &error))
			EM_DEBUG_EXCEPTION("em_storage_clear_mail_data failed [%d]", error);
	}
	else {
		EM_DEBUG_EXCEPTION("emf_init failed [%d]", error);
		if (err_code)
			*err_code = error;
		return false;
	}

	em_core_check_unread_mail();

	ret = true;

	if (!em_storage_create_table(EMF_CREATE_DB_NORMAL, &error)) 
		EM_DEBUG_EXCEPTION("em_storage_create_table failed [%d]", error);
	
	emf_close(&error);
	
	if (err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
    return ret;
}

	
/* --------------------------------------------------------------------------------*/
