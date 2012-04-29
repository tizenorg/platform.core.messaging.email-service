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
 * @file		email-api-mail.c
 * @brief 		This file contains the data structures and interfaces of Message related Functionality provided by 
 *			email-service . 
 */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "email-api.h"
#include "email-ipc.h"
#include "email-convert.h"
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-smtp.h"
#include "email-storage.h"
#include "email-utilities.h"
#include "db-util.h"

#define  DIR_SEPERATOR_CH '/'

#define _DIRECT_TO_DB

EXPORT_API int email_add_mail(emf_mail_data_t *input_mail_data, emf_attachment_data_t *input_attachment_data_list, int input_attachment_count, emf_meeting_request_t* input_meeting_request, int input_from_eas)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count [%d], input_meeting_request [%p], input_from_eas [%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);

	EM_IF_NULL_RETURN_VALUE(input_mail_data,               EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mail_data->account_id,   EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mail_data->mailbox_name, EMF_ERROR_INVALID_PARAM);

	if(input_attachment_count > 0 && !input_attachment_data_list) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;
	}

	int       err = EMF_ERROR_NONE;
	int       size = 0;
	char     *mail_data_stream = NULL;
	char     *attachment_data_list_stream  = NULL;
	char     *meeting_request_stream = NULL;
	HIPC_API  hAPI = NULL;
	
	if(input_from_eas == 0) {
		hAPI = emipc_create_email_api(_EMAIL_API_ADD_MAIL);

		if(!hAPI) {
			EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
			err = EMF_ERROR_NULL_VALUE;		
			goto FINISH_OFF;
		}

		/* emf_mail_data_t */
		mail_data_stream = em_convert_mail_data_to_byte_stream(input_mail_data, 1, &size);

		if(!mail_data_stream) {
			EM_DEBUG_EXCEPTION("em_convert_mail_data_to_byte_stream failed");
			err = EMF_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mail_data_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter for head failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		/* emf_attachment_data_t */
		attachment_data_list_stream = em_convert_attachment_data_to_byte_stream(input_attachment_data_list, input_attachment_count, &size);

		if(!attachment_data_list_stream) {
			EM_DEBUG_EXCEPTION("em_convert_attachment_data_to_byte_stream failed");
			err = EMF_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, attachment_data_list_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/*  emf_meeting_request_t */
		if ( input_mail_data->meeting_request_status != EMF_MAIL_TYPE_NORMAL ) {
			meeting_request_stream = em_convert_meeting_req_to_byte_stream(input_meeting_request, &size);

			if(!meeting_request_stream) {
				EM_DEBUG_EXCEPTION("em_convert_meeting_req_to_byte_stream failed");
				err = EMF_ERROR_NULL_VALUE;
				goto FINISH_OFF;
			}

			if(!emipc_add_parameter(hAPI, ePARAMETER_IN, meeting_request_stream, size)) {
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
		}

		/* input_from_eas */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_from_eas, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/* Execute API */
		if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			err = EMF_ERROR_IPC_SOCKET_FAILURE;
			goto FINISH_OFF;
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

		if(err == EMF_ERROR_NONE) {
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &input_mail_data->mail_id); /* result mail_id */
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 2, sizeof(int), &input_mail_data->thread_id); /* result thread_id */
		}
	} 
	else {
		if((err = emcore_add_mail(input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas)) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_add_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	if(hAPI) 
		emipc_destroy_email_api(hAPI);

	EM_SAFE_FREE(mail_data_stream);
	EM_SAFE_FREE(attachment_data_list_stream);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;

}

EXPORT_API int email_add_read_receipt(int input_read_mail_id, int *output_receipt_mail_id)
{
	EM_DEBUG_FUNC_BEGIN("input_read_mail_id [%d], output_receipt_mail_id [%p]", input_read_mail_id, output_receipt_mail_id);

	EM_IF_NULL_RETURN_VALUE(output_receipt_mail_id, EMF_ERROR_INVALID_PARAM);

	int      err = EMF_ERROR_NONE;
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_ADD_READ_RECEIPT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_read_mail_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("Add Param mail body Fail");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	EM_DEBUG_LOG("err [%d]", err);

	if(err == EMF_ERROR_NONE) {
		/* Get receipt mail id */
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), output_receipt_mail_id);
		EM_DEBUG_LOG("output_receipt_mail_id [%d]", *output_receipt_mail_id);
	}

	emipc_destroy_email_api(hAPI);

	hAPI = NULL;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


int email_create_db_full()
{
	int mailbox_index, mail_index, mailbox_count, mail_slot_size;
	emstorage_mail_tbl_t mail_table_data = {0};
	emf_mailbox_t *mailbox_list = NULL;
	int err = EMF_ERROR_NONE;

	if ( (err = email_open_db()) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_open_db failed [%d]", err);
		return err;
	}

	mail_table_data.subject = (char*) em_malloc(50); 
	mail_table_data.full_address_from = strdup("<dummy_from@nowhere.com>");
	mail_table_data.full_address_to = strdup("<dummy_to@nowhere.com>");
	mail_table_data.account_id =1;
	mail_table_data.mailbox_name = (char*) em_malloc(250);

	if( (err = email_get_mailbox_list_ex(1, -1, 0, &mailbox_list, &mailbox_count)) < EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_get_mailbox_list_ex failed [%d]", err);
		goto FINISH_OFF;
	}

	for(mailbox_index = 0; mailbox_index < mailbox_count; mailbox_index++) {
		mail_slot_size= mailbox_list[mailbox_index].mail_slot_size;
		for(mail_index = 0; mail_index < mail_slot_size; mail_index++) {
			sprintf(mail_table_data.subject, "Subject #%d",mail_index);
			strncpy(mail_table_data.mailbox_name, mailbox_list[mailbox_index].name, 250 - 1);
			mail_table_data.mailbox_type = mailbox_list[mailbox_index].mailbox_type;
			if( !emstorage_add_mail(&mail_table_data, 1, true, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_add_mail failed [%d]",err);
		
				goto FINISH_OFF;
			}
		}
	}

FINISH_OFF:
	if ( (err = email_close_db()) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_close_db failed [%d]", err);
	}	
	
	if(mailbox_list)
		email_free_mailbox(&mailbox_list, mailbox_count);

	EM_SAFE_FREE(mail_table_data.subject);
	EM_SAFE_FREE(mail_table_data.mailbox_name);
	EM_SAFE_FREE(mail_table_data.full_address_from);
	EM_SAFE_FREE(mail_table_data.full_address_to);

	return err;
}

EXPORT_API int email_update_mail(emf_mail_data_t *input_mail_data, emf_attachment_data_t *input_attachment_data_list, int input_attachment_count, emf_meeting_request_t* input_meeting_request, int input_from_eas)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count [%d], input_meeting_request [%p], input_from_eas [%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);

	EM_IF_NULL_RETURN_VALUE(input_mail_data,               EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mail_data->account_id,   EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mail_data->mailbox_name, EMF_ERROR_INVALID_PARAM);

	if(input_attachment_count > 0 && !input_attachment_data_list) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;
	}

	int       err = EMF_ERROR_NONE;
	int       size = 0;
	char     *mail_data_stream = NULL;
	char     *attachment_data_list_stream  = NULL;
	char     *meeting_request_stream = NULL;

	HIPC_API  hAPI = NULL;
	
	if(input_from_eas == 0) {
		hAPI = emipc_create_email_api(_EMAIL_API_UPDATE_MAIL);	

		if(!hAPI) {
			EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
			err = EMF_ERROR_NULL_VALUE;		
			goto FINISH_OFF;
		}

		/* emf_mail_data_t */
		mail_data_stream = em_convert_mail_data_to_byte_stream(input_mail_data, 1, &size);

		if(!mail_data_stream) {
			EM_DEBUG_EXCEPTION("em_convert_mail_data_to_byte_stream failed");
			err = EMF_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}
		
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mail_data_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter for head failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		/* emf_attachment_data_t */
		attachment_data_list_stream = em_convert_attachment_data_to_byte_stream(input_attachment_data_list, input_attachment_count, &size);

		if(!attachment_data_list_stream) {
			EM_DEBUG_EXCEPTION("em_convert_attachment_data_to_byte_stream failed");
			err = EMF_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, attachment_data_list_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/*  emf_meeting_request_t */
		if ( input_mail_data->meeting_request_status != EMF_MAIL_TYPE_NORMAL ) {
			meeting_request_stream = em_convert_meeting_req_to_byte_stream(input_meeting_request, &size);

			if(!meeting_request_stream) {
				EM_DEBUG_EXCEPTION("em_convert_meeting_req_to_byte_stream failed");
				err = EMF_ERROR_NULL_VALUE;
				goto FINISH_OFF;
			}

			if(!emipc_add_parameter(hAPI, ePARAMETER_IN, meeting_request_stream, size)) {
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
		}

		/* input_from_eas */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_from_eas, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/* Execute API */
		if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			err = EMF_ERROR_IPC_SOCKET_FAILURE;
			goto FINISH_OFF;
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		
		if(err == EMF_ERROR_NONE) {
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &input_mail_data->mail_id);
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 2, sizeof(int), &input_mail_data->thread_id);
		}
	} 
	else {
		if( (err = emcore_update_mail(input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas)) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_update_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	if(hAPI) 
		emipc_destroy_email_api(hAPI);

	EM_SAFE_FREE(mail_data_stream);
	EM_SAFE_FREE(attachment_data_list_stream);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;

}

EXPORT_API int email_clear_mail_data()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_CLEAR_DATA);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);
	
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api Fail");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


