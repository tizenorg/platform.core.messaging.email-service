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
 * to interact with email-service.
 * @file		email_api_mailbox.c
 * @brief 		This file contains the data structures and interfaces of mailbox related Functionality provided by 
 *			email-service . 
 */
 
#include "email-api.h"
#include "string.h"
#include "email-convert.h"
#include "email-storage.h"
#include "email-core-utils.h"
#include "email-utilities.h"
#include "email-ipc.h"
#include "db-util.h"

/* API - Create a mailbox */

EXPORT_API int email_add_mailbox(emf_mailbox_t* new_mailbox, int on_server, unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN();
	
	char* local_mailbox_stream = NULL;
	int size = 0;
	int err = EMF_ERROR_NONE;
	EM_IF_NULL_RETURN_VALUE(new_mailbox, EMF_ERROR_INVALID_PARAM);

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_ADD_MAILBOX);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);
	
	local_mailbox_stream = em_convert_mailbox_to_byte_stream(new_mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(local_mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, local_mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_SAFE_FREE(local_mailbox_stream); 
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &on_server, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	EM_DEBUG_LOG(" >>>>> error VALUE [%d]", err);

	if(handle) {
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
		EM_DEBUG_LOG(" >>>>> Handle [%d]", *handle);
	} 	
	

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


/* API - Delete a mailbox */

EXPORT_API int email_delete_mailbox(emf_mailbox_t* mailbox, int on_server,  unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p]", mailbox);
	
	char* local_mailbox_stream = NULL;
	int err = EMF_ERROR_NONE;
	int size = 0;
	
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(mailbox->account_id, EMF_ERROR_INVALID_PARAM)	;

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_DELETE_MAILBOX);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	local_mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);
	
	EM_PROXY_IF_NULL_RETURN_VALUE(local_mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, local_mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_SAFE_FREE(local_mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}
	
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &on_server, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	EM_DEBUG_LOG("error VALUE [%d]", err);

	if(handle) {
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
		EM_DEBUG_LOG("Handle [%d]", handle);
	} 	

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

/* API - Update a mailbox */


EXPORT_API int email_update_mailbox(emf_mailbox_t* old_mailbox, emf_mailbox_t* new_mailbox)
{
	EM_DEBUG_FUNC_BEGIN();
	
	char* local_mailbox_stream = NULL;
	int err = EMF_ERROR_NONE;	
	int size = 0;
	int on_server = 0;
	EM_DEBUG_LOG(" old_mailbox[%p], new_mailbox[%p]", old_mailbox, new_mailbox);

	EM_IF_NULL_RETURN_VALUE(old_mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(old_mailbox->account_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(new_mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(new_mailbox->account_id, EMF_ERROR_INVALID_PARAM);
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_UPDATE_MAILBOX);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);


	local_mailbox_stream = em_convert_mailbox_to_byte_stream(old_mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(local_mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);
		
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, local_mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_SAFE_FREE(local_mailbox_stream); 
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}
	
	local_mailbox_stream = em_convert_mailbox_to_byte_stream(new_mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(local_mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, local_mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_SAFE_FREE(local_mailbox_stream); 
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &on_server, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	} 

 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
 	
	EM_DEBUG_LOG("error VALUE [%d]", err);
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


EXPORT_API int email_get_sync_mailbox_list(int account_id, emf_mailbox_t** mailbox_list, int* count)
{
	EM_DEBUG_FUNC_BEGIN();
	int counter = 0;
	emstorage_mailbox_tbl_t* mailbox = NULL; 
	int err = EMF_ERROR_NONE ;
	
	EM_IF_NULL_RETURN_VALUE(mailbox_list, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(account_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMF_ERROR_INVALID_PARAM);

	if (!emstorage_get_mailbox(account_id, 0, EMAIL_MAILBOX_SORT_BY_NAME_ASC, &counter, &mailbox, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox failed [%d]", err);


		goto FINISH_OFF;
	} else
		err = EMF_ERROR_NONE;
	
	if (counter > 0)  {
		if (!(*mailbox_list = em_malloc(sizeof(emf_mailbox_t) * counter)))  {
			EM_DEBUG_EXCEPTION("malloc failed...");
			err= EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	} else
		*mailbox_list = NULL;
	
	*count = counter;
	
	FINISH_OFF:
	if (mailbox != NULL)
		emstorage_free_mailbox(&mailbox, counter, NULL);
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


EXPORT_API int email_get_mailbox_list(int account_id, int mailbox_sync_type, emf_mailbox_t** mailbox_list, int* count)
{
	EM_DEBUG_FUNC_BEGIN();	

	int mailbox_count = 0;
	emstorage_mailbox_tbl_t* mailbox_tbl_list = NULL; 
	int err =EMF_ERROR_NONE;
	int i;
	
	EM_IF_NULL_RETURN_VALUE(mailbox_list, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(account_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMF_ERROR_INVALID_PARAM);

	if (!emstorage_get_mailbox(account_id, mailbox_sync_type, EMAIL_MAILBOX_SORT_BY_NAME_ASC, &mailbox_count, &mailbox_tbl_list, true, &err))  {	
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox failed [%d]", err);

		goto FINISH_OFF;
	} else
		err = EMF_ERROR_NONE;
	
	if (mailbox_count > 0)  {
		if (!(*mailbox_list = em_malloc(sizeof(emf_mailbox_t) * mailbox_count)))  {
			EM_DEBUG_EXCEPTION("malloc failed for mailbox_list");
			err= EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (i = 0; i < mailbox_count; i++)  {
			(*mailbox_list)[i].mailbox_id = mailbox_tbl_list[i].mailbox_id;
			(*mailbox_list)[i].account_id = account_id;
			(*mailbox_list)[i].name = mailbox_tbl_list[i].mailbox_name; 
			mailbox_tbl_list[i].mailbox_name = NULL;
			(*mailbox_list)[i].alias = mailbox_tbl_list[i].alias; 
			mailbox_tbl_list[i].alias = NULL;
			(*mailbox_list)[i].local = mailbox_tbl_list[i].local_yn;
			(*mailbox_list)[i].mailbox_type = mailbox_tbl_list[i].mailbox_type;
			(*mailbox_list)[i].synchronous = mailbox_tbl_list[i].sync_with_server_yn;
			(*mailbox_list)[i].unread_count = mailbox_tbl_list[i].unread_count;
			(*mailbox_list)[i].total_mail_count_on_local = mailbox_tbl_list[i].total_mail_count_on_local;
			(*mailbox_list)[i].total_mail_count_on_server = mailbox_tbl_list[i].total_mail_count_on_server;
			(*mailbox_list)[i].has_archived_mails = mailbox_tbl_list[i].has_archived_mails;
			(*mailbox_list)[i].mail_slot_size = mailbox_tbl_list[i].mail_slot_size;
		}
	} else
		*mailbox_list = NULL;
	
	*count = mailbox_count;

FINISH_OFF:
	if (mailbox_tbl_list != NULL)
		emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}



EXPORT_API int email_get_mailbox_list_ex(int account_id, int mailbox_sync_type, int with_count, emf_mailbox_t** mailbox_list, int* count)
{
	EM_DEBUG_FUNC_BEGIN();	

	int mailbox_count = 0;
	emstorage_mailbox_tbl_t* mailbox_tbl_list = NULL; 
	int err =EMF_ERROR_NONE;
	int i;
	
	EM_IF_NULL_RETURN_VALUE(mailbox_list, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(account_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMF_ERROR_INVALID_PARAM);

	if (!emstorage_get_mailbox_ex(account_id, mailbox_sync_type, with_count, &mailbox_count, &mailbox_tbl_list, true, &err))  {	
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_ex failed [%d]", err);

		goto FINISH_OFF;
	} else
		err = EMF_ERROR_NONE;
	
	if (mailbox_count > 0)  {
		if (!(*mailbox_list = em_malloc(sizeof(emf_mailbox_t) * mailbox_count)))  {
			EM_DEBUG_EXCEPTION("malloc failed for mailbox_list");
			err= EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (i = 0; i < mailbox_count; i++)  {
			(*mailbox_list)[i].mailbox_id = mailbox_tbl_list[i].mailbox_id;
			(*mailbox_list)[i].account_id = account_id;
			(*mailbox_list)[i].name = mailbox_tbl_list[i].mailbox_name; 
			mailbox_tbl_list[i].mailbox_name = NULL;
			(*mailbox_list)[i].alias = mailbox_tbl_list[i].alias; 
			mailbox_tbl_list[i].alias = NULL;
			(*mailbox_list)[i].local = mailbox_tbl_list[i].local_yn;
			(*mailbox_list)[i].mailbox_type = mailbox_tbl_list[i].mailbox_type;
			(*mailbox_list)[i].synchronous = mailbox_tbl_list[i].sync_with_server_yn;
			(*mailbox_list)[i].unread_count = mailbox_tbl_list[i].unread_count;
			(*mailbox_list)[i].total_mail_count_on_local = mailbox_tbl_list[i].total_mail_count_on_local;
			(*mailbox_list)[i].total_mail_count_on_server = mailbox_tbl_list[i].total_mail_count_on_server;
			(*mailbox_list)[i].has_archived_mails = mailbox_tbl_list[i].has_archived_mails;
			(*mailbox_list)[i].mail_slot_size = mailbox_tbl_list[i].mail_slot_size;
		}
	} else
		*mailbox_list = NULL;

	if ( count )	
		*count = mailbox_count;

FINISH_OFF:
	if (mailbox_tbl_list != NULL)
		emstorage_free_mailbox(&mailbox_tbl_list, mailbox_count, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


EXPORT_API int email_get_mailbox_by_name(int account_id, const char *pMailboxName, emf_mailbox_t **pMailbox)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMF_ERROR_NONE;
	emf_mailbox_t* curr_mailbox = NULL;
	emstorage_mailbox_tbl_t* local_mailbox = NULL;

	EM_IF_NULL_RETURN_VALUE(pMailbox, EMF_ERROR_INVALID_PARAM);
	if(!pMailboxName)
		return EMF_ERROR_INVALID_PARAM;	
	
	if (!emstorage_get_mailbox_by_name(account_id, -1, (char*)pMailboxName, &local_mailbox, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_name failed [%d]", err);
		return err;
	} else {
		curr_mailbox = em_malloc(sizeof(emf_mailbox_t));
		memset(curr_mailbox, 0x00, sizeof(emf_mailbox_t));

		curr_mailbox->account_id = local_mailbox->account_id;
		if(local_mailbox->mailbox_name)
			curr_mailbox->name = strdup(local_mailbox->mailbox_name);
		curr_mailbox->local = local_mailbox->local_yn;
		if(local_mailbox->alias)
			curr_mailbox->alias= strdup(local_mailbox->alias);
		curr_mailbox->synchronous = local_mailbox->sync_with_server_yn;
		curr_mailbox->mailbox_type = local_mailbox->mailbox_type;
		curr_mailbox->unread_count = local_mailbox->unread_count;
		curr_mailbox->total_mail_count_on_local= local_mailbox->total_mail_count_on_local;
		curr_mailbox->total_mail_count_on_server= local_mailbox->total_mail_count_on_server;
		curr_mailbox->has_archived_mails = local_mailbox->has_archived_mails;
		curr_mailbox->mail_slot_size = local_mailbox->mail_slot_size;
	}

	*pMailbox = curr_mailbox;

	emstorage_free_mailbox(&local_mailbox, 1, &err);
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


EXPORT_API int email_get_child_mailbox_list(int account_id, const char *parent_mailbox,  emf_mailbox_t** mailbox_list, int* count)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], parent_mailbox[%p], mailbox_list[%p], count[%p]", account_id, parent_mailbox, mailbox_list, count);
	
	int err = EMF_ERROR_NONE;
	int counter = 0;
	emstorage_mailbox_tbl_t* mailbox_tbl = NULL; 
	int i =0;
	
	EM_IF_NULL_RETURN_VALUE(account_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(mailbox_list, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(count, EMF_ERROR_INVALID_PARAM);
		
	*mailbox_list = NULL;
	*count = 0;
	
	if (!emstorage_get_child_mailbox_list(account_id,(char*)parent_mailbox, &counter, &mailbox_tbl, true, &err))   {
		EM_DEBUG_EXCEPTION("emstorage_get_child_mailbox_list failed[%d]", err);
		

		goto FINISH_OFF;
	} else
		err = EMF_ERROR_NONE;
	
	if (counter > 0)  {
		if (!(*mailbox_list = em_malloc(sizeof(emf_mailbox_t) * counter)))  {
			EM_DEBUG_EXCEPTION("malloc failed for mailbox_list");
			
			err= EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		memset(*mailbox_list, 0x00, (sizeof(emf_mailbox_t) * counter));
		
		for (i = 0; i < counter; i++)  {
			(*mailbox_list)[i].mailbox_id = mailbox_tbl[i].mailbox_id;
			(*mailbox_list)[i].account_id = account_id;
			(*mailbox_list)[i].name = mailbox_tbl[i].mailbox_name; mailbox_tbl[i].mailbox_name = NULL;
			(*mailbox_list)[i].alias = mailbox_tbl[i].alias; mailbox_tbl[i].alias = NULL;
			(*mailbox_list)[i].local = mailbox_tbl[i].local_yn;
			(*mailbox_list)[i].mailbox_type = mailbox_tbl[i].mailbox_type;
			(*mailbox_list)[i].synchronous = mailbox_tbl[i].sync_with_server_yn;
			(*mailbox_list)[i].unread_count = mailbox_tbl[i].unread_count;
			(*mailbox_list)[i].total_mail_count_on_local = mailbox_tbl[i].total_mail_count_on_local;
			(*mailbox_list)[i].total_mail_count_on_server = mailbox_tbl[i].total_mail_count_on_server;
			(*mailbox_list)[i].has_archived_mails = mailbox_tbl[i].has_archived_mails;
			(*mailbox_list)[i].mail_slot_size = mailbox_tbl[i].mail_slot_size;
		}
	} else
		*mailbox_list = NULL;
	
	*count = counter;

FINISH_OFF:
	if (mailbox_tbl != NULL)
		emstorage_free_mailbox(&mailbox_tbl, counter, NULL);
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

/* sowmya.kr, 05-Apr-2010, new MAPI*/

EXPORT_API int email_get_mailbox_by_mailbox_type(int account_id, emf_mailbox_type_e mailbox_type,  emf_mailbox_t** mailbox)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMF_ERROR_NONE;
	emf_mailbox_t* curr_mailbox = NULL;
	emstorage_mailbox_tbl_t* local_mailbox = NULL;

	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);	
	EM_IF_NULL_RETURN_VALUE(account_id, EMF_ERROR_INVALID_PARAM)	;


	if(mailbox_type < EMF_MAILBOX_TYPE_INBOX || mailbox_type > EMF_MAILBOX_TYPE_ALL_EMAILS)
		return EMF_ERROR_INVALID_PARAM;
	if (!emstorage_get_mailbox_by_mailbox_type(account_id, mailbox_type, &local_mailbox, true, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);

		goto FINISH_OFF;
	} else {
		err = EMF_ERROR_NONE;
		curr_mailbox = em_malloc(sizeof(emf_mailbox_t));
		memset(curr_mailbox, 0x00, sizeof(emf_mailbox_t));

		curr_mailbox->account_id = local_mailbox->account_id;
		if(local_mailbox->mailbox_name)
			curr_mailbox->name = strdup(local_mailbox->mailbox_name);
		curr_mailbox->local = local_mailbox->local_yn;
		if(local_mailbox->alias)
			curr_mailbox->alias= strdup(local_mailbox->alias);
		curr_mailbox->synchronous = 1;
		curr_mailbox->mailbox_type = local_mailbox->mailbox_type;
		curr_mailbox->unread_count = local_mailbox->unread_count;
		curr_mailbox->total_mail_count_on_local = local_mailbox->total_mail_count_on_local;
		curr_mailbox->total_mail_count_on_server = local_mailbox->total_mail_count_on_server;
		curr_mailbox->has_archived_mails = local_mailbox->has_archived_mails;
		curr_mailbox->mail_slot_size = local_mailbox->mail_slot_size;
	}

	*mailbox = curr_mailbox;	
FINISH_OFF:

	if(local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, NULL);
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_set_mail_slot_size(int account_id, char* mailbox_name, int new_slot_size/*, unsigned* handle*/)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], mailbox_name[%p], new_slot_size[%d]", account_id, mailbox_name, new_slot_size);

	int err = EMF_ERROR_NONE, *handle = NULL;

	if(new_slot_size < 0) {
		EM_DEBUG_EXCEPTION("new_slot_size should be greater than 0 or should be equal to 0");
		return EMF_ERROR_INVALID_PARAM;
	}
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SET_MAIL_SLOT_SIZE);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	if (hAPI) {
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for account_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &new_slot_size, sizeof(int))) {
			EM_DEBUG_EXCEPTION(" emipc_add_parameter for new_slot_size failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		if(mailbox_name) {
			if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_name, strlen(mailbox_name) )) {
				EM_DEBUG_EXCEPTION(" emipc_add_parameter for mailbox_name failed");
				EM_SAFE_FREE(mailbox_name);
				EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
			}
		}
		
		if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		} 
	
	 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		EM_DEBUG_LOG("email_set_mail_slot_size error VALUE [%d]", err);
		if(handle) {
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
			EM_DEBUG_LOG("email_set_mail_slot_size handle VALUE [%d]", handle);
		}
		emipc_destroy_email_api(hAPI);
		hAPI = NULL;
		
	}
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_free_mailbox(emf_mailbox_t** mailbox_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_list[%p], count[%d]", mailbox_list, count);
	int err = EMF_ERROR_NONE;

	if (count <= 0 || !mailbox_list || !*mailbox_list) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	emf_mailbox_t *p = *mailbox_list;
	int i;

	for (i = 0; i < count; i++)  {
		EM_SAFE_FREE(p[i].name);
		EM_SAFE_FREE(p[i].alias);
		EM_SAFE_FREE(p[i].account_name);
	}

	EM_SAFE_FREE(p);
	*mailbox_list = NULL;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;

}
