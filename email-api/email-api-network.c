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



/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-network.c
 * @brief 		This file contains the data structures and interfaces of Network related Functionality provided by 
 *			email-service . 
 */
 
#include "email-api.h"
#include "string.h"
#include "email-convert.h"
#include "email-api-mailbox.h"
#include "email-types.h"
#include "email-core-signal.h"
#include "email-utilities.h"
#include "email-ipc.h"
#include "email-storage.h"


EXPORT_API int email_send_mail(int mail_id, int *handle)
{
	EM_DEBUG_API_BEGIN ("mail_id[%d] handle[%p]", mail_id, handle);
	
	int err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t* mail_table_data = NULL;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;

	if(mail_id <= 0) {
		EM_DEBUG_EXCEPTION("mail_id is not valid");
		err= EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if(!emstorage_get_mail_by_id(mail_id, &mail_table_data, true, &err) || !mail_table_data) {
		EM_DEBUG_EXCEPTION("Failed to get mail by mail_id [%d]", err);
		goto FINISH_OFF;
	}

	if (mail_table_data->account_id <= 0) {
		EM_DEBUG_EXCEPTION ("EM_IF_ACCOUNT_ID_NULL: Account ID [ %d ]  ", mail_table_data->account_id);
		emstorage_free_mail(&mail_table_data, 1, NULL);
		return EMAIL_ERROR_INVALID_PARAM;
	}

	EM_DEBUG_LOG("mail_table_data->account_id[%d], mail_table_data->mailbox_id[%d]", mail_table_data->account_id, mail_table_data->mailbox_id);

	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	/*  check account bind type and branch off  */
	if ( em_get_account_server_type_by_account_id(mail_table_data->account_id, &account_server_type, false, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle;
		if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}
		
		/*  noti to active sync */
		as_noti_data.send_mail.handle     = as_handle;
		as_noti_data.send_mail.account_id = mail_table_data->account_id;
		as_noti_data.send_mail.mail_id    = mail_id;

		if ( em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SEND_MAIL, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(handle)
			*handle = as_handle;
	}
	else {
		hAPI = emipc_create_email_api(_EMAIL_API_SEND_MAIL);	

		if (!hAPI ) {
			EM_DEBUG_EXCEPTION ("INVALID PARAM: hAPI NULL ");
			emstorage_free_mail(&mail_table_data, 1, NULL);
			return EMAIL_ERROR_NULL_VALUE;
		}

		/* mail_id */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))){
			EM_DEBUG_EXCEPTION("email_send_mail--Add Param mail_id failed");
			emstorage_free_mail(&mail_table_data, 1, NULL);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("email_send_mail--emipc_execute_proxy_api failed  ");
			emstorage_free_mail(&mail_table_data, 1, NULL);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_CRASH);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		if (err == EMAIL_ERROR_NONE) {
			if(handle)
				emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
	 	}
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = (HIPC_API)NULL;

	emstorage_free_mail(&mail_table_data, 1, NULL);

	EM_DEBUG_API_END ("err[%d]", err);  
	return err;	
}