EXPORT_API int email_count_message(emf_mailbox_t* mailbox, int* total, int* unseen)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], total[%p], unseen[%p]", mailbox, total, unseen);
	
	int total_count = 0;
	int unread = 0;
	int err = EMF_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(total, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(unseen, EMF_ERROR_INVALID_PARAM);

   	if (!emstorage_get_mail_count(mailbox->account_id,  mailbox->name, &total_count, &unread, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_count Failed");

	} else {
		*total = total_count;
		*unseen = unread;
	}
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}




EXPORT_API int  email_count_message_all_mailboxes(emf_mailbox_t* mailbox, int* total, int* unseen)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], total[%p], unseen[%p]", mailbox, total, unseen);
	
	int total_count = 0;
	int unread = 0;
	int err = EMF_ERROR_NONE;
		
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(total, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(unseen, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(mailbox->account_id, EMF_ERROR_INVALID_PARAM); 

    if (!emstorage_get_mail_count(mailbox->account_id,  NULL, &total_count, &unread, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_count Failed");

	} else {
		*total = total_count;
		*unseen = unread;
	}
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}



EXPORT_API int email_delete_message(emf_mailbox_t* mailbox, int *mail_ids, int num, int from_server)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%p], num[%d], from_server[%d]", mailbox, mail_ids, num, from_server);

	char* mailbox_stream = NULL;
	int size = 0;
	int err = EMF_ERROR_NONE;
	HIPC_API hAPI = NULL;

	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(mail_ids, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(mailbox->account_id, EMF_ERROR_INVALID_PARAM); 
	
	if (num <= 0) {
		EM_DEBUG_EXCEPTION("num = %d", num);
		err = EMF_ERROR_INVALID_PARAM;		
		return err;
	}

	hAPI = emipc_create_email_api(_EMAIL_API_DELETE_MAIL);
	
	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* mailbox */
	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	if(!mailbox_stream) {
		EM_DEBUG_EXCEPTION("em_convert_mailbox_to_byte_stream failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}
	
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Number of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&num, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* set of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)mail_ids, num * sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* from-server */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&from_server, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_SAFE_FREE(mailbox_stream); 

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


EXPORT_API int email_delete_all_message_in_mailbox(emf_mailbox_t* mailbox, int from_server)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], from_server[%d]", mailbox, from_server);

	char* mailbox_stream = NULL;
	int size = 0;
	int err = EMF_ERROR_NONE;
	HIPC_API hAPI = NULL;
	
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(mailbox->account_id, EMF_ERROR_INVALID_PARAM); 
	
	hAPI =emipc_create_email_api(_EMAIL_API_DELETE_ALL_MAIL);	

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	if(!mailbox_stream) {
		EM_DEBUG_EXCEPTION("em_convert_mailbox_to_byte_stream failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}
	
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&from_server, sizeof(int))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int),&err );

FINISH_OFF:

	if(hAPI)	
		emipc_destroy_email_api(hAPI);

	EM_SAFE_FREE(mailbox_stream);
	
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}



