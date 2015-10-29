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
 * @file		email-api-mail.c
 * @brief 		This file contains the data structures and interfaces of Message related Functionality provided by 
 *			email-service . 
 */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "email-ipc.h"
#include "email-api-init.h"
#include "email-api-mailbox.h"
#include "email-convert.h"
#include "email-core-tasks.h"
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-smtp.h"
#include "email-core-account.h"
#include "email-core-task-manager.h"
#include "email-storage.h"
#include "email-core-signal.h"
#include "email-utilities.h"
#include "email-core-smime.h"

#define  DIR_SEPERATOR_CH '/'

#define _DIRECT_TO_DB

#define MAX_SEARCH_LEN 1000

EXPORT_API int email_add_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas)
{
	EM_DEBUG_API_BEGIN ("input_mail_data[%p] input_attachment_data_list[%p] input_attachment_count[%d] input_meeting_request[%p] input_from_eas[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);

	EM_IF_NULL_RETURN_VALUE(input_mail_data,               EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mail_data->account_id,   EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mail_data->mailbox_id,   EMAIL_ERROR_INVALID_PARAM);

	if(input_attachment_count > 0 && !input_attachment_data_list) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int       err = EMAIL_ERROR_NONE;
	int      *ret_nth_value = NULL;
	int       size = 0;
	char	 *rfc822_file = NULL;
    char     *multi_user_name = NULL;
	char     *mail_data_stream = NULL;
	char     *attachment_data_list_stream  = NULL;
	char     *meeting_request_stream = NULL;
	HIPC_API  hAPI = NULL;

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if(input_from_eas == 0) { /* native emails */
		if ((input_mail_data->smime_type != EMAIL_SMIME_NONE) && (input_mail_data->mailbox_type != EMAIL_MAILBOX_TYPE_DRAFT)) {
			if (!emcore_make_rfc822_file(multi_user_name, input_mail_data, input_attachment_data_list, input_attachment_count, true, &rfc822_file, &err)) {
				EM_DEBUG_EXCEPTION("emcore_make_rfc822_file failed [%d]", err);
				goto FINISH_OFF;
			}
		
			input_mail_data->file_path_mime_entity = EM_SAFE_STRDUP(emcore_set_mime_entity(rfc822_file));
			
			emstorage_delete_file(rfc822_file, NULL);
			EM_SAFE_FREE(rfc822_file);	
		}

		hAPI = emipc_create_email_api(_EMAIL_API_ADD_MAIL);

		if(!hAPI) {
			EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
			err = EMAIL_ERROR_NULL_VALUE;		
			goto FINISH_OFF;
		}

		/* email_mail_data_t */
		mail_data_stream = em_convert_mail_data_to_byte_stream(input_mail_data, &size);

		if(!mail_data_stream) {
			EM_DEBUG_EXCEPTION("em_convert_mail_data_to_byte_stream failed");
			err = EMAIL_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}

		if (!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, mail_data_stream, size))
			EM_DEBUG_EXCEPTION("emipc_add_dynamic_parameter failed");
		
		/* email_attachment_data_t */
		attachment_data_list_stream = em_convert_attachment_data_to_byte_stream(input_attachment_data_list, input_attachment_count, &size);

		if((size > 0) && !emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, attachment_data_list_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_dynamic_parameter failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/*  email_meeting_request_t */
		if ( input_mail_data->meeting_request_status != EMAIL_MAIL_TYPE_NORMAL ) {
			meeting_request_stream = em_convert_meeting_req_to_byte_stream(input_meeting_request, &size);

			if(!meeting_request_stream) {
				EM_DEBUG_EXCEPTION("em_convert_meeting_req_to_byte_stream failed");
				err = EMAIL_ERROR_NULL_VALUE;
				goto FINISH_OFF;
			}

			if (!emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, meeting_request_stream, size))
				EM_DEBUG_EXCEPTION("emipc_add_dynamic_parameter failed");
		}

		/* input_from_eas */
		emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_from_eas, sizeof(int));

		/* Execute API */
		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
			goto FINISH_OFF;
		}

		/* get result from service */
		if ((ret_nth_value = (int*)emipc_get_nth_parameter_data(hAPI, ePARAMETER_OUT, 0))) {
			err = *ret_nth_value;
			
			if (err == EMAIL_ERROR_NONE) {
				if ((ret_nth_value = (int *)emipc_get_nth_parameter_data(hAPI, ePARAMETER_OUT, 1))) {
					input_mail_data->mail_id = *ret_nth_value;
				} else {
					err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
					goto FINISH_OFF;
				}
				
				if ((ret_nth_value = (int *)emipc_get_nth_parameter_data(hAPI, ePARAMETER_OUT, 2))) {
					input_mail_data->thread_id = *ret_nth_value;
				} else {
					err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
					goto FINISH_OFF;
				}
			}
		} else {
			err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
			goto FINISH_OFF;
		}
	} else { /* take care of mails from eas */
		err = emcore_add_mail(multi_user_name, input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas, false);
		if(err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_add_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:

	if(hAPI) 
		emipc_destroy_email_api(hAPI);

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_API_END ("err[%d]", err);
	return err;

}

EXPORT_API int email_add_read_receipt(int input_read_mail_id, int *output_receipt_mail_id)
{
	EM_DEBUG_API_BEGIN ("input_read_mail_id[%d] output_receipt_mail_id[%p]", input_read_mail_id, output_receipt_mail_id);

	EM_IF_NULL_RETURN_VALUE(output_receipt_mail_id, EMAIL_ERROR_INVALID_PARAM);

	int      err = EMAIL_ERROR_NONE;
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_ADD_READ_RECEIPT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &input_read_mail_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("Add Param mail body Fail");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_NULL_VALUE);
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	EM_DEBUG_LOG("err [%d]", err);

	if(err == EMAIL_ERROR_NONE) {
		/* Get receipt mail id */
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), output_receipt_mail_id);
		EM_DEBUG_LOG("output_receipt_mail_id [%d]", *output_receipt_mail_id);
	}

	emipc_destroy_email_api(hAPI);

	hAPI = NULL;

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}


