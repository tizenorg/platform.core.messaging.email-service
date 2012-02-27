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
 * File: emf-mailbox.c
 * Desc: Mail Framework Mailbox Operation
 *
 * Auth: 
 *
 * History:
 *    2006.08.16 : created
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emflib.h"
#include "emf-global.h"
#include "em-core-event.h"
#include "emf-account.h"
#include "emf-dbglog.h"
#include "em-core-mailbox.h"
#include "em-core-global.h"
#include "em-core-utils.h"

#ifdef __LOCAL_ACTIVITY__
extern int g_local_activity_run;
#endif

EXPORT_API int
emf_mailbox_get_imap_mailbox_list(int account_id, char* mailbox, unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d] mailbox[%p] err_code[%p]", account_id, mailbox, err_code);

	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;

	if (account_id <= 0 ||!mailbox)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox[%p]", account_id, mailbox);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	emf_account_t* ref_account = emf_get_account_reference(account_id);
	
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	emf_event_t event_data;
	memset(&event_data, 0x00, sizeof(emf_event_t));

	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE:
			event_data.type = EMF_EVENT_SYNC_IMAP_MAILBOX;
			event_data.account_id = account_id;
			event_data.event_param_data_3 = EM_SAFE_STRDUP(mailbox);

			if (!em_core_insert_event(&event_data, (int*)handle, &err))  { 
				EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
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


	

EXPORT_API int emf_mailbox_get_list(int account_id, emf_mailbox_t** mailbox_list, int* count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_list[%p], count[%p], err_code[%p]", account_id, mailbox_list, count, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (account_id <= 0 || !mailbox_list || !count)  {
		EM_DEBUG_EXCEPTION("account_id[%d], mailbox_list[%p], count[%p]", account_id, mailbox_list, count);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	emf_account_t* ref_account = emf_get_account_reference(account_id);
	
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE:
			if (!em_core_mailbox_get_list(account_id, mailbox_list, count, &err))  {
				EM_DEBUG_EXCEPTION("em_core_mailbox_get_list failed [%d]", err);
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


EXPORT_API int emf_mailbox_get_mail_count(emf_mailbox_t* mailbox, int* total, int* unseen, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], total[%p], unseen[%p], err_code[%p]", mailbox, total, unseen, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!mailbox || !total || !unseen)  {
		EM_DEBUG_EXCEPTION("mailbox[%p], total[%p], unseen[%p]", mailbox, total, unseen);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	emf_account_t* ref_account = emf_get_account_reference(mailbox->account_id);
	if (ref_account == NULL)  {	
		EM_DEBUG_EXCEPTION(" emf_get_account_reference failed [%d]", mailbox->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;		
		goto FINISH_OFF;
	}
	
	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE: 
			if (!em_core_mailbox_get_mail_count(mailbox, total, unseen, &err))  {
				EM_DEBUG_EXCEPTION("em_core_mailbox_get_mail_count failed [%d]", err);
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

EXPORT_API int emf_mailbox_create(emf_mailbox_t* new_mailbox, int on_server, unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("new_mailbox[%p], err_code[%p]", new_mailbox, err_code);
	
	int ret = false;;
	int err = EMF_ERROR_NONE;

	if (!new_mailbox || new_mailbox->account_id <= 0 || !new_mailbox->name)  {
		if (new_mailbox != NULL)
			EM_DEBUG_EXCEPTION("new_mailbox->account_id[%d], new_mailbox->name[%p]", new_mailbox->account_id, new_mailbox->name);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	emf_account_t* ref_account = emf_get_account_reference(new_mailbox->account_id);
	
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", new_mailbox->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	emf_event_t event_data;
	memset(&event_data, 0x00, sizeof(emf_event_t));
	
	switch (ref_account->account_bind_type)  {	
		case EMF_BIND_TYPE_EM_CORE:
			/*  on_server is allowed to be only 0 when server_type is EMF_SERVER_TYPE_ACTIVE_SYNC */
			if ( ref_account->receiving_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC )
				on_server = 0;

			if ( on_server ) {	/*  async */
				event_data.type = EMF_EVENT_CREATE_MAILBOX;
				event_data.account_id = new_mailbox->account_id;
				event_data.event_param_data_1 = EM_SAFE_STRDUP(new_mailbox->name);
				event_data.event_param_data_2 = EM_SAFE_STRDUP(new_mailbox->alias);
				event_data.event_param_data_4 = on_server;
				event_data.event_param_data_3 = GINT_TO_POINTER(new_mailbox->mailbox_type);
				if(!em_core_insert_event(&event_data, (int*)handle, &err))    {
					EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
					goto FINISH_OFF;
				}
			}
			else {	/*  sync */
				if (!em_core_mailbox_create(new_mailbox, on_server, &err))  {
					EM_DEBUG_EXCEPTION("em_core_mailbox_create failed [%d]", err);
					goto FINISH_OFF;
				}
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


EXPORT_API int emf_mailbox_update(emf_mailbox_t* old_mailbox, emf_mailbox_t* new_mailbox, int on_server, unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("old_mailbox[%p], new_mailbox[%p], on_server[%d], handle[%p], err_code[%p]", old_mailbox, new_mailbox, on_server, handle, err_code);
	
	/*  default variable */
	int ret = false;;
	int err = EMF_ERROR_NONE;
	
	if (!old_mailbox || old_mailbox->account_id <= 0 || !old_mailbox->name 
		|| !new_mailbox || new_mailbox->account_id <= 0 || !new_mailbox->name)  {
		EM_DEBUG_EXCEPTION("INVALID PARAM");
		if (old_mailbox != NULL)
			EM_DEBUG_EXCEPTION("old_mailbox->account_id[%d], old_mailbox->name[%p]", old_mailbox->account_id, old_mailbox->name);
		if (new_mailbox != NULL)
			EM_DEBUG_EXCEPTION("new_mailbox->account_id[%d], new_mailbox->name[%p]", new_mailbox->account_id, new_mailbox->name);
		
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	emf_account_t* ref_account = emf_get_account_reference(old_mailbox->account_id);
	
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", old_mailbox->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;		/*  instead of EMF_ERROR_INVALID_PARAM; */
		goto FINISH_OFF;
	}

	emf_event_t event_data;
	memset(&event_data, 0x00, sizeof(emf_event_t));
	
	switch (ref_account->account_bind_type)  {	
		case EMF_BIND_TYPE_EM_CORE:
			/* Unsupport sync with server */
			/*  on_server is allowed to be only 0 when server_type is EMF_SERVER_TYPE_ACTIVE_SYNC */
			/*
			if ( ref_account->receiving_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC ) {
				on_server = 0;
			}
			if ( on_server ) {	
				event_data.type = EMF_EVENT_UPDATE_MAILBOX;
				event_data.account_id = new_mailbox->account_id;
				event_data.event_param_data_1 = ;
				event_data.event_param_data_2 = ;
				event_data.event_param_data_4 = on_server;
				event_data.event_param_data_3 = GINT_TO_POINTER(new_mailbox->mailbox_type);
				if(!em_core_insert_event(&event_data, handle, &err))  {
					EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
					goto FINISH_OFF;
				}
			}
			else {	
				if (!em_core_mailbox_modify(old_mailbox, new_mailbox, on_server, &err))  {
					EM_DEBUG_EXCEPTION("em_core_mailbox_create failed [%d]", err);
					goto FINISH_OFF;
				}
			}
			*/
			/*  Update mailbox information only on local db */
			if (!em_core_mailbox_update(old_mailbox, new_mailbox, &err))  {
				EM_DEBUG_EXCEPTION("em_core_mailbox_modify failed [%d]", err);
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



EXPORT_API int emf_mailbox_delete(emf_mailbox_t* mailbox, int on_server, unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], err_code[%p]", mailbox, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!mailbox || mailbox->account_id <= 0)  {
		if (mailbox != NULL)
			EM_DEBUG_EXCEPTION("mailbox->account_id[%d]", mailbox->account_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	emf_account_t* ref_account = emf_get_account_reference(mailbox->account_id);
	

	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", mailbox->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	emf_event_t event_data;
	memset(&event_data, 0x00, sizeof(emf_event_t));
	
	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE: 
			/*  on_server is allowed to be only 0 when server_type is EMF_SERVER_TYPE_ACTIVE_SYNC */
			if ( ref_account->receiving_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC ) {
				on_server = 0;
			}
			if ( on_server ) {	/*  async */
				event_data.type = EMF_EVENT_DELETE_MAILBOX;
				event_data.account_id = mailbox->account_id;
				event_data.event_param_data_1 = EM_SAFE_STRDUP(mailbox->name);
				event_data.event_param_data_4 = on_server;
				if(!em_core_insert_event(&event_data, (int*)handle, &err))    {
					EM_DEBUG_EXCEPTION("em_core_insert_event failed [%d]", err);
					goto FINISH_OFF;
				}
			}

			else {
				if (!em_core_mailbox_delete(mailbox, on_server,  &err))  {
					EM_DEBUG_EXCEPTION("em_core_mailbox_delete failed [%d]", err);
					goto FINISH_OFF;
				}
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

EXPORT_API int
emf_mailbox_delete_all(emf_mailbox_t* mailbox, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("malibox[%p], err_code[%p]", mailbox, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!mailbox || mailbox->account_id <= 0)  {
		if (mailbox == NULL)
			EM_DEBUG_EXCEPTION("malibox->account_id[%d]", mailbox->account_id);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	emf_account_t* ref_account = emf_get_account_reference(mailbox->account_id);
	

	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", mailbox->account_id);
		err = EMF_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}
	
	switch (ref_account->account_bind_type)  {
		case EMF_BIND_TYPE_EM_CORE:
			if (!em_core_mailbox_delete_all(mailbox, &err))  {
				EM_DEBUG_EXCEPTION("em_core_mailbox_delete_all failed [%d]", err);
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

	

EXPORT_API int emf_mailbox_free(emf_mailbox_t** mailbox_list, int count, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	return em_core_mailbox_free(mailbox_list, count, err_code);
}
		

EXPORT_API int emf_mailbox_sync_header(emf_mailbox_t* mailbox,  unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], handle[%p], err_code[%p]", mailbox, handle, err_code);
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (!mailbox ) {
		EM_DEBUG_EXCEPTION("mailbox[%p]", mailbox);
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	emf_event_t event_data;
	emf_account_t* ref_account = NULL;
	
	memset(&event_data, 0x00, sizeof(emf_event_t));

	if(mailbox->account_id == ALL_ACCOUNT) {
		EM_DEBUG_LOG(">>>> emf_mailbox_sync_header for all account event_data.event_param_data_4 [%d]", event_data.event_param_data_4);
		event_data.type = EMF_EVENT_SYNC_HEADER;
		event_data.event_param_data_1 = mailbox ? EM_SAFE_STRDUP(mailbox->name) : NULL;
		event_data.event_param_data_3 = NULL;
		event_data.account_id = mailbox->account_id;
		/* In case of Mailbox NULL, we need to set arg as EMF_SYNC_ALL_MAILBOX */
		if (!event_data.event_param_data_1)
			event_data.event_param_data_4 = EMF_SYNC_ALL_MAILBOX;
		
		if (!em_core_insert_event(&event_data, (int*)handle, &err))   {
			EM_DEBUG_EXCEPTION("em_core_insert_event falied [%d]", err);
			goto FINISH_OFF;
		}
	}
	else {
	
		if (!(ref_account = emf_get_account_reference(mailbox->account_id))) {
			EM_DEBUG_EXCEPTION("emf_get_account_reference failed [%d]", mailbox->account_id);
			err = EMF_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
		}

		/* Modified the code to sync all mailbox in a Single Event */
		switch (ref_account->account_bind_type)  {
			case EMF_BIND_TYPE_EM_CORE:
				event_data.type = EMF_EVENT_SYNC_HEADER;
				event_data.event_param_data_1 = mailbox ? EM_SAFE_STRDUP(mailbox->name) : NULL;
				event_data.event_param_data_3 = NULL;
				event_data.account_id = mailbox->account_id;
				/* In case of Mailbox NULL, we need to set arg as EMF_SYNC_ALL_MAILBOX */
				if (!event_data.event_param_data_1)
					event_data.event_param_data_4 = EMF_SYNC_ALL_MAILBOX;
				EM_DEBUG_LOG(">>>> EVENT ARG [ %d ] ", event_data.event_param_data_4);
				
				if (!em_core_insert_event(&event_data, (int*)handle, &err))   {
					EM_DEBUG_EXCEPTION("em_core_insert_event falied [%d]", err);
					goto FINISH_OFF;
				}
				break;
				
			default:
				EM_DEBUG_EXCEPTION("unknown account bind type...");
				err = EMF_ERROR_INVALID_ACCOUNT;
				goto FINISH_OFF;
		}
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

EXPORT_API int
emf_mailbox_set_mail_slot_size(int account_id, char* mailbox_name, int new_slot_size, unsigned* handle, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mailbox_name[%p], handle[%p], err_code[%p]", account_id, mailbox_name, handle, err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	emf_event_t event_data;

	if(handle == NULL) {
		EM_DEBUG_EXCEPTION("handle is required");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	memset(&event_data, 0x00, sizeof(emf_event_t));

	event_data.type = EMF_EVENT_SET_MAIL_SLOT_SIZE;
	event_data.event_param_data_4 = new_slot_size;
	event_data.event_param_data_3 = EM_SAFE_STRDUP(mailbox_name);
	event_data.account_id = account_id;
	
	if (!em_core_insert_event(&event_data, (int*)handle, &err))   {
		EM_DEBUG_EXCEPTION("em_core_insert_event falied [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