EXPORT_API int email_send_mail_with_downloading_attachment_of_original_mail(int input_mail_id, int *output_handle)
{
	EM_DEBUG_API_BEGIN ("input_mail_id[%d] output_handle[%p]", input_mail_id, output_handle);

	int err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t* mail_table_data = NULL;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	task_parameter_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL task_parameter;

	if(input_mail_id <= 0) {
		EM_DEBUG_EXCEPTION("mail_id is not valid");
		err= EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if(!emstorage_get_mail_by_id(input_mail_id, &mail_table_data, true, &err) || !mail_table_data) {
		EM_DEBUG_EXCEPTION("Failed to get mail by mail_id [%d]", err);
		goto FINISH_OFF;
	}

	if (mail_table_data->account_id <= 0) {
		EM_DEBUG_EXCEPTION ("EM_IF_ACCOUNT_ID_NULL: Account ID [ %d ]  ", mail_table_data->account_id);
		emstorage_free_mail(&mail_table_data, 1, NULL);
		return EMAIL_ERROR_INVALID_PARAM;
	}

	EM_DEBUG_LOG("mail_table_data->account_id[%d], mail_table_data->mailbox_id[%d]", mail_table_data->account_id, mail_table_data->mailbox_id);

	/*  check account bind type and branch off  */
	if ( em_get_account_server_type_by_account_id(mail_table_data->account_id, &account_server_type, false, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle;
		ASNotiData as_noti_data;

		memset(&as_noti_data, 0x00, sizeof(ASNotiData));

		if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.send_mail_with_downloading_attachment_of_original_mail.handle     = as_handle;
		as_noti_data.send_mail_with_downloading_attachment_of_original_mail.mail_id    = input_mail_id;
		as_noti_data.send_mail_with_downloading_attachment_of_original_mail.account_id = mail_table_data->account_id;

		if ( em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SEND_MAIL_WITH_DOWNLOADING_OF_ORIGINAL_MAIL, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(output_handle)
			*output_handle = as_handle;
	}
	else {
		task_parameter.mail_id        = input_mail_id;

		if((err = emipc_execute_proxy_task(EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL, &task_parameter)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("execute_proxy_task failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = (HIPC_API)NULL;

	emstorage_free_mail(&mail_table_data, 1, NULL);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_schedule_sending_mail(int input_mail_id, time_t input_scheduled_time)
{
	EM_DEBUG_API_BEGIN ("mail_id[%d] input_time[%d]", input_mail_id, input_scheduled_time);

	int err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t* mail_table_data = NULL;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	task_parameter_EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL task_parameter;

	if(input_mail_id <= 0) {
		EM_DEBUG_EXCEPTION("mail_id is not valid");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if(!emstorage_get_mail_by_id(input_mail_id, &mail_table_data, true, &err) || !mail_table_data) {
		EM_DEBUG_EXCEPTION("Failed to get mail by mail_id [%d]", err);
		goto FINISH_OFF;
	}

	/*  check account bind type and branch off  */
	if ( em_get_account_server_type_by_account_id(mail_table_data->account_id, &account_server_type, false, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
		int as_handle;
		ASNotiData as_noti_data;

		memset(&as_noti_data, 0x00, sizeof(ASNotiData));

		if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.schedule_sending_mail.handle         = as_handle;
		as_noti_data.schedule_sending_mail.account_id     = mail_table_data->account_id;
		as_noti_data.schedule_sending_mail.mail_id        = input_mail_id;
		as_noti_data.schedule_sending_mail.scheduled_time = input_scheduled_time;

		if ( em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SCHEDULE_SENDING_MAIL, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}
	}
	else {
		task_parameter.mail_id         = input_mail_id;
		task_parameter.scheduled_time  = input_scheduled_time;

		if((err = emipc_execute_proxy_task(EMAIL_SYNC_TASK_SCHEDULE_SENDING_MAIL, &task_parameter)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("execute_proxy_task failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = (HIPC_API)NULL;

	emstorage_free_mail(&mail_table_data, 1, NULL);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_send_saved(int account_id, int *handle)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] handle[%p]", account_id, handle);
	
	char* pOptionStream = NULL;
	int err = EMAIL_ERROR_NONE;
	
	EM_IF_NULL_RETURN_VALUE(account_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(account_id, EMAIL_ERROR_INVALID_PARAM);
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SEND_SAVED);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);
	
	/* Account ID */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(account_id), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter account_id failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_SAFE_FREE(pOptionStream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	
	emipc_destroy_email_api(hAPI);
	
	hAPI = NULL;
	EM_SAFE_FREE(pOptionStream);

	EM_DEBUG_API_END ("err[%d]", err);  
	return err;
}

EXPORT_API int email_sync_header(int input_account_id, int input_mailbox_id, int *handle)
{
	EM_DEBUG_API_BEGIN ("input_account_id[%d] input_mailbox_id[%d] handle[%p]", input_account_id, input_mailbox_id, handle);
	int err = EMAIL_ERROR_NONE;	
	/* int total_count = 0; */
	
	EM_IF_ACCOUNT_ID_NULL(input_account_id, EMAIL_ERROR_INVALID_PARAM);

	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	/*  2010/02/12 ch715.lee : check account bind type and branch off  */
	if ( em_get_account_server_type_by_account_id(input_account_id, &account_server_type, true, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle;
		if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			goto FINISH_OFF;
		}
		
		/*  noti to active sync */
		as_noti_data.sync_header.handle = as_handle;
		as_noti_data.sync_header.account_id = input_account_id;
		/* In case that Mailbox is NULL,   SYNC ALL MAILBOX */
		as_noti_data.sync_header.mailbox_id = input_mailbox_id;

		if ( em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SYNC_HEADER, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(handle)
			*handle = as_handle;

	}
	else {
		hAPI = emipc_create_email_api(_EMAIL_API_SYNC_HEADER);	

		EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

		/* input_account_id */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_account_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/* input_mailbox_id */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_mailbox_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}
			
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	

		if (err != EMAIL_ERROR_NONE)
			goto FINISH_OFF;

		if(handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);	
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_API_END ("err[%d]", err);  
	return err;
}


EXPORT_API int email_sync_header_for_all_account(int *handle)
{
	EM_DEBUG_API_BEGIN ("handle[%p]", handle);
	char* mailbox_stream = NULL;
	int err = EMAIL_ERROR_NONE;	
	HIPC_API hAPI = NULL;
	int return_handle;
	ASNotiData as_noti_data;
	int i, account_count = 0;
	emstorage_account_tbl_t *account_tbl_array = NULL;
	int as_err;
	int input_account_id = ALL_ACCOUNT;
	int input_mailbox_id = 0; /* all case */

	hAPI = emipc_create_email_api(_EMAIL_API_SYNC_HEADER);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* input_account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	/* input_account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_mailbox_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}
		
	 emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	

	 if (err != EMAIL_ERROR_NONE)
		 goto FINISH_OFF;

	   emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &return_handle);

	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	/*  Get all accounts for sending notification to active sync engine. */
	if (!emstorage_get_account_list(&account_count, &account_tbl_array , true, false, &as_err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [ %d ]  ", as_err);

		goto FINISH_OFF;
	}

	for(i = 0; i < account_count; i++) {
		if ( account_tbl_array[i].incoming_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
			/*  use returned handle value for a active sync handle */
			/* int as_handle; */
			/*
			if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
				EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
				goto FINISH_OFF;
			}
			*/
			
			/*  noti to active sync */
			as_noti_data.sync_header.handle = return_handle;
			as_noti_data.sync_header.account_id = account_tbl_array[i].account_id;
			/* In case that Mailbox is NULL,   SYNC ALL MAILBOX */
			as_noti_data.sync_header.mailbox_id = 0;

			if ( em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SYNC_HEADER, &as_noti_data) == false) {
				EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
				err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
				goto FINISH_OFF;
			}
		}
	}
	 
	 if(handle)
		*handle = return_handle;

FINISH_OFF:

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	if ( account_tbl_array )
		emstorage_free_account(&account_tbl_array, account_count, NULL);
	
	EM_DEBUG_API_END ("err[%d]", err);  
	return err;
}

EXPORT_API int email_download_body(int mail_id, int with_attachment, int *handle)
{
	EM_DEBUG_API_BEGIN ("mail_id[%d] with_attachment[%d]", mail_id, with_attachment);
	int err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t* mail_table_data = NULL;
	int account_id = 0;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	if(mail_id <= 0) {
		EM_DEBUG_EXCEPTION("mail_id is not valid");
		err= EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if(!emstorage_get_mail_by_id(mail_id, &mail_table_data, true, &err) || !mail_table_data ) {
		EM_DEBUG_EXCEPTION("Failed to get mail by mail_id [%d]", err);
		goto FINISH_OFF;
	}

	if (mail_table_data->account_id <= 0) {
		EM_DEBUG_EXCEPTION("EM_IF_ACCOUNT_ID_NULL: Account ID [ %d ]  ", mail_table_data->account_id);
		goto FINISH_OFF;
	}

	account_id = mail_table_data->account_id;
		
	/*  2010/02/12 ch715.lee : check account bind type and branch off  */
	if ( em_get_account_server_type_by_account_id(account_id, &account_server_type, true, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle;
		if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			goto FINISH_OFF;
		}
		
		/*  noti to active sync */
		as_noti_data.download_body.handle = as_handle;
		as_noti_data.download_body.account_id = account_id;
		as_noti_data.download_body.mail_id = mail_id;
		as_noti_data.download_body.with_attachment = with_attachment;

		if ( em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_DOWNLOAD_BODY, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(handle)
			*handle = as_handle;
	} else {
		hAPI = emipc_create_email_api(_EMAIL_API_DOWNLOAD_BODY);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

		/* Account Id */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}
		/* Mail Id */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		/* with_attachment */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&with_attachment, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}
		
		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}
			
		 emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	
		 if (err != EMAIL_ERROR_NONE)		
			 goto FINISH_OFF;
		 
		 if(handle)	
		 {
		 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
			EM_DEBUG_LOG("RETURN VALUE : %d  handle %d", err, *handle);

		 }
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	if(mail_table_data) {
		emstorage_free_mail(&mail_table_data, 1, &err);
	}

	EM_DEBUG_API_END ("err[%d]", err);  
	return err;

}


/* API - Downloads the Email Attachment Information [ INTERNAL ] */

EXPORT_API int email_download_attachment(int mail_id, int nth, int *handle)
{
	EM_DEBUG_API_BEGIN ("mail_id[%d] nth[%d] handle[%p]", mail_id, nth, handle);
	char* mailbox_stream = NULL;
	int err = EMAIL_ERROR_NONE;
	emstorage_mail_tbl_t* mail_table_data = NULL;
	int account_id = 0;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	if(mail_id <= 0) {
		EM_DEBUG_EXCEPTION("mail_id is not valid");
		err= EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if(!emstorage_get_mail_by_id(mail_id, &mail_table_data, true, &err) || !mail_table_data ) {
		EM_DEBUG_EXCEPTION("Failed to get mail by mail_id [%d]", err);
		goto FINISH_OFF;
	}

	if (mail_table_data->account_id <= 0) {
		EM_DEBUG_EXCEPTION("EM_IF_ACCOUNT_ID_NULL: Account ID [ %d ]  ", mail_table_data->account_id);
		goto FINISH_OFF;
	}

	account_id = mail_table_data->account_id;
	
	if ( em_get_account_server_type_by_account_id(account_id, &account_server_type, true, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle;
		if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			goto FINISH_OFF;
		}
		
		/*  noti to active sync */
		as_noti_data.download_attachment.handle = as_handle;
		as_noti_data.download_attachment.account_id = account_id;
		as_noti_data.download_attachment.mail_id = mail_id;
		as_noti_data.download_attachment.attachment_order = nth;
		if ( em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_DOWNLOAD_ATTACHMENT, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(handle)
			*handle = as_handle;
	} else {
		hAPI = emipc_create_email_api(_EMAIL_API_DOWNLOAD_ATTACHMENT);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

		/* Account Id */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		/* Mail ID */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter mail_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		/* nth */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(nth), sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter mail_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		/* Execute API */
		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		if (err != EMAIL_ERROR_NONE)		
			goto FINISH_OFF;
		 
		if(handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
		
	}
	
FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	if(mail_table_data) {
		emstorage_free_mail(&mail_table_data, 1, &err);
	}

	EM_DEBUG_API_END ("err[%d]", err);  
	return err;
	
}


EXPORT_API int email_cancel_job(int input_account_id, int input_handle, email_cancelation_type input_cancel_type)
{
	EM_DEBUG_API_BEGIN ("input_account_id[%d] input_handle[%d] input_cancel_type[%d]", input_account_id, input_handle, input_cancel_type);
	int err = EMAIL_ERROR_NONE;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	emstorage_account_tbl_t *account_list = NULL;
	int i, account_count = 0;

	if(input_account_id < 0)
		return EMAIL_ERROR_INVALID_PARAM;

	if ( input_account_id == ALL_ACCOUNT ) {
		/*  this means that job is executed with all account */
		/*  Get all accounts for sending notification to active sync engine. */
		if (!emstorage_get_account_list(&account_count, &account_list , true, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", err);
			goto FINISH_OFF;
		}

		for(i = 0; i < account_count; i++) {
			if ( em_get_account_server_type_by_account_id(account_list[i].account_id, &account_server_type, true, &err) == false ) {
				EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
				goto FINISH_OFF;
			}

			if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
				memset(&as_noti_data, 0x00, sizeof(ASNotiData));
				as_noti_data.cancel_job.account_id  = account_list[i].account_id;
				as_noti_data.cancel_job.handle      = input_handle;
				as_noti_data.cancel_job.cancel_type = input_cancel_type;


				if ( em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_CANCEL_JOB, &as_noti_data) == false) {
					EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
					err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
					goto FINISH_OFF;
				}
			}
		}

		/*  request canceling to stub */
		hAPI = emipc_create_email_api(_EMAIL_API_CANCEL_JOB);

		if (!hAPI) {
			EM_DEBUG_EXCEPTION ("INVALID PARAM: hAPI NULL ");
			err = EMAIL_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_account_id, sizeof(int))) {		/*  input_account_id == 0 */
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMAIL_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_handle, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMAIL_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}

		/* Execute API */
		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
			goto FINISH_OFF;
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		emipc_destroy_email_api(hAPI);
		hAPI = NULL;
	}
	else {
		if ( em_get_account_server_type_by_account_id(input_account_id, &account_server_type, true, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
			goto FINISH_OFF;
		}

		if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
			memset(&as_noti_data, 0x00, sizeof(ASNotiData));
			as_noti_data.cancel_job.account_id = input_account_id;
			as_noti_data.cancel_job.handle = input_handle;

			if ( em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_CANCEL_JOB, &as_noti_data) == false) {
				EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
				err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
				goto FINISH_OFF;
			}
		} else {
			hAPI = emipc_create_email_api(_EMAIL_API_CANCEL_JOB);

			EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

			if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_account_id, sizeof(int))) {
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
				EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
			}

			if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_handle, sizeof(int))) {
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
				EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
			}

			/* Execute API */
			if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
				EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
			}
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
			emipc_destroy_email_api(hAPI);
			hAPI = NULL;
		}
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	if (account_list)
		emstorage_free_account(&account_list, account_count, NULL);
		
	EM_DEBUG_API_END ("err[%d]", err);  
	return err;
}



EXPORT_API int email_get_pending_job(email_action_t action, int account_id, int mail_id, email_event_status_type_t * status)
{
	EM_DEBUG_API_BEGIN ("action[%d] account_id[%d] mail_id[%d] status[%p]", action, account_id, mail_id, status);
	int err = EMAIL_ERROR_NOT_SUPPORTED;
	EM_DEBUG_API_END ("err[%d]", err);  
	return err;

}

EXPORT_API int email_get_network_status(int* on_sending, int* on_receiving)
{
	EM_DEBUG_API_BEGIN ("on_sending[%p] on_receiving[%p]", on_sending, on_receiving);
	int err = EMAIL_ERROR_NOT_SUPPORTED;
	EM_DEBUG_API_END ("err[%d]", err);  
	return err;
}

EXPORT_API int email_get_task_information(email_task_information_t **output_task_information, int *output_task_information_count)
{
	EM_DEBUG_API_BEGIN ("output_task_information[%p] output_task_information_count[%p]", output_task_information, output_task_information_count);
	int err = EMAIL_ERROR_NONE;
	int task_information_stream_length = 0;
	HIPC_API hAPI = NULL;
	char *task_information_stream = NULL;

	if(output_task_information == NULL || output_task_information_count == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	if((hAPI = emipc_create_email_api(_EMAIL_API_GET_TASK_INFORMATION)) == NULL) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_IPC_CRASH;
		goto FINISH_OFF;
	}

	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_CRASH;
		goto FINISH_OFF;
	}
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	if(EMAIL_ERROR_NONE == err) {
		task_information_stream_length = emipc_get_parameter_length(hAPI, ePARAMETER_OUT, 1);
		if(task_information_stream_length > 0) {
			task_information_stream = (char*)em_malloc(task_information_stream_length + 1);

			if(!task_information_stream) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, task_information_stream_length, task_information_stream);
			em_convert_byte_stream_to_task_information(task_information_stream, task_information_stream_length, output_task_information, output_task_information_count);
			EM_SAFE_FREE(task_information_stream); /*prevent 18951*/
		}

		if(!output_task_information) {
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_NULL_VALUE");
			err = EMAIL_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_sync_imap_mailbox_list(int account_id, int *handle)
{
	EM_DEBUG_API_BEGIN ();

	int err = EMAIL_ERROR_NONE;

	if(account_id <= 0) {
		EM_DEBUG_EXCEPTION("invalid parameters");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_GET_IMAP_MAILBOX_LIST);	

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_LOG("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	if(!emipc_execute_proxy_api(hAPI))  {
		EM_DEBUG_LOG("ipcProxy_ExecuteAsyncAPI failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	if(handle)
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_API_END ("err[%d]", err);  
	return err;
}

EXPORT_API int email_search_mail_on_server(int input_account_id, int input_mailbox_id, email_search_filter_t *input_search_filter_list, int input_search_filter_count, int *output_handle)
{
	EM_DEBUG_API_BEGIN ("input_account_id[%d] input_mailbox_id[%d] input_search_filter_list[%p] input_search_filter_count[%d] output_handle[%p]", input_account_id, input_mailbox_id, input_search_filter_list, input_search_filter_count, output_handle);

	int       err = EMAIL_ERROR_NONE;
	int       return_value = 0;
	int       stream_size_for_search_filter_list = 0;
	char     *stream_for_search_filter_list = NULL;
	HIPC_API  hAPI = NULL;
	email_account_server_t account_server_type = EMAIL_SERVER_TYPE_NONE;
	ASNotiData as_noti_data;

	EM_IF_NULL_RETURN_VALUE(input_account_id,         EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mailbox_id,         EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_search_filter_list, EMAIL_ERROR_INVALID_PARAM);

	memset(&as_noti_data, 0, sizeof(ASNotiData)); /* initialization of union members */

	if ( em_get_account_server_type_by_account_id(input_account_id, &account_server_type, true, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle = 0;

		if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.search_mail_on_server.handle              = as_handle;
		as_noti_data.search_mail_on_server.account_id          = input_account_id;
		as_noti_data.search_mail_on_server.mailbox_id          = input_mailbox_id;
		as_noti_data.search_mail_on_server.search_filter_list  = input_search_filter_list;
		as_noti_data.search_mail_on_server.search_filter_count = input_search_filter_count;

		return_value = em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SEARCH_ON_SERVER, &as_noti_data);

		if ( return_value == false ) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(output_handle)
			*output_handle = as_handle;
	}
	else
	{
		hAPI = emipc_create_email_api(_EMAIL_API_SEARCH_MAIL_ON_SERVER);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (void*)&input_account_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
			goto FINISH_OFF;
		}

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (void*)&input_mailbox_id, sizeof(int))){
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
			goto FINISH_OFF;
		}

		stream_for_search_filter_list = em_convert_search_filter_to_byte_stream(input_search_filter_list, input_search_filter_count, &stream_size_for_search_filter_list);

		EM_PROXY_IF_NULL_RETURN_VALUE(stream_for_search_filter_list, hAPI, EMAIL_ERROR_NULL_VALUE);

		if(!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, stream_for_search_filter_list, stream_size_for_search_filter_list)) { /*prevent 18950*/
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
			goto FINISH_OFF;
		}

		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("_EMAIL_API_SEARCH_MAIL_ON_SERVER failed [%d]", err);
			goto FINISH_OFF;
		}

		if(output_handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), output_handle);
	}

FINISH_OFF:
	if(hAPI) {
		emipc_destroy_email_api(hAPI);
		hAPI = NULL;
	}

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_clear_result_of_search_mail_on_server(int input_account_id)
{
	EM_DEBUG_API_BEGIN ("input_account_id[%d]", input_account_id);

	int       err = EMAIL_ERROR_NONE;
	int       return_value = 0;
	HIPC_API  hAPI = NULL;
	email_account_server_t account_server_type = EMAIL_SERVER_TYPE_NONE;
	ASNotiData as_noti_data;

	EM_IF_NULL_RETURN_VALUE(input_account_id,         EMAIL_ERROR_INVALID_PARAM);

	memset(&as_noti_data, 0, sizeof(ASNotiData)); /* initialization of union members */

	if ( em_get_account_server_type_by_account_id(input_account_id, &account_server_type, true, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle = 0;

		if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.clear_result_of_search_mail_on_server.handle              = as_handle;
		as_noti_data.clear_result_of_search_mail_on_server.account_id          = input_account_id;

		return_value = em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_CLEAR_RESULT_OF_SEARCH_ON_SERVER, &as_noti_data);

		if ( return_value == false ) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}
	}
	else {
		hAPI = emipc_create_email_api(_EMAIL_API_CLEAR_RESULT_OF_SEARCH_MAIL_ON_SERVER);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (void*)&input_account_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
			goto FINISH_OFF;
		}

		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("_EMAIL_API_CLEAR_RESULT_OF_SEARCH_MAIL_ON_SERVER failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	if(hAPI) {
		emipc_destroy_email_api(hAPI);
		hAPI = NULL;
	}

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_query_smtp_mail_size_limit(int account_id, int *handle)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] handle[%p]", account_id, handle);
	int err = EMAIL_ERROR_NONE;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;

	if (account_id <= 0) {
		EM_DEBUG_EXCEPTION("account_id is not valid");
		err= EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (em_get_account_server_type_by_account_id(account_id, &account_server_type, false, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if (account_server_type != EMAIL_SERVER_TYPE_ACTIVE_SYNC) {
		hAPI = emipc_create_email_api(_EMAIL_API_QUERY_SMTP_MAIL_SIZE_LIMIT);

		if (!hAPI ) {
			EM_DEBUG_EXCEPTION ("INVALID PARAM: hAPI NULL");
			return EMAIL_ERROR_NULL_VALUE;
		}

		/*account_id */
		if (!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("email_query_smtp_mail_size_limit--Add Param mail_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
		}

		if (emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("email_query_smtp_mail_size_limit--emipc_execute_proxy_api failed  ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_CRASH);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		if (err == EMAIL_ERROR_NONE) {
			if(handle)
				emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
		}
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = (HIPC_API)NULL;

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}