#define TMP_BODY_PATH "/tmp/UTF-8"
EXPORT_API int email_create_db_full()
{
	int mailbox_index, mail_index, mailbox_count, mail_slot_size;
	int account_id = 0;
	emstorage_mail_tbl_t mail_table_data = {0};
	email_mailbox_t *mailbox_list = NULL;
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	FILE *body_file = NULL;

	if ( (err = email_open_db()) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_open_db failed [%d]", err);
		return err;
	}

	if ((err = emcore_load_default_account_id(NULL, &account_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_load_default_account_id failed : [%d]", err);
		goto FINISH_OFF;
	}

	err = em_fopen(TMP_BODY_PATH, "w", &body_file);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_fopen failed : [%s][%d]", TMP_BODY_PATH, err);
		goto FINISH_OFF;
	}

	for (i = 0; i < 10; i++)
		fprintf(body_file, "Dummy test. [line:%d]\n", i);


	fflush(body_file);

	mail_table_data.subject = (char*) em_malloc(50); 
	mail_table_data.full_address_from = strdup("<dummy_from@nowhere.com>");
	mail_table_data.full_address_to = strdup("<dummy_to@nowhere.com>");
	mail_table_data.account_id = account_id;
	mail_table_data.file_path_plain = strdup(TMP_BODY_PATH);
	mail_table_data.body_download_status = 1;

	if( (err = email_get_mailbox_list_ex(1, -1, 0, &mailbox_list, &mailbox_count)) < EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_get_mailbox_list_ex failed [%d]", err);
		goto FINISH_OFF;
	}

	for(mailbox_index = 0; mailbox_index < mailbox_count; mailbox_index++) {
		mail_slot_size= mailbox_list[mailbox_index].mail_slot_size;
		for(mail_index = 0; mail_index < mail_slot_size; mail_index++) {
			sprintf(mail_table_data.subject, "Subject #%d",mail_index);
			mail_table_data.mailbox_id   = mailbox_list[mailbox_index].mailbox_id;
			mail_table_data.mailbox_type = mailbox_list[mailbox_index].mailbox_type;

			if( !emstorage_add_mail(NULL, &mail_table_data, 1, true, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_add_mail failed [%d]",err);
		
				goto FINISH_OFF;
			}
		}
	}

FINISH_OFF:
	if ( (err = email_close_db()) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_close_db failed [%d]", err);
	}	

	if(body_file) fclose(body_file); /*prevent 39446*/
	
	if(mailbox_list)
		email_free_mailbox(&mailbox_list, mailbox_count);

	EM_SAFE_FREE(mail_table_data.subject);
	EM_SAFE_FREE(mail_table_data.full_address_from);
	EM_SAFE_FREE(mail_table_data.full_address_to);
	EM_SAFE_FREE(mail_table_data.file_path_plain);

	return err;
}

EXPORT_API int email_update_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas)
{
	EM_DEBUG_API_BEGIN ("input_mail_data[%p] input_attachment_data_list[%p] input_attachment_count[%d] input_meeting_request[%p] input_from_eas[%d]", input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas);

	EM_IF_NULL_RETURN_VALUE(input_mail_data,               EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mail_data->account_id,   EMAIL_ERROR_INVALID_PARAM);

	if(input_attachment_count > 0 && !input_attachment_data_list) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int       err = EMAIL_ERROR_NONE;
	int       size = 0;
	char     *mail_data_stream = NULL;
	char     *attachment_data_list_stream  = NULL;
	char     *meeting_request_stream = NULL;

	HIPC_API  hAPI = NULL;
	
	if(input_from_eas == 0) {
		hAPI = emipc_create_email_api(_EMAIL_API_UPDATE_MAIL);	

		if(!hAPI) {
			EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
			err = EMAIL_ERROR_NULL_VALUE;		
			goto FINISH_OFF;
		}

		/* email_mail_data_t */
		mail_data_stream = em_convert_mail_data_to_byte_stream(input_mail_data, &size);

		if(!mail_data_stream) {
			EM_DEBUG_EXCEPTION("em_convert_mail_data_to_byte_stream failed");
			err = EMAIL_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}
		
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mail_data_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter for head failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		/* email_attachment_data_t */
		attachment_data_list_stream = em_convert_attachment_data_to_byte_stream(input_attachment_data_list, input_attachment_count, &size);
		if ((size > 0) && !emipc_add_dynamic_parameter(hAPI, ePARAMETER_IN, attachment_data_list_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_dynamic_parameter failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/*  email_meeting_request_t */
		if (input_mail_data->meeting_request_status != EMAIL_MAIL_TYPE_NORMAL) {
			meeting_request_stream = em_convert_meeting_req_to_byte_stream(input_meeting_request, &size);

			if(!meeting_request_stream) {
				EM_DEBUG_EXCEPTION("em_convert_meeting_req_to_byte_stream failed");
				err = EMAIL_ERROR_NULL_VALUE;
				goto FINISH_OFF;
			}

			if(!emipc_add_parameter(hAPI, ePARAMETER_IN, meeting_request_stream, size)) {
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
		}

		/* input_from_eas */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_from_eas, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/* Execute API */
		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
			goto FINISH_OFF;
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		
		if(err == EMAIL_ERROR_NONE) {
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &input_mail_data->mail_id);
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 2, sizeof(int), &input_mail_data->thread_id);
		}
	} 
	else {
		if( (err = emcore_update_mail(NULL, input_mail_data, input_attachment_data_list, input_attachment_count, input_meeting_request, input_from_eas)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_update_mail failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	if(hAPI) 
		emipc_destroy_email_api(hAPI);

	EM_SAFE_FREE(mail_data_stream);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_update_mail_attribute(int input_account_id, int *input_mail_id_array, int input_mail_id_count, email_mail_attribute_type input_attribute_type, email_mail_attribute_value_t input_value)
{
	EM_DEBUG_API_BEGIN ("input_account_id[%d] input_mail_id_array[%p] input_mail_id_count[%d] input_attribute_type[%d]", input_account_id, input_mail_id_array, input_mail_id_count, input_attribute_type);

	int err = EMAIL_ERROR_NONE;
	task_parameter_EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE task_parameter;
	email_mail_attribute_value_type value_type;

	if(input_mail_id_count <= 0 || input_mail_id_array == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if((err = emcore_get_mail_attribute_value_type(input_attribute_type, &value_type)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_attribute_value_type failed [%d]", err);
		goto FINISH_OFF;
	}

	task_parameter.account_id     = input_account_id;
	task_parameter.mail_id_count  = input_mail_id_count;
	task_parameter.mail_id_array  = input_mail_id_array;
	task_parameter.attribute_type = input_attribute_type;
	task_parameter.value          = input_value;

	if(value_type == EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING)
		task_parameter.value_length = EM_SAFE_STRLEN(input_value.string_type_value);
	else
		task_parameter.value_length = 4;

	if((err = emipc_execute_proxy_task(EMAIL_SYNC_TASK_UPDATE_ATTRIBUTE, &task_parameter)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("execute_proxy_task failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_clear_mail_data()
{
	EM_DEBUG_API_BEGIN ();
	int err = EMAIL_ERROR_NONE;
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_CLEAR_DATA);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);
	
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api Fail");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMAIL_ERROR_IPC_SOCKET_FAILURE);
	}
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}


EXPORT_API int email_count_mail(email_list_filter_t *input_filter_list, int input_filter_count, int *output_total_mail_count, int *output_unseen_mail_count)
{
	EM_DEBUG_API_BEGIN ("input_filter_list[%p] input_filter_count[%d] output_total_mail_count[%p] output_unseen_mail_count[%p]", input_filter_list, input_filter_count, output_total_mail_count, output_unseen_mail_count);
	
	int total_count = 0;
	int unread = 0;
	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	char *conditional_clause_string = NULL;

	EM_IF_NULL_RETURN_VALUE(input_filter_list, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_filter_count, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(output_total_mail_count, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(output_unseen_mail_count, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if ((err = emstorage_write_conditional_clause_for_getting_mail_list(multi_user_name, input_filter_list, input_filter_count, NULL, 0, -1, -1, &conditional_clause_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_write_conditional_clause_for_getting_mail_list failed[%d]", err);
		goto FINISH_OFF;
	}

	if ((err = emstorage_query_mail_count(multi_user_name, conditional_clause_string, true, &total_count, &unread)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_count failed [%d]", err);
		goto FINISH_OFF;
	}

	*output_total_mail_count = total_count;
	*output_unseen_mail_count = unread;

FINISH_OFF:

    EM_SAFE_FREE(multi_user_name);
	EM_SAFE_FREE (conditional_clause_string); /* detected by valgrind */
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}


EXPORT_API int email_delete_mail(int input_mailbox_id, int *input_mail_ids, int input_num, int input_from_server)
{
	EM_DEBUG_API_BEGIN ("input_mailbox_id[%d] input_mail_ids[%p] input_num[%d] input_from_server[%d]", input_mailbox_id, input_mail_ids, input_num, input_from_server);

	int err = EMAIL_ERROR_NONE;
	HIPC_API hAPI = NULL;

	EM_IF_NULL_RETURN_VALUE(input_mail_ids, EMAIL_ERROR_INVALID_PARAM);
	
	if (input_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("input_mailbox_id = %d", input_mailbox_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	if (input_num <= 0) {
		EM_DEBUG_EXCEPTION("input_num = %d", input_num);
		err = EMAIL_ERROR_INVALID_PARAM;		
		return err;
	}

	hAPI = emipc_create_email_api(_EMAIL_API_DELETE_MAIL);
	
	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* mailbox id*/
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_mailbox_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Number of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_num, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* set of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)input_mail_ids, input_num * sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* from-server */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_from_server, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}


EXPORT_API int email_delete_all_mails_in_mailbox(int input_mailbox_id, int input_from_server)
{
	EM_DEBUG_API_BEGIN ("input_mailbox_id[%d] input_from_server[%d]", input_mailbox_id, input_from_server);

	int err = EMAIL_ERROR_NONE;
	HIPC_API hAPI = NULL;
	
	if(input_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("input_mailbox_id = %d", input_mailbox_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}
	
	hAPI = emipc_create_email_api(_EMAIL_API_DELETE_ALL_MAIL);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}
	
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_mailbox_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_from_server, sizeof(int))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int),&err );

FINISH_OFF:

	if(hAPI)	
		emipc_destroy_email_api(hAPI);
	
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_add_attachment(int mail_id, email_attachment_data_t* attachment)
{
	EM_DEBUG_API_BEGIN ("mail_id[%d] attachment[%p]", mail_id, attachment);
	int err = EMAIL_ERROR_NONE;
	char *attchment_stream = NULL;
	int size = 0;
	HIPC_API hAPI = NULL;
	
	EM_IF_NULL_RETURN_VALUE(mail_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(attachment, EMAIL_ERROR_INVALID_PARAM);
	
	hAPI = emipc_create_email_api(_EMAIL_API_ADD_ATTACHMENT);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* mail_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Attachment */
	attchment_stream = em_convert_attachment_data_to_byte_stream(attachment, 1, &size);

	if(!attchment_stream) {
		EM_DEBUG_EXCEPTION("em_convert_attachment_data_to_byte_stream failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, attchment_stream, size)){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	
	if(EMAIL_ERROR_NONE == err) {
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &attachment->attachment_id);
	}

FINISH_OFF:
	EM_SAFE_FREE(attchment_stream);
	
	if(hAPI)
		emipc_destroy_email_api(hAPI);
	
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
 }


EXPORT_API int email_delete_attachment(int attachment_id)
{
	EM_DEBUG_API_BEGIN ("attachment_id[%d]", attachment_id);
	int err = EMAIL_ERROR_NONE;
	HIPC_API hAPI = NULL;
	
	EM_IF_NULL_RETURN_VALUE(attachment_id, EMAIL_ERROR_INVALID_PARAM);
	
	hAPI = emipc_create_email_api(_EMAIL_API_DELETE_ATTACHMENT);
	
	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* attachment_index */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&attachment_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

/* -----------------------------------------------------------
					      Mail Search API
    -----------------------------------------------------------*/

EXPORT_API int email_query_mails(char *conditional_clause_string, email_mail_data_t** mail_list,  int *result_count)
{
	EM_DEBUG_API_BEGIN ("conditional_clause_string[%s] mail_list[%p] result_count[%p]", conditional_clause_string, mail_list, result_count);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	emstorage_mail_tbl_t *result_mail_tbl = NULL;
	
	EM_IF_NULL_RETURN_VALUE(result_count, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(conditional_clause_string, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed :[%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_query_mail_tbl(multi_user_name, conditional_clause_string, true, &result_mail_tbl, result_count, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if(!em_convert_mail_tbl_to_mail_data(result_mail_tbl, *result_count, mail_list, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:	

	if(result_mail_tbl && !emstorage_free_mail(&result_mail_tbl, *result_count, NULL))
		EM_DEBUG_EXCEPTION("emstorage_free_mail failed");

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_query_mail_list(char *input_conditional_clause_string, email_mail_list_item_t** output_mail_list,  int *output_result_count)
{
	EM_DEBUG_API_BEGIN ("input_conditional_clause_string[%s] output_mail_list[%p] output_result_count[%p]", input_conditional_clause_string, output_mail_list, output_result_count);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	
	EM_IF_NULL_RETURN_VALUE(input_conditional_clause_string, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(output_result_count, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed :[%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_query_mail_list(multi_user_name, input_conditional_clause_string, true, output_mail_list, output_result_count, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_list failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:	

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_API_END ("err [%d]", err);
	return err;
}


/* -----------------------------------------------------------
					      Mail Get Info API
    -----------------------------------------------------------*/
EXPORT_API int email_get_attachment_data(int attachment_id, email_attachment_data_t** attachment)
{
	EM_DEBUG_API_BEGIN ("attachment_id[%d] attachment[%p]", attachment_id, attachment);
	
	int err = EMAIL_ERROR_NONE;
	int nSize = 0;
	int attachment_count = 0;
	char* attchment_stream = NULL;
	email_attachment_data_t* attachment_data = NULL;

	EM_IF_NULL_RETURN_VALUE(attachment_id, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(attachment, EMAIL_ERROR_INVALID_PARAM);

	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_GET_ATTACHMENT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMAIL_ERROR_NULL_VALUE);

	/* attachment_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&attachment_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	if(EMAIL_ERROR_NONE == err) {
		nSize = emipc_get_parameter_length(hAPI, ePARAMETER_OUT, 1);
		if(nSize > 0) {
			attchment_stream = (char*)em_malloc(nSize+1);

			if(!attchment_stream) {
				EM_DEBUG_EXCEPTION("em_malloc failed");
				err = EMAIL_ERROR_OUT_OF_MEMORY;		
				goto FINISH_OFF;
			}	
		
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, nSize, attchment_stream);
			em_convert_byte_stream_to_attachment_data(attchment_stream, nSize, &attachment_data, &attachment_count);
		}
		
		if(!attachment_data) {
			EM_DEBUG_EXCEPTION("EMAIL_ERROR_NULL_VALUE");
			err = EMAIL_ERROR_NULL_VALUE;		
			goto FINISH_OFF;
		}

		*attachment = attachment_data;
	}

FINISH_OFF:
	EM_SAFE_FREE(attchment_stream);

	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;

}

EXPORT_API int email_get_attachment_data_list(int input_mail_id, email_attachment_data_t **output_attachment_data, int *output_attachment_count)
{
	EM_DEBUG_API_BEGIN ("input_mail_id[%d] output_attachment_data[%p] output_attachment_count[%p]", input_mail_id, output_attachment_data, output_attachment_count);
	int   err             = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        return err;
    }

	if((err = emcore_get_attachment_data_list(multi_user_name, input_mail_id, output_attachment_data, output_attachment_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_get_attachment_data_list failed [%d]", err);
	}

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_free_attachment_data(email_attachment_data_t **attachment_data_list, int attachment_data_count)
{
	EM_DEBUG_FUNC_BEGIN ("attachment_data_list[%p] attachment_data_count[%d]", attachment_data_list, attachment_data_count);	
	
	int err = EMAIL_ERROR_NONE;

	emcore_free_attachment_data(attachment_data_list, attachment_data_count, &err);
	
	EM_DEBUG_FUNC_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_mail_list_ex(email_list_filter_t *input_filter_list, int input_filter_count, email_list_sorting_rule_t *input_sorting_rule_list, int input_sorting_rule_count, int input_start_index, int input_limit_count, email_mail_list_item_t** output_mail_list, int *output_result_count)
{
	EM_DEBUG_FUNC_BEGIN ("input_filter_list[%p] input_filter_count[%d] input_sorting_rule_list[%p] input_sorting_rule_count[%d] input_start_index[%d] input_limit_count[%d] output_mail_list[%p] output_result_count[%p]", input_filter_list, input_filter_count, input_sorting_rule_list, input_sorting_rule_count, input_start_index, input_limit_count, output_mail_list, output_result_count);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	char *conditional_clause_string = NULL;

	EM_IF_NULL_RETURN_VALUE(output_mail_list, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(output_result_count, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if( (err = emstorage_write_conditional_clause_for_getting_mail_list(multi_user_name, input_filter_list, input_filter_count, input_sorting_rule_list, input_sorting_rule_count, input_start_index, input_limit_count, &conditional_clause_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_write_conditional_clause_for_getting_mail_list failed[%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_DEV ("conditional_clause_string[%s].", conditional_clause_string);

	if(!emstorage_query_mail_list(multi_user_name, conditional_clause_string, true, output_mail_list, output_result_count, &err)) {
		if (err != EMAIL_ERROR_MAIL_NOT_FOUND) /* there is no mail and it is definitely common */
			EM_DEBUG_EXCEPTION("emstorage_query_mail_list [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_SAFE_FREE(conditional_clause_string);
    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_FUNC_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_free_list_filter(email_list_filter_t **input_filter_list, int input_filter_count)
{
	EM_DEBUG_FUNC_BEGIN ("input_filter_list[%p] input_filter_count[%d]", input_filter_list, input_filter_count);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(input_filter_list, EMAIL_ERROR_INVALID_PARAM);

	err = emstorage_free_list_filter(input_filter_list, input_filter_count);

	EM_DEBUG_FUNC_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_mails(int account_id , int mailbox_id, int thread_id, int start_index, int limit_count, email_sort_type_t sorting, email_mail_data_t** mail_list,  int* result_count)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] mailbox_id[%d] thread_id[%d]", account_id, mailbox_id, thread_id);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	emstorage_mail_tbl_t *mail_tbl_list = NULL;
	EM_IF_NULL_RETURN_VALUE(result_count, EMAIL_ERROR_INVALID_PARAM);

	if( account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM ;
		goto FINISH_OFF;
	}

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_mails(multi_user_name, account_id, mailbox_id, NULL, thread_id, start_index, limit_count, sorting, true, &mail_tbl_list, result_count, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mails failed [%d]", err);
		goto FINISH_OFF;
	}

	if(!em_convert_mail_tbl_to_mail_data(mail_tbl_list, *result_count, mail_list, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:	

	if(mail_tbl_list && !emstorage_free_mail(&mail_tbl_list, *result_count, NULL))
		EM_DEBUG_EXCEPTION("emstorage_free_mail failed");

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_mail_list(int account_id , int mailbox_id, int thread_id, int start_index, int limit_count, email_sort_type_t sorting, email_mail_list_item_t** mail_list,  int* result_count)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] mailbox_id[%d] thread_id[%d]", account_id, mailbox_id, thread_id);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	
	EM_IF_NULL_RETURN_VALUE(result_count, EMAIL_ERROR_INVALID_PARAM);

	if( account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_mail_list(multi_user_name, account_id, mailbox_id, NULL, thread_id, start_index, limit_count, 0, NULL, sorting, true, mail_list, result_count, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_list failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:	

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_mail_by_address(int account_id , int mailbox_id, email_email_address_list_t* addr_list,
									int start_index, int limit_count, int search_type, const char *search_value, email_sort_type_t sorting, email_mail_list_item_t** mail_list,  int* result_count)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] mailbox_id[%d]", account_id, mailbox_id);
	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	email_mail_list_item_t* mail_list_item = NULL;
	
	EM_IF_NULL_RETURN_VALUE(mail_list, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(result_count, EMAIL_ERROR_INVALID_PARAM);

	if( account_id < ALL_ACCOUNT) {
		EM_DEBUG_EXCEPTION("Invalid account id param");
		err = EMAIL_ERROR_INVALID_PARAM ;
		return err;
	}

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_mail_list(multi_user_name, account_id, mailbox_id, addr_list, EMAIL_LIST_TYPE_NORMAL, start_index, limit_count, search_type, search_value, sorting, true, &mail_list_item, result_count, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_list failed [%d]", err);
		goto FINISH_OFF;
	}

	*mail_list = mail_list_item;

FINISH_OFF:

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_thread_information_by_thread_id(int thread_id, email_mail_data_t** thread_info)
{
	EM_DEBUG_API_BEGIN ("thread_id[%d]", thread_id);
	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	emstorage_mail_tbl_t *mail_table_data = NULL;
	
	EM_IF_NULL_RETURN_VALUE(thread_info, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_thread_information(multi_user_name, thread_id, &mail_table_data , true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_thread_information  failed [%d]", err);
		goto FINISH_OFF;
	}

	if(!em_convert_mail_tbl_to_mail_data(mail_table_data, 1, thread_info, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_tbl_to_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:	

	if(mail_table_data && !emstorage_free_mail(&mail_table_data, 1, NULL))
		EM_DEBUG_EXCEPTION("emstorage_free_mail failed");

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_thread_information_ex(int thread_id, email_mail_list_item_t** thread_info)
{
	EM_DEBUG_API_BEGIN ("thread_id[%d]", thread_id);
	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	emstorage_mail_tbl_t *mail_table_data = NULL;
	email_mail_list_item_t *temp_thread_info = NULL;
	
	EM_IF_NULL_RETURN_VALUE(thread_info, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emstorage_get_thread_information(multi_user_name, thread_id, &mail_table_data , true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_thread_information -- failed [%d]", err);
		goto FINISH_OFF;
	}

	temp_thread_info = em_malloc(sizeof(email_mail_list_item_t));

	if(!temp_thread_info) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	EM_SAFE_STRNCPY(temp_thread_info->full_address_from       , mail_table_data->full_address_from, STRING_LENGTH_FOR_DISPLAY);
	EM_SAFE_STRNCPY(temp_thread_info->email_address_sender    , mail_table_data->email_address_sender, MAX_EMAIL_ADDRESS_LENGTH);
	EM_SAFE_STRNCPY(temp_thread_info->email_address_recipient , mail_table_data->email_address_recipient, STRING_LENGTH_FOR_DISPLAY);
	EM_SAFE_STRNCPY(temp_thread_info->subject                 , mail_table_data->subject, STRING_LENGTH_FOR_DISPLAY);
	EM_SAFE_STRNCPY(temp_thread_info->preview_text            , mail_table_data->preview_text, MAX_PREVIEW_TEXT_LENGTH);
	temp_thread_info->mail_id                                 = mail_table_data->mail_id;
	temp_thread_info->mailbox_id                              = mail_table_data->mailbox_id;
	temp_thread_info->mailbox_type                            = mail_table_data->mailbox_type;
	temp_thread_info->account_id                              = mail_table_data->account_id;
	temp_thread_info->date_time                               = mail_table_data->date_time;
	temp_thread_info->body_download_status                    = mail_table_data->body_download_status;
	temp_thread_info->flags_seen_field                        = mail_table_data->flags_seen_field;
	temp_thread_info->priority                                = mail_table_data->priority;
	temp_thread_info->save_status                             = mail_table_data->save_status;
	temp_thread_info->lock_status                             = mail_table_data->lock_status;
	temp_thread_info->report_status                           = mail_table_data->report_status;
	temp_thread_info->attachment_count                        = mail_table_data->attachment_count;
	temp_thread_info->DRM_status                              = mail_table_data->DRM_status;
	temp_thread_info->thread_id                               = mail_table_data->thread_id;
	temp_thread_info->thread_item_count                       = mail_table_data->thread_item_count;
	temp_thread_info->meeting_request_status                  = mail_table_data->meeting_request_status;

	*thread_info = temp_thread_info;

FINISH_OFF:	

	if(mail_table_data)
		emstorage_free_mail(&mail_table_data, 1, NULL);

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_mail_data(int input_mail_id, email_mail_data_t **output_mail_data)
{
	EM_DEBUG_API_BEGIN ("input_mail_id[%d]", input_mail_id);
	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        return err;
    }

	if ( ((err = emcore_get_mail_data(multi_user_name, input_mail_id, output_mail_data)) != EMAIL_ERROR_NONE) || !output_mail_data) 
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed [%d]", err);	
	
    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);	
	return err;
}


/* -----------------------------------------------------------
					      Mail Flag API
    -----------------------------------------------------------*/

EXPORT_API int email_modify_seen_flag(int *mail_ids, int num, int seen_flag, int onserver)
{
	EM_DEBUG_API_BEGIN ("mail_ids[%p] num[%d] seen_flag[%d] on_server[%d]", mail_ids, num, seen_flag, onserver);
	EM_DEBUG_API_END ("EMAIL_ERROR_NOT_IMPLEMENTED");
	return EMAIL_ERROR_NOT_IMPLEMENTED;
}

EXPORT_API int email_set_flags_field(int account_id, int *mail_ids, int num, email_flags_field_type field_type, int value, int onserver)
{
	EM_DEBUG_API_BEGIN ("account_id[%d] mail_ids[%p] num[%d] field_type[%d] seen_flag[%d] on_server[%d]", account_id, mail_ids, num, field_type, value, onserver);
	
	int err = EMAIL_ERROR_NONE;

		
	EM_IF_NULL_RETURN_VALUE(mail_ids, EMAIL_ERROR_INVALID_PARAM);
	if (account_id == 0 || num <= 0 || (onserver != 0 && onserver != 1)) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;			
	}
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SET_FLAGS_FIELD);
	
	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}
	
	/* account_id*/
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Number of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&num, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* set of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)mail_ids, num * sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* field_type */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&field_type, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* value */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&value, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* onserver  */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(onserver), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}


/* -----------------------------------------------------------
					      Mail Move API
    -----------------------------------------------------------*/
EXPORT_API int email_move_mail_to_mailbox(int *mail_ids, int num, int input_target_mailbox_id)
{
	EM_DEBUG_API_BEGIN ("mail_ids[%p] num[%d] input_target_mailbox_id[%d]",  mail_ids, num, input_target_mailbox_id);
	
	int err = EMAIL_ERROR_NONE;
	HIPC_API hAPI = NULL;

	if(input_target_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("input_target_mailbox_id is invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}
	EM_IF_NULL_RETURN_VALUE(mail_ids, EMAIL_ERROR_INVALID_PARAM);
	
	if (num <= 0)  {
		EM_DEBUG_LOG("num = %d", num);
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	hAPI = emipc_create_email_api(_EMAIL_API_MOVE_MAIL);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}
	
	/* Number of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&num, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* set of mail_ids */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)mail_ids, num * sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* input_target_mailbox_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_target_mailbox_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:	 	
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}


EXPORT_API int  email_move_all_mails_to_mailbox(int input_source_mailbox_id, int input_target_mailbox_id)
{
	EM_DEBUG_API_BEGIN ("input_source_mailbox_id[%d] input_target_mailbox_id[%d]", input_source_mailbox_id, input_target_mailbox_id);
	
	int err = EMAIL_ERROR_NONE;
	HIPC_API hAPI = NULL;
	
	if(input_source_mailbox_id <= 0 || input_target_mailbox_id <= 0) {
		EM_DEBUG_EXCEPTION("mailbox_id is invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}
	
	hAPI = emipc_create_email_api(_EMAIL_API_MOVE_ALL_MAIL);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}
	
	/* input_source_mailbox_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_source_mailbox_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* input_target_mailbox_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_target_mailbox_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;		
		goto FINISH_OFF;
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);
	
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_move_mails_to_mailbox_of_another_account(int input_source_mailbox_id, int *input_mail_id_array, int input_mail_id_count, int input_target_mailbox_id, int *output_task_id)
{
	EM_DEBUG_API_BEGIN ("input_source_mailbox_id[%d] input_mail_id_array[%p] input_mail_id_count[%d] input_target_mailbox_id[%d] output_task_id[%p]",  input_source_mailbox_id, input_mail_id_array, input_mail_id_count, input_target_mailbox_id, output_task_id);

	int err = EMAIL_ERROR_NONE;
	task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT task_parameter;

	if(input_source_mailbox_id <= 0 || input_target_mailbox_id <= 0 || input_mail_id_array == NULL || input_mail_id_count == 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	task_parameter.source_mailbox_id = input_source_mailbox_id;
	task_parameter.mail_id_array     = input_mail_id_array;
	task_parameter.mail_id_count     = input_mail_id_count;
	task_parameter.target_mailbox_id = input_target_mailbox_id;

	if((err = emipc_execute_proxy_task(EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT, &task_parameter)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("execute_proxy_task failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}
 
EXPORT_API int email_free_mail_data(email_mail_data_t** mail_list, int count)
{
	EM_DEBUG_FUNC_BEGIN ("mail_list[%p] count[%d]", mail_list, count);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(mail_list, EMAIL_ERROR_INVALID_PARAM);

	emcore_free_mail_data_list(mail_list, count);
	EM_DEBUG_FUNC_END ("err[%d]", err);	
	return err;
}

/* Convert Modified UTF-7 mailbox name to UTF-8 */
/* returns modified UTF-8 Name if success else NULL */

EXPORT_API int email_cancel_sending_mail(int mail_id)
{
	EM_DEBUG_API_BEGIN ("mail_id[%d]", mail_id);
	EM_IF_NULL_RETURN_VALUE(mail_id, EMAIL_ERROR_INVALID_PARAM);
	
	int err = EMAIL_ERROR_NONE;
	int account_id = 0;
    char *multi_user_name = NULL;
	email_mail_data_t* mail_data = NULL;
	email_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	
    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_container_name error : [%d]", err);
        goto FINISH_OFF;
    }
	
	if (((err = emcore_get_mail_data(multi_user_name, mail_id, &mail_data)) != EMAIL_ERROR_NONE) || mail_data == NULL)  {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed [%d]", err);
		goto FINISH_OFF;
	}

	account_id = mail_data->account_id;

	/*  check account bind type and branch off  */
	if ( em_get_account_server_type_by_account_id(multi_user_name, account_id, &account_server_type, false, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle;
		ASNotiData as_noti_data;

		if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.cancel_sending_mail.handle          = as_handle;
		as_noti_data.cancel_sending_mail.mail_id         = mail_id;
		as_noti_data.cancel_sending_mail.account_id      = account_id;
		as_noti_data.cancel_sending_mail.multi_user_name = EM_SAFE_STRDUP(multi_user_name);

		if ( em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_CANCEL_SENDING_MAIL, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("em_send_notification_to_active_sync_engine failed.");
			err = EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}
	}
	else {

		hAPI = emipc_create_email_api(_EMAIL_API_SEND_MAIL_CANCEL_JOB);

		if(!hAPI) {
			EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
			err = EMAIL_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}

		/* Account_id */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/* Mail ID */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	
		/* Execute API */
		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
			goto FINISH_OFF;
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	}

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	if(mail_data)
		emcore_free_mail_data_list(&mail_data, 1);

	EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;

}

EXPORT_API int email_retry_sending_mail(int mail_id, int timeout_in_sec)
{
	EM_DEBUG_API_BEGIN ("mail_id[%d]",mail_id);
	
	int err = EMAIL_ERROR_NONE;


	EM_IF_NULL_RETURN_VALUE(mail_id, EMAIL_ERROR_INVALID_PARAM);
	if( timeout_in_sec < 0 )  {
		EM_DEBUG_EXCEPTION("Invalid timeout_in_sec");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SEND_RETRY);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* Mail ID */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* timeout */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(timeout_in_sec), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;

}

EXPORT_API int email_get_max_mail_count(int *Count)
{
	EM_DEBUG_API_BEGIN ();
	int err = EMAIL_ERROR_NONE;
	EM_IF_NULL_RETURN_VALUE(Count, EMAIL_ERROR_INVALID_PARAM);
	*Count = emstorage_get_max_mail_count();
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}



/* for setting application,disk usage of email in KB */
EXPORT_API int email_get_disk_space_usage(unsigned long *total_size)
{
	EM_DEBUG_API_BEGIN ("total_size[%p]", total_size);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(total_size, EMAIL_ERROR_INVALID_PARAM);

	if (!emstorage_mail_get_total_diskspace_usage(total_size,true,&err))  {
		EM_DEBUG_EXCEPTION("emstorage_mail_get_total_diskspace_usage failed [%d]", err);

		goto FINISH_OFF;
	}
FINISH_OFF :	
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_address_info_list(int mail_id, email_address_info_list_t** address_info_list)
{
	EM_DEBUG_FUNC_BEGIN ("mail_id[%d] address_info_list[%p]", mail_id, address_info_list);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;

	email_address_info_list_t *temp_address_info_list = NULL;

	EM_IF_NULL_RETURN_VALUE(address_info_list, EMAIL_ERROR_INVALID_PARAM);
	if( mail_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM ;
		return err;
	}

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if (!emcore_get_mail_address_info_list(multi_user_name, mail_id, &temp_address_info_list, &err)) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_address_info_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if (address_info_list) {
		*address_info_list = temp_address_info_list;
		temp_address_info_list = NULL;
	}

FINISH_OFF:

	if (temp_address_info_list)
		emstorage_free_address_info_list(&temp_address_info_list);

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_FUNC_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_free_address_info_list(email_address_info_list_t **address_info_list)
{
	EM_DEBUG_FUNC_BEGIN ("address_info_list[%p]", address_info_list);

	int err = EMAIL_ERROR_NONE;

	if ( (err = emstorage_free_address_info_list(address_info_list)) != EMAIL_ERROR_NONE ) {
		EM_DEBUG_EXCEPTION("address_info_list[%p] free failed.", address_info_list);
	}
	EM_DEBUG_FUNC_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_query_meeting_request(char *input_conditional_clause_string, email_meeting_request_t **output_meeting_req, int *output_count)
{
	EM_DEBUG_API_BEGIN ("input_conditional_clause_string[%s] output_meeting_req[%p] output_count[%p]", input_conditional_clause_string, output_meeting_req, output_count);
	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;

	EM_IF_NULL_RETURN_VALUE(input_conditional_clause_string, EMAIL_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(output_count, EMAIL_ERROR_INVALID_PARAM);

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        return err;
    }

	if ((err = emstorage_query_meeting_request(multi_user_name, input_conditional_clause_string, output_meeting_req, output_count, true)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_meeting_request failed [%d]", err);
	}

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_get_meeting_request(int mail_id, email_meeting_request_t **meeting_req)
{
	EM_DEBUG_API_BEGIN ("mail_id[%d] meeting_req[%p]", mail_id, meeting_req);

	int err = EMAIL_ERROR_NONE;
    char *multi_user_name = NULL;
	email_meeting_request_t *temp_meeting_req = NULL;

	EM_IF_NULL_RETURN_VALUE(meeting_req, EMAIL_ERROR_INVALID_PARAM);
	if (mail_id <= 0) {
		EM_DEBUG_EXCEPTION(" Invalid Mail Id Param ");
		err = EMAIL_ERROR_INVALID_PARAM ;
		return err;
	}

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        return err;
    }

	if (!emstorage_get_meeting_request(multi_user_name, mail_id, &temp_meeting_req, 1, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_meeting_request failed[%d]", err);
		goto FINISH_OFF;
	}

	if (meeting_req)
		*meeting_req = temp_meeting_req;

FINISH_OFF:

    EM_SAFE_FREE(multi_user_name);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_free_meeting_request(email_meeting_request_t** meeting_req, int count)
{
	EM_DEBUG_FUNC_BEGIN ("meeting_req[%p] count[%d]", meeting_req, count);
	if( !meeting_req || !*meeting_req ) {
		EM_DEBUG_EXCEPTION("NULL PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	int i = 0;
	email_meeting_request_t *cur = *meeting_req;
	for ( ; i < count ; i++ )
		emstorage_free_meeting_request(cur + i);

	EM_SAFE_FREE(*meeting_req);

	EM_DEBUG_FUNC_END ();
	return EMAIL_ERROR_NONE;
}

EXPORT_API int email_move_thread_to_mailbox(int thread_id, int target_mailbox_id, int move_always_flag)
{
	EM_DEBUG_API_BEGIN ("thread_id[%d] target_mailbox_id[%d] move_always_flag[%d]", thread_id, target_mailbox_id, move_always_flag);
	int err = EMAIL_ERROR_NONE;

	EM_IF_NULL_RETURN_VALUE(target_mailbox_id, EMAIL_ERROR_INVALID_PARAM);
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_MOVE_THREAD_TO_MAILBOX);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* thread_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&thread_id, sizeof(int))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* target mailbox information */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&target_mailbox_id, sizeof(int))){
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
	}

	/* move_always_flag */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&move_always_flag, sizeof(int))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_delete_thread(int thread_id, int delete_always_flag)
{
	EM_DEBUG_API_BEGIN ("thread_id[%d] delete_always_flag[%d]", thread_id, delete_always_flag);
	int err = EMAIL_ERROR_NONE;

	if (thread_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
                return err;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_DELETE_THREAD);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* thread_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&thread_id, sizeof(int))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* delete_always_flag */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&delete_always_flag, sizeof(int))){
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:

	if(hAPI)
		emipc_destroy_email_api(hAPI);

	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_modify_seen_flag_of_thread(int thread_id, int seen_flag, int on_server)
{
	EM_DEBUG_API_BEGIN ("thread_id[%d] seen_flag[%d] on_server[%d]", thread_id, seen_flag, on_server);
	int err = EMAIL_ERROR_NONE;

	if (thread_id <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
                return err;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_MODIFY_SEEN_FLAG_OF_THREAD);

	if(!hAPI) {
		EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
		err = EMAIL_ERROR_NULL_VALUE;		
		goto FINISH_OFF;
	}

	/* thread_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&thread_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* seen_flag */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&seen_flag, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}

	/* on_server */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&on_server, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;		
		goto FINISH_OFF;
	}
	
	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		err = EMAIL_ERROR_IPC_SOCKET_FAILURE;	
		goto FINISH_OFF;
	}
	
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

FINISH_OFF:
	if(hAPI)
		emipc_destroy_email_api(hAPI);
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

EXPORT_API int email_expunge_mails_deleted_flagged(int input_mailbox_id, int input_on_server, int *output_handle)
{
	EM_DEBUG_API_BEGIN ("input_mailbox_id[%d] input_on_server[%d] output_handle[%p]", input_mailbox_id, input_on_server, output_handle);
	int err = EMAIL_ERROR_NONE;
	int return_value = 0;
    char *multi_user_name = NULL;
	HIPC_API hAPI = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl_data = NULL;

    if ((err = emipc_get_user_name(&multi_user_name)) != EMAIL_ERROR_NONE) {
        EM_DEBUG_EXCEPTION("emipc_get_user_name failed : [%d]", err);
        goto FINISH_OFF;
    }

	if( (err = emstorage_get_mailbox_by_id(multi_user_name, input_mailbox_id, &mailbox_tbl_data)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_id failed[%d]", err);
		goto FINISH_OFF;
	}

	email_account_server_t account_server_type = EMAIL_SERVER_TYPE_NONE;
	ASNotiData as_noti_data;

	memset(&as_noti_data, 0, sizeof(ASNotiData)); /* initialization of union members */

	if ( em_get_account_server_type_by_account_id(multi_user_name, mailbox_tbl_data->account_id, &account_server_type, true, &err) == false ) {
		EM_DEBUG_EXCEPTION("em_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( account_server_type == EMAIL_SERVER_TYPE_ACTIVE_SYNC && input_on_server == true) {
		int as_handle = 0;

		if ( em_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("em_get_handle_for_activesync failed[%d].", err);
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.expunge_mails_deleted_flagged.handle          = as_handle;
		as_noti_data.expunge_mails_deleted_flagged.account_id      = mailbox_tbl_data->account_id;
		as_noti_data.expunge_mails_deleted_flagged.mailbox_id      = input_mailbox_id;
		as_noti_data.expunge_mails_deleted_flagged.on_server       = input_on_server;
		as_noti_data.expunge_mails_deleted_flagged.multi_user_name = multi_user_name;

		return_value = em_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_EXPUNGE_MAILS_DELETED_FLAGGED, &as_noti_data);

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
		hAPI = emipc_create_email_api(_EMAIL_API_EXPUNGE_MAILS_DELETED_FLAGGED);

		if(!hAPI) {
			EM_DEBUG_EXCEPTION("emipc_create_email_api failed");
			err = EMAIL_ERROR_NULL_VALUE;
			goto FINISH_OFF;
		}

		/* input_mailbox_id */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_mailbox_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/* input_on_server */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&input_on_server, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		/* Execute API */
		if(emipc_execute_proxy_api(hAPI) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			err = EMAIL_ERROR_IPC_SOCKET_FAILURE;
			goto FINISH_OFF;
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		if (err == EMAIL_ERROR_NONE) {
			if(output_handle)
				emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), output_handle);
		}
	}

FINISH_OFF:

	if(hAPI)
		emipc_destroy_email_api(hAPI);

	if(mailbox_tbl_data) {
		emstorage_free_mailbox(&mailbox_tbl_data, 1, NULL);
	}

    EM_SAFE_FREE(multi_user_name);
	EM_DEBUG_API_END ("err[%d]", err);
	return err;
}