EXPORT_API int email_add_attachment( emf_mailbox_t* mailbox, int mail_id, emf_attachment_info_t* attachment)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], attachment[%p]", mailbox, mail_id, attachment);
	int err = EMF_ERROR_NONE;
	char* mailbox_stream = NULL;
	char* pAttchStream = NULL;
	int size = 0;
	HIPC_API hAPI = NULL;
	
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(mail_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(attachment, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(mailbox->account_id, EMF_ERROR_INVALID_PARAM);
	
	hAPI = emipc_create_email_api(_EMAIL_API_ADD_ATTACHMENT);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* Mailbox */
	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	if(!mailbox_stream) {
		EM_DEBUG_EXCEPTION("em_convert_mailbox_to_byte_stream failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* mail_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Attachment */
	pAttchStream = em_convert_attachment_info_to_byte_stream(attachment, &size);

	if(!pAttchStream) {
		EM_DEBUG_EXCEPTION("em_convert_attachment_info_to_byte_stream failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, pAttchStream, size)){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	
	if(EMF_ERROR_NONE == err) {
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &attachment->attachment_id);
	}

FINISH_OFF:
	EM_SAFE_FREE(mailbox_stream);
	EM_SAFE_FREE(pAttchStream);
	
	if(hAPI)
		emipc_destroy_email_api(hAPI);
	
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
 }


EXPORT_API int email_delete_attachment(emf_mailbox_t * mailbox, int mail_id, const char *attachment_id)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], attachment_id[%s]", mailbox, mail_id, attachment_id);
	int err = EMF_ERROR_NONE;
	char* mailbox_stream = NULL;
	int size = 0;

	HIPC_API hAPI = NULL;
	
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(mail_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(attachment_id, EMF_ERROR_INVALID_PARAM);
	
	hAPI = emipc_create_email_api(_EMAIL_API_DELETE_ATTACHMENT);
	
	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* Mailbox */
	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	if(!mailbox_stream) {
		EM_DEBUG_EXCEPTION("em_convert_mailbox_to_byte_stream failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* mail_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* attachment_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)attachment_id, strlen(attachment_id))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_SAFE_FREE(mailbox_stream);

	return err;
}

/* -----------------------------------------------------------
					      Mail Search API
    -----------------------------------------------------------*/

EXPORT_API int email_query_mails(char *conditional_clause_string, emf_mail_data_t** mail_list,  int *result_count)
{
	EM_DEBUG_FUNC_BEGIN("conditional_clause_string [%s], mail_list [%p], result_count [%p]", conditional_clause_string, mail_list, result_count);

	int err = EMF_ERROR_NONE;
	emstorage_mail_tbl_t *result_mail_tbl;
	
	EM_IF_NULL_RETURN_VALUE(result_count, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(conditional_clause_string, EMF_ERROR_INVALID_PARAM);

	if (!emstorage_query_mail_tbl(conditional_clause_string, true, &result_mail_tbl, result_count, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_list failed [%d]", err);

		goto FINISH_OFF;
	}

	if(!em_convert_mail_tbl_to_mail_data(result_mail_tbl, *result_count, mail_list, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:	
	if(result_mail_tbl && !emstorage_free_mail(&result_mail_tbl, *result_count, &err))
		EM_DEBUG_EXCEPTION("emstorage_free_mail failed [%d]", err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_query_message_ex(char *conditional_clause_string, emf_mail_list_item_t** mail_list,  int *result_count)
{
	EM_DEBUG_FUNC_BEGIN("conditional_clause_string [%s], mail_list [%p], result_count [%p]", conditional_clause_string, mail_list, result_count);

	int err = EMF_ERROR_NONE;
	
	EM_IF_NULL_RETURN_VALUE(result_count, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(conditional_clause_string, EMF_ERROR_INVALID_PARAM);

	if (!emstorage_query_mail_list(conditional_clause_string, true, mail_list, result_count, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_list failed [%d]", err);

		goto FINISH_OFF;
	}

FINISH_OFF:	
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


/* -----------------------------------------------------------
					      Mail Get Info API
    -----------------------------------------------------------*/
EXPORT_API int email_get_attachment_info(emf_mailbox_t* mailbox, int mail_id, const char* attachment_id, emf_attachment_info_t** attachment)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], attachment_id[%s], attachment[%p]", mailbox, mail_id, attachment_id, attachment);
	
	int err = EMF_ERROR_NONE;
	int size = 0;
	int nSize = 0;
	char* mailbox_stream = NULL;
	char* pAttchStream = NULL;
	emf_attachment_info_t* pAttch = NULL;
	

	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(mail_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(attachment_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(attachment, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(mailbox->account_id, EMF_ERROR_INVALID_PARAM); 
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_GET_ATTACHMENT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* Mailbox */
	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	if(!mailbox_stream) {
		EM_DEBUG_EXCEPTION("em_convert_mailbox_to_byte_stream failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* mail_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	
	/* attachment_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)attachment_id, strlen(attachment_id))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	if(EMF_ERROR_NONE == err) {
		nSize = emipc_get_parameter_length(hAPI, ePARAMETER_OUT, 1);
		if(nSize > 0) {
			pAttchStream = (char*)em_malloc(nSize+1);

			if(!pAttchStream) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				err = EMF_ERROR_OUT_OF_MEMORY;		
				goto FINISH_OFF;
			}	
		
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, nSize, pAttchStream);
			em_convert_byte_stream_to_attachment_info(pAttchStream, 1, &pAttch);
		}
		
		if(!pAttch) {
			EM_DEBUG_EXCEPTION("EMF_ERROR_NULL_VALUE");
			err = EMF_ERROR_NULL_VALUE;		
			goto FINISH_OFF;
		}

		*attachment = pAttch;
	}

FINISH_OFF:
	EM_SAFE_FREE(pAttchStream);

	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;

}

EXPORT_API int email_get_attachment_data_list(int input_mail_id, emf_attachment_data_t **output_attachment_data, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_id[%d], output_attachment_data[%p], output_attachment_count[%p]", input_mail_id, output_attachment_data, output_attachment_count);
	int err = EMF_ERROR_NONE;
	
	if((err = emcore_get_attachment_data_list(input_mail_id, output_attachment_data, output_attachment_count)) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_attachment_data_list failed [%d]", err);
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_free_attachment_info(emf_attachment_info_t** atch_info)
{
	EM_DEBUG_FUNC_BEGIN("atch_info[%p]", atch_info);	
	
	int err = EMF_ERROR_NONE;

	if (!atch_info || !*atch_info)
		return EMF_ERROR_INVALID_PARAM;
	
	emf_attachment_info_t* p = *atch_info;
	emf_attachment_info_t* t;
	
	while (p)  {
		EM_SAFE_FREE(p->name);
		EM_SAFE_FREE(p->savename);
		t = p->next;
		EM_SAFE_FREE(p);
		p = t;
	}
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_free_attachment_data(emf_attachment_data_t **attachment_data_list, int attachment_data_count)
{
	EM_DEBUG_FUNC_BEGIN("attachment_data_list[%p], attachment_data_count[%d]", attachment_data_list, attachment_data_count);	
	
	int err = EMF_ERROR_NONE;

	emcore_free_attachment_data(attachment_data_list, attachment_data_count, &err);
	
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


EXPORT_API int email_get_mails(int account_id , const char *mailbox_name, int thread_id, int start_index, int limit_count, emf_sort_type_t sorting, emf_mail_data_t** mail_list,  int* result_count)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMF_ERROR_NONE;
	emstorage_mail_tbl_t *mail_tbl_list = NULL;
	EM_IF_NULL_RETURN_VALUE(result_count, EMF_ERROR_INVALID_PARAM);

	if( account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM ;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mails(account_id, (char*)mailbox_name, NULL, thread_id, start_index, limit_count, sorting, true, &mail_tbl_list, result_count, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mails failed [%d]", err);

		goto FINISH_OFF;
	}

	if(!em_convert_mail_tbl_to_mail_data(mail_tbl_list, *result_count, mail_list, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:	
	if(mail_tbl_list && !emstorage_free_mail(&mail_tbl_list, *result_count, &err))
		EM_DEBUG_EXCEPTION("emstorage_free_mail failed [%d]", err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_mail_list_ex(int account_id , const char *mailbox_name, int thread_id, int start_index, int limit_count, emf_sort_type_t sorting, emf_mail_list_item_t** mail_list,  int* result_count)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMF_ERROR_NONE;
	
	EM_IF_NULL_RETURN_VALUE(result_count, EMF_ERROR_INVALID_PARAM);

	if( account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;
	}

	if (!emstorage_get_mail_list(account_id, (char*) mailbox_name, NULL, thread_id, start_index, limit_count, 0, NULL, sorting, true, mail_list, result_count, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_list failed [%d]", err);

		goto FINISH_OFF;
	}

FINISH_OFF:	
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_find_mail (int account_id , const char *mailbox_name, int thread_id, 
	int search_type, char *search_value, int start_index, int limit_count,
	emf_sort_type_t sorting, emf_mail_list_item_t** mail_list,  int* result_count)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMF_ERROR_NONE;
	int search_num = 0;
	emf_mail_list_item_t* mail_list_item = NULL;

	EM_IF_NULL_RETURN_VALUE(mail_list, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(result_count, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(search_value, EMF_ERROR_INVALID_PARAM);

	switch ( search_type ) {
		case EMF_SEARCH_FILTER_SUBJECT:
		case EMF_SEARCH_FILTER_SENDER:
		case EMF_SEARCH_FILTER_RECIPIENT:
		case EMF_SEARCH_FILTER_ALL:
			break;
		default:
			EM_DEBUG_EXCEPTION("Invalid search filter type[%d]", search_type);
			err = EMF_ERROR_INVALID_PARAM;
			return err;
	}

	if (!emstorage_get_searched_mail_list(account_id, (char*)mailbox_name, thread_id, search_type, search_value,  start_index, limit_count, sorting, true, &mail_list_item, &search_num, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_search_mails -- Failed [%d]", err);

		goto FINISH_OFF;
	}

	*mail_list = mail_list_item;
	*result_count = search_num;
	
FINISH_OFF:	
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_mail_by_address(int account_id , const char *mailbox_name, emf_email_address_list_t* addr_list, 
									int start_index, int limit_count, int search_type, const char *search_value, emf_sort_type_t sorting, emf_mail_list_item_t** mail_list,  int* result_count)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;

	emf_mail_list_item_t* mail_list_item = NULL;
	
	EM_IF_NULL_RETURN_VALUE(mail_list, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(result_count, EMF_ERROR_INVALID_PARAM);

	if( account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("Invalid account id param");
		err = EMF_ERROR_INVALID_PARAM ;
		return err;
	}

	if (!emstorage_get_mail_list(account_id, (char*)mailbox_name, addr_list, EMF_LIST_TYPE_NORMAL, start_index, limit_count, search_type, search_value, sorting, true, &mail_list_item, result_count, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_list failed [%d]", err);

		goto FINISH_OFF;
	}

	*mail_list = mail_list_item;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_thread_information_by_thread_id(int thread_id, emf_mail_data_t** thread_info)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	emstorage_mail_tbl_t *mail_table_data = NULL;
	
	EM_IF_NULL_RETURN_VALUE(thread_info, EMF_ERROR_INVALID_PARAM);

	if (!emstorage_get_thread_information(thread_id, &mail_table_data , true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_thread_information  failed [%d]", err);
		goto FINISH_OFF;
	}

	if(!em_convert_mail_tbl_to_mail_data(mail_table_data, 1, thread_info, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:	

	if(mail_table_data && !emstorage_free_mail(&mail_table_data, 1, &err)) 
		EM_DEBUG_EXCEPTION("emstorage_free_mail failed [%d]", err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_thread_information_ex(int thread_id, emf_mail_list_item_t** thread_info)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	emstorage_mail_tbl_t *mail_table_data = NULL;
	emf_mail_list_item_t *temp_thread_info = NULL;
	
	EM_IF_NULL_RETURN_VALUE(thread_info, EMF_ERROR_INVALID_PARAM);

	if (!emstorage_get_thread_information(thread_id, &mail_table_data , true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_thread_information -- failed [%d]", err);
		goto FINISH_OFF;
	}

	temp_thread_info = em_malloc(sizeof(emf_mail_list_item_t));

	if(!temp_thread_info) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EM_SAFE_STRNCPY(temp_thread_info->mailbox_name       , mail_table_data->mailbox_name, STRING_LENGTH_FOR_DISPLAY);
	EM_SAFE_STRNCPY(temp_thread_info->from               , mail_table_data->full_address_from, STRING_LENGTH_FOR_DISPLAY);
	EM_SAFE_STRNCPY(temp_thread_info->from_email_address , mail_table_data->email_address_sender, MAX_EMAIL_ADDRESS_LENGTH);
	EM_SAFE_STRNCPY(temp_thread_info->recipients         , mail_table_data->email_address_recipient, STRING_LENGTH_FOR_DISPLAY);
	EM_SAFE_STRNCPY(temp_thread_info->subject            , mail_table_data->subject, STRING_LENGTH_FOR_DISPLAY);
	EM_SAFE_STRNCPY(temp_thread_info->previewBodyText    , mail_table_data->preview_text, MAX_PREVIEW_TEXT_LENGTH);
	temp_thread_info->mail_id                            = mail_table_data->mail_id;
	temp_thread_info->account_id                         = mail_table_data->account_id;
	temp_thread_info->date_time                          = mail_table_data->date_time;
	temp_thread_info->is_text_downloaded                 = mail_table_data->body_download_status;
	temp_thread_info->flags_seen_field                   = mail_table_data->flags_seen_field;
	temp_thread_info->priority                           = mail_table_data->priority;
	temp_thread_info->save_status                        = mail_table_data->save_status;
	temp_thread_info->is_locked                          = mail_table_data->lock_status;
	temp_thread_info->is_report_mail                     = mail_table_data->report_status;
	temp_thread_info->has_attachment                     = mail_table_data->attachment_count;
	temp_thread_info->has_drm_attachment                 = mail_table_data->DRM_status;
	temp_thread_info->thread_id                          = mail_table_data->thread_id;
	temp_thread_info->thread_item_count                  = mail_table_data->thread_item_count;
	temp_thread_info->is_meeting_request                 = mail_table_data->meeting_request_status;

	*thread_info = temp_thread_info;

FINISH_OFF:	

	if(mail_table_data)
		emstorage_free_mail(&mail_table_data, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_mail_data(int input_mail_id, emf_mail_data_t **output_mail_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	
	if ( ((err = emcore_get_mail_data(input_mail_id, output_mail_data)) != EMF_ERROR_NONE) || !output_mail_data) 
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed [%d]", err);	
		
	EM_DEBUG_FUNC_END("err [%d]", err);	
	return err;
}


/* -----------------------------------------------------------
					      Mail Flag API
    -----------------------------------------------------------*/
EXPORT_API int email_modify_mail_flag(int mail_id, emf_mail_flag_t new_flag, int onserver)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], on_server [ %d] ", mail_id, onserver);
	
	int err = EMF_ERROR_NONE;

	int i_flag, sticky = 0;
		
	if ( mail_id <= 0 || (onserver != 0 && onserver != 1) ) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;			
	}
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_MODIFY_MAIL_FLAG);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* Mail ID */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* new_flag */
	if(!em_convert_mail_flag_to_int(new_flag, &i_flag, &err))  {
		EM_DEBUG_EXCEPTION("em_convert_mail_flag_to_int failed ");
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(i_flag), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	sticky = new_flag.sticky;

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(sticky), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* onserver  */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(onserver), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}




EXPORT_API int email_modify_seen_flag(int *mail_ids, int num, int seen_flag, int onserver)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], num[%d],seen_flag[%d], on_server [ %d]", mail_ids, num, seen_flag, onserver);
	EM_DEBUG_FUNC_END("EMF_ERROR_NOT_IMPLEMENTED");
	return EMF_ERROR_NOT_IMPLEMENTED;
}

EXPORT_API int email_set_flags_field(int account_id, int *mail_ids, int num, emf_flags_field_type field_type, int value, int onserver)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], mail_ids[%p], num[%d], field_type [%d], seen_flag[%d], on_server [ %d]", account_id, mail_ids, num, field_type, value, onserver);
	
	int err = EMF_ERROR_NONE;

		
	EM_IF_NULL_RETURN_VALUE(mail_ids, EMF_ERROR_INVALID_PARAM);
	if (account_id == 0 || num <= 0 || (onserver != 0 && onserver != 1)) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		return EMF_ERROR_INVALID_PARAM;			
	}
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SET_FLAGS_FIELD);
	
	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}
	
	/* account_id*/
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Number of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&num, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* set of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)mail_ids, num * sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* field_type */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&field_type, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* value */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&value, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* onserver  */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(onserver), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_modify_extra_mail_flag(int mail_id, emf_extra_flag_t new_flag)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d]", mail_id);
	
	int size = 0;
	int err = EMF_ERROR_NONE;
	char* pMailExtraFlagsStream = NULL;

		
	EM_IF_NULL_RETURN_VALUE(mail_id, EMF_ERROR_INVALID_PARAM);

	pMailExtraFlagsStream = em_convert_extra_flags_to_byte_stream(new_flag, &size);
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_MODIFY_MAIL_EXTRA_FLAG);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* Mail ID */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	/*  Flag */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, pMailExtraFlagsStream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;
		goto FINISH_OFF;
	} 

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	EM_SAFE_FREE(pMailExtraFlagsStream);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;

}



/* -----------------------------------------------------------
					      Mail Move API
    -----------------------------------------------------------*/
EXPORT_API int email_move_mail_to_mailbox(int *mail_ids, int num, emf_mailbox_t* mailbox)
{
	EM_DEBUG_FUNC_BEGIN("mail_ids[%p], num [%d], mailbox[%p]",  mail_ids, num, mailbox);
	
	int size = 0;
	char* mailbox_stream =  NULL;
	int err = EMF_ERROR_NONE;
	HIPC_API hAPI = NULL;


	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(mail_ids, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(mailbox->account_id, EMF_ERROR_INVALID_PARAM); 
	
	if (num <= 0)  {
		EM_DEBUG_LOG("num = %d", num);
		err = EMF_ERROR_INVALID_PARAM;
		return err;
	}
	
	hAPI = emipc_create_email_api(_EMAIL_API_MOVE_MAIL);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}
	
	/* Number of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&num, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* set of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)mail_ids, num * sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Mailbox */
	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	if(!mailbox_stream) {
		EM_DEBUG_EXCEPTION("em_convert_mailbox_to_byte_stream failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:	 	
	EM_SAFE_FREE(mailbox_stream); 

	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


EXPORT_API int  email_move_all_mails_to_mailbox(emf_mailbox_t* src_mailbox, emf_mailbox_t* new_mailbox)
{
	EM_DEBUG_FUNC_BEGIN("src_mailbox[%p] , new_mailbox[%p]",  src_mailbox, new_mailbox);
	
	int size = 0;
	int err = EMF_ERROR_NONE;
	char *dest_mailbox_stream = NULL;
	char *source_mailbox_stream = NULL;
	HIPC_API hAPI = NULL;
	

	EM_IF_NULL_RETURN_VALUE(src_mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(new_mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(new_mailbox->account_id, EMF_ERROR_INVALID_PARAM); 
	EM_IF_ACCOUNT_ID_NULL(src_mailbox->account_id, EMF_ERROR_INVALID_PARAM); 
	
	hAPI = emipc_create_email_api(_EMAIL_API_MOVE_ALL_MAIL);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}
	
	/* Src Mailbox Information */
	source_mailbox_stream = em_convert_mailbox_to_byte_stream(src_mailbox, &size);

	if(!source_mailbox_stream) {
		EM_DEBUG_EXCEPTION("em_convert_mailbox_to_byte_stream failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, source_mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	size = 0;
	
	/* Dst Mailbox Information */
	dest_mailbox_stream = em_convert_mailbox_to_byte_stream(new_mailbox, &size);

	if(!dest_mailbox_stream) {
		EM_DEBUG_EXCEPTION("em_convert_mailbox_to_byte_stream failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, dest_mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	EM_SAFE_FREE(source_mailbox_stream);
	EM_SAFE_FREE(dest_mailbox_stream); 

	if(hAPI)
		emipc_destroy_email_api(hAPI);
	
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
 


EXPORT_API int email_count_message_with_draft_flag(emf_mailbox_t* mailbox, int* total)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], total[%p]", mailbox, total);
	
	int err = EMF_ERROR_NONE;
	int total_count = 0;
		
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(total, EMF_ERROR_INVALID_PARAM);

    if (!emstorage_get_mail_count_with_draft_flag(mailbox->account_id,  mailbox->name, &total_count, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_count Failed");

	} else {
		*total = total_count;
	}
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


EXPORT_API int email_count_message_on_sending(emf_mailbox_t* mailbox, int* total)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], total[%p]", mailbox, total);
	int err = EMF_ERROR_NONE;
	int total_count = 0;
	
		
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(total, EMF_ERROR_INVALID_PARAM);

   	if (!emstorage_get_mail_count_on_sending(mailbox->account_id,  mailbox->name, &total_count, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_count Failed");

	} else
		*total = total_count;
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

/**
 
 * @open
 * @fn email_get_mail_flag(int account_id, int mail_id, emf_mail_flag_t* mail_flag)
 * @brief	Get the Mail Flag information based on the account id and Mail Id.
 * 
 * @param[in] account_id	Specifies the Account ID
 * @param[in] mail_id		Specifies the Mail id for which  Flag details need to be fetched
 * @param[in/out] mail_flag	Specifies the Pointer to the structure emf_mail_flag_t.
 * @remarks N/A
 * @return True on Success, False on Failure.
 */
EXPORT_API int email_get_mail_flag(int account_id, int mail_id, emf_mail_flag_t* mail_flag)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMF_ERROR_NONE;

	if (account_id < FIRST_ACCOUNT_ID|| mail_id < 1 || mail_flag == NULL) {
		EM_DEBUG_EXCEPTION("Invalid Param");
		err = EMF_ERROR_INVALID_PARAM ;
		goto FINISH_OFF;
	}

	/* Fetch the flag Information */
	if (!emcore_fetch_flags(account_id, mail_id, mail_flag, &err)) {	
		EM_DEBUG_EXCEPTION("emcore_fetch_flags Failed [%d]", err);
		goto FINISH_OFF;
	}
FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;

}

 
EXPORT_API int email_free_mail_data(emf_mail_data_t** mail_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("mail_list[%p], count[%d]", mail_list, count);
	int err = EMF_ERROR_NONE;
	emcore_free_mail_data(mail_list, count, &err);
	EM_DEBUG_FUNC_END("err [%d]", err);	
	return err;
}

/* Convert Modified UTF-7 mailbox name to UTF-8 */
/* returns modified UTF-8 Name if success else NULL */

EXPORT_API int email_cancel_send_mail( int mail_id)
{
	EM_DEBUG_FUNC_BEGIN("Mail ID [ %d]", mail_id);
	EM_IF_NULL_RETURN_VALUE(mail_id, EMF_ERROR_INVALID_PARAM);
	
	int err = EMF_ERROR_NONE;
	int account_id = 0;
	emf_mail_data_t* mail_data = NULL;

	HIPC_API hAPI = NULL;
	
	
	if ((err = emcore_get_mail_data(mail_id, &mail_data)) != EMF_ERROR_NONE)  {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!mail_data) {
		EM_DEBUG_EXCEPTION("mail_data is null");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	account_id = mail_data->account_id;

	hAPI = emipc_create_email_api(_EMAIL_API_SEND_MAIL_CANCEL_JOB);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* Account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Mail ID */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	if  ( !emcore_free_mail_data(&mail_data, 1, &err))
		EM_DEBUG_EXCEPTION("emcore_free_mail_data Failed [%d ] ", err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;

}
/**
  * EmfSendCancel - Callback function for cm popup. We set the status as EMF_MAIL_STATUS_NONE 
  *
  **/


EXPORT_API int email_retry_send_mail( int mail_id, int timeout_in_sec)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMF_ERROR_NONE;


	EM_IF_NULL_RETURN_VALUE(mail_id, EMF_ERROR_INVALID_PARAM);
	if( timeout_in_sec < 0 )  {
		EM_DEBUG_EXCEPTION("Invalid timeout_in_sec");
		err = EMF_ERROR_INVALID_PARAM;
		return err;
	}
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SEND_RETRY);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* Mail ID */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* timeout */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(timeout_in_sec), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;

 }

EXPORT_API int email_get_mailbox_name_by_mail_id(int mail_id, char **pMailbox_name)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMF_ERROR_NONE;
	char* mailbox_name = NULL;
	emstorage_mail_tbl_t* mail_table_data = NULL;
	
	if(mail_id <= 0) {
		EM_DEBUG_EXCEPTION("mail_id is not valid");
		err= EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}	
	EM_IF_NULL_RETURN_VALUE(pMailbox_name, EMF_ERROR_INVALID_PARAM);
	
	if(!emstorage_get_mail_by_id(mail_id, &mail_table_data, true, &err)) {
		EM_DEBUG_EXCEPTION("Failed to get mail by mail_id [%d]", err);

		goto FINISH_OFF;
	}

	if(mail_table_data->mailbox_name)
		mailbox_name = strdup(mail_table_data->mailbox_name);
	
	*pMailbox_name = mailbox_name;

FINISH_OFF:
	if(mail_table_data) {
		emstorage_free_mail(&mail_table_data, 1, &err);	

	}
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


EXPORT_API int email_get_latest_unread_mail_id(int account_id, int *pMailID)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMF_ERROR_NONE;

	if( (!pMailID) ||(account_id <= 0 &&  account_id != -1)) {		
		err = EMF_ERROR_INVALID_PARAM;
		return err;
	}
	if(!emstorage_get_latest_unread_mailid(account_id,pMailID, &err)) {
		EM_DEBUG_LOG("emstorage_get_latest_unread_mailid - failed");

	}
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_max_mail_count(int *Count)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	EM_IF_NULL_RETURN_VALUE(Count, EMF_ERROR_INVALID_PARAM);
	*Count = emstorage_get_max_mail_count();
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}



/* for setting application,disk usage of email in KB */
EXPORT_API int email_get_disk_space_usage(unsigned long *total_size)
{
	EM_DEBUG_FUNC_BEGIN("total_size[%p]", total_size);
	int err = EMF_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(total_size, EMF_ERROR_INVALID_PARAM);

	if (!emstorage_mail_get_total_diskspace_usage(total_size,true,&err))  {
		EM_DEBUG_EXCEPTION("emstorage_mail_get_total_diskspace_usage failed [%d]", err);

		goto FINISH_OFF;
	}
FINISH_OFF :	
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_recipients_list(int account_id, const char *mailbox_name, emf_sender_list_t **recipients_list)
{
	EM_DEBUG_FUNC_BEGIN("recipients_list[%p]",  recipients_list);

	int number_of_mails, index;
	int number_of_recipients;
	int ret=0, err = 0;

	emf_sender_list_t *temp_recipients_list = NULL;
	emf_sender_list_t *p_recipients_list = NULL;
	GList *addr_list = NULL, *temp_addr_list = NULL;
	emstorage_mail_tbl_t *mail_table_data = NULL;

	if (!emstorage_get_mails(account_id, (char*)mailbox_name, NULL, EMF_LIST_TYPE_NORMAL, -1, -1, EMF_SORT_SENDER_HIGH, true, &mail_table_data, &number_of_mails, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mails failed");

		goto FINISH_OFF;
	}

	for (index = 0; index < number_of_mails; index++) {
		addr_list = emcore_get_recipients_list(addr_list, mail_table_data[index].full_address_to, &err);
		addr_list = emcore_get_recipients_list(addr_list, mail_table_data[index].full_address_cc, &err);
		addr_list = emcore_get_recipients_list(addr_list, mail_table_data[index].full_address_bcc, &err);
	}

	number_of_recipients = g_list_length(addr_list);

	p_recipients_list = (emf_sender_list_t *)malloc(sizeof(emf_sender_list_t) * number_of_recipients);
	if (p_recipients_list == NULL) {
		EM_DEBUG_EXCEPTION("malloc for emf_sender_list_t failed...");
		err = EMF_ERROR_OUT_OF_MEMORY;
		ret = err;
		goto FINISH_OFF;
	}	
	memset(p_recipients_list, 0x00, sizeof(emf_sender_list_t) * number_of_recipients);
	
	temp_addr_list = g_list_first(addr_list);
	index = 0;
	while (temp_addr_list != NULL) {
		temp_recipients_list = (emf_sender_list_t *)temp_addr_list->data;
		p_recipients_list[index].address = temp_recipients_list->address;
		p_recipients_list[index].display_name = temp_recipients_list->display_name;
		p_recipients_list[index].total_count = temp_recipients_list->total_count + 1;
		EM_DEBUG_LOG("address[%s], display_name[%s], total_count[%d]", p_recipients_list[index].address, p_recipients_list[index].display_name, p_recipients_list[index].total_count);
		temp_addr_list = g_list_next(temp_addr_list);
		index++;
	}

	ret = true;	
FINISH_OFF:
	if (ret == true && recipients_list)
		*recipients_list = p_recipients_list; else if (p_recipients_list != NULL) {
		email_free_sender_list(&p_recipients_list, number_of_recipients);
	}

	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int email_get_sender_list(int account_id, const char *mailbox_name, int search_type, const char *search_value, emf_sort_type_t sorting, emf_sender_list_t** sender_list, int *sender_count)
{
	EM_DEBUG_FUNC_BEGIN("sender_list[%p],sender_count[%p], sorting[%d]",  sender_list, sender_count, sorting);

	int err = EMF_ERROR_NONE;

	emf_sender_list_t *temp_sender_list = NULL;

	EM_IF_NULL_RETURN_VALUE(sender_list, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(sender_count, EMF_ERROR_INVALID_PARAM);
	if( account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION(" Invalid Account Id Param ");
		err = EMF_ERROR_INVALID_PARAM ;
		return err;
	}
	
	if ( !emstorage_get_sender_list(account_id, mailbox_name, search_type, search_value, sorting, &temp_sender_list, sender_count, &err) ) { 
		EM_DEBUG_EXCEPTION("emstorage_get_sender_list failed [%d]", err);

		goto FINISH_OFF;
	}

	if ( sender_list )
		*sender_list = temp_sender_list;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_sender_list_ex(int account_id, const char *mailbox_name, int start_index, int limit_count, emf_sort_type_t sorting, emf_sender_list_t** sender_list, int *sender_count)
{
	return EMF_ERROR_NONE;
}

EXPORT_API int email_free_sender_list(emf_sender_list_t **sender_list, int count)
{
	EM_DEBUG_FUNC_BEGIN("sender_list[%p], count[%d]", sender_list, count);

	int err = EMF_ERROR_NONE;
	
	if (count > 0)  {
		if (!sender_list || !*sender_list)  {
			EM_DEBUG_EXCEPTION("sender_list[%p], count[%d]", sender_list, count);			
			err = EMF_ERROR_INVALID_PARAM;
			return err;
		}
		
		emf_sender_list_t* p = *sender_list;
		int i = 0;
		
		for (; i < count; i++)  {
			EM_SAFE_FREE(p[i].address);
			EM_SAFE_FREE(p[i].display_name);
		}
		
		EM_SAFE_FREE(p); 
		*sender_list = NULL;
	}	
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_address_info_list(int mail_id, emf_address_info_list_t** address_info_list)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], address_info_list[%p]", mail_id, address_info_list);

	int err = EMF_ERROR_NONE;

	emf_address_info_list_t *temp_address_info_list = NULL;

	EM_IF_NULL_RETURN_VALUE(address_info_list, EMF_ERROR_INVALID_PARAM);
	if( mail_id <= 0) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM ;
		return err;
	}
	
	if ( !emcore_get_mail_address_info_list(mail_id, &temp_address_info_list, &err) ) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_address_info_list failed [%d]", err);

		goto FINISH_OFF;
	}

	if ( address_info_list ) {
		*address_info_list = temp_address_info_list;
		temp_address_info_list = NULL;
	}

FINISH_OFF:
	if ( temp_address_info_list )
		emstorage_free_address_info_list(&temp_address_info_list);
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_free_address_info_list(emf_address_info_list_t **address_info_list)
{
	EM_DEBUG_FUNC_BEGIN("address_info_list[%p]", address_info_list);

	int err = EMF_ERROR_NONE;

	if ( (err = emstorage_free_address_info_list(address_info_list)) != EMF_ERROR_NONE ) {
		EM_DEBUG_EXCEPTION("address_info_list[%p] free failed.", address_info_list);
	}
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_structure(const char*encoded_string, void **struct_var, emf_convert_struct_type_e type)
{
	EM_DEBUG_FUNC_BEGIN("encoded_string[%s], struct_var[%p], type[%d]", encoded_string, struct_var, type);

	int err = EMF_ERROR_NONE;
	void * temp_struct = NULL;

	EM_IF_NULL_RETURN_VALUE(encoded_string, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(struct_var, EMF_ERROR_INVALID_PARAM);

	if ( (err = emcore_convert_string_to_structure((char*)encoded_string, &temp_struct, type)) != EMF_ERROR_NONE )  {
		EM_DEBUG_EXCEPTION("emcore_convert_string_to_structure failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( struct_var )
		*struct_var = temp_struct;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_get_meeting_request(int mail_id, emf_meeting_request_t **meeting_req)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d],meeting_req[%p]",  mail_id, meeting_req);

	int err = EMF_ERROR_NONE;

	emf_meeting_request_t *temp_meeting_req = NULL;

	EM_IF_NULL_RETURN_VALUE(meeting_req, EMF_ERROR_INVALID_PARAM);
	if( mail_id <= 0 ) {
		EM_DEBUG_EXCEPTION(" Invalid Mail Id Param ");
		err = EMF_ERROR_INVALID_PARAM ;
		return err;
	}
	
	if ( !emstorage_get_meeting_request(mail_id, &temp_meeting_req, 1, &err) ) {
		EM_DEBUG_EXCEPTION("emstorage_get_meeting_request -- Failed [%d]", err);

		goto FINISH_OFF;
	}

	if ( meeting_req )
		*meeting_req = temp_meeting_req;

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_free_meeting_request(emf_meeting_request_t** meeting_req, int count)
{
	EM_DEBUG_FUNC_BEGIN("meeting_req[%p], count[%d]", meeting_req, count);

	int err = EMF_ERROR_NONE;

	emstorage_free_meeting_request(meeting_req, count, &err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_move_thread_to_mailbox(int thread_id, char *target_mailbox_name, int move_always_flag)
{
	EM_DEBUG_FUNC_BEGIN("thread_id[%d], target_mailbox_name[%p], move_always_flag[%d]", thread_id, target_mailbox_name, move_always_flag);
	int err = EMF_ERROR_NONE;
	

	EM_IF_NULL_RETURN_VALUE(target_mailbox_name, EMF_ERROR_INVALID_PARAM);
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_MOVE_THREAD_TO_MAILBOX);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* thread_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&thread_id, sizeof(int))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* target mailbox information */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, target_mailbox_name, strlen(target_mailbox_name))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* move_always_flag */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&move_always_flag, sizeof(int))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_delete_thread(int thread_id, int delete_always_flag)
{
	EM_DEBUG_FUNC_BEGIN("thread_id[%d], delete_always_flag[%d]", thread_id, delete_always_flag);
	int err = EMF_ERROR_NONE;

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_DELETE_THREAD);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* thread_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&thread_id, sizeof(int))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* delete_always_flag */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&delete_always_flag, sizeof(int))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

EXPORT_API int email_modify_seen_flag_of_thread(int thread_id, int seen_flag, int on_server)
{
	EM_DEBUG_FUNC_BEGIN("thread_id[%d], seen_flag[%d], on_server[%d]", thread_id, seen_flag, on_server);
	int err = EMF_ERROR_NONE;
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_MODIFY_SEEN_FLAG_OF_THREAD);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMF_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* thread_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&thread_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* seen_flag */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&seen_flag, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* on_server */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&on_server, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMF_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMF_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

