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


/* Email service Main .c */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <heynoti/heynoti.h>
#include <signal.h>
#include <time.h>

#include "emflib.h"
#include "Emf_Mapi.h"
#include "Msg_Convert.h"
#include "ipc-library.h"
#include "emf-dbglog.h"
#include "emf-auto-poll.h"   
#include "emf-account.h"   
#include "em-core-types.h" 
#include "em-core-account.h" 
#include "em-core-mesg.h" 
#include "em-core-event.h" 
#include "em-core-global.h" 
#include "em-core-mailbox.h" 
#include "em-core-utils.h"
#include "em-storage.h"    
#include "db-util.h"
#include "emf-emn-noti.h"
#include "emf-emn-storage.h"

void stb_account_create(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int local_result = 0;
	char* local_account_stream = NULL;
	emf_account_t account;
	int err = EMF_ERROR_NONE;

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);

	if(buffer_size <= 0)	{
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_account_stream = (char*)em_core_malloc(buffer_size+1);
	if(!local_account_stream) {
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	memcpy(local_account_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
	/* Convert account stream to structure */
	EM_DEBUG_LOG("Before Convert_ByteStream_To_Account");
	em_convert_byte_stream_to_account(local_account_stream, &account);
	EM_SAFE_FREE(local_account_stream);

	if (account.account_name)
		EM_DEBUG_LOG("Account name - %s", account.account_name);
	if(account.email_addr)
		EM_DEBUG_LOG("Email Address - %s", account.email_addr);
	
	if(!emf_account_create(&account, &err)) {
		EM_DEBUG_EXCEPTION("emf_account_create fail ");
		goto FINISH_OFF;
	}

#ifdef __FEATURE_AUTO_POLLING__
	/* start auto polling, if check_interval not zero */
	if(account.check_interval > 0) {
		if(!emf_add_polling_alarm( account.account_id,account.check_interval))
			EM_DEBUG_EXCEPTION("emf_add_polling_alarm[NOTI_ACCOUNT_ADD] : start auto poll failed >>> ");
	}
#endif
	/* add account details to contact DB */
	emf_account_insert_accountinfo_to_contact(&account);
	local_result = 1;
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
	
	EM_DEBUG_LOG("[3] APPID[%d], APIID [%d]", ipcEmailAPI_GetAPPID(a_hAPI), ipcEmailAPI_GetAPIID(a_hAPI));
	EM_DEBUG_LOG("account id[%d]", account.account_id);
	
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &(account.account_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

FINISH_OFF:
	if ( local_result == 0 ) {			
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed : local_result ");
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed : err");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}
	EM_DEBUG_FUNC_END();
}

void stb_account_delete(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id = 0;
	int local_result = 0;
	int err = EMF_ERROR_NONE;
	
	/* account_id */
	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));;
	if(!emf_account_delete(account_id, &err)) {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_LOG("ipcEmailAPI_AddParameter failed ");
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
				
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
		goto FINISH_OFF;
	}

#ifdef __FEATURE_AUTO_POLLING__
	/* stop auto polling for this acount */
	if(is_auto_polling_started(account_id)) {
		if(!emf_remove_polling_alarm(account_id))
			EM_DEBUG_EXCEPTION("emf_remove_polling_alarm[ NOTI_ACCOUNT_DELETE] : remove auto poll failed >>> ");
	}
#endif
	local_result = 1;

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
			
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");

FINISH_OFF:

	EM_DEBUG_FUNC_END();
}

void stb_account_update(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id = 0, buffer_size = 0, local_result = 0, with_validation = 0;
	char* AccountStream = NULL;
	emf_account_t  *new_account_info = NULL;
	int err = EMF_ERROR_NONE;
	unsigned int handle = 0;  

	/* filter_id */
	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));;
	
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	
	if(buffer_size > 0)	 {
		AccountStream = (char*)em_core_malloc(buffer_size+1);
		if(AccountStream) {
			memcpy(AccountStream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), buffer_size);
			/* Convert Byte stream to account structure */
			EM_DEBUG_LOG("Before Convert_ByteStream_To_account");
			new_account_info = (emf_account_t*)em_core_malloc(sizeof(emf_account_t));

			if(NULL == new_account_info)			 {
				EM_SAFE_FREE(AccountStream);
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_convert_byte_stream_to_account(AccountStream, new_account_info);
			EM_SAFE_FREE(AccountStream);
		}
		else {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
			
		memcpy(&with_validation, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), sizeof(int));
	}

	if ( NULL == new_account_info ) {
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	emf_account_t  *old_account_info = NULL;
	
	if(!emf_account_get(account_id, GET_FULL_DATA, &old_account_info, &err)) {
		EM_DEBUG_EXCEPTION("emf_account_get failed ");
		goto FINISH_OFF;
	}

	if(new_account_info->password == NULL || (new_account_info->password != NULL && strlen(new_account_info->password) == 0) ) {
		EM_SAFE_FREE(new_account_info->password);
		if(old_account_info->password) {
			new_account_info->password = strdup(old_account_info->password);
			if(new_account_info->password == NULL) {
				EM_DEBUG_EXCEPTION("allocation for new_account_info->password failed");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
		}
	}
	
	if(new_account_info->sending_password == NULL || (new_account_info->sending_password != NULL && strlen(new_account_info->sending_password) == 0) ) {
		if(new_account_info->sending_password != NULL)
			free(new_account_info->sending_password);
		if(old_account_info->sending_password) {
			new_account_info->sending_password = strdup(old_account_info->sending_password);
			if(new_account_info->sending_password == NULL) {
				EM_DEBUG_EXCEPTION("allocation for new_account_info->sending_password failed");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
		}
	}

	
	if(with_validation) {
		emf_account_validate_and_update(account_id, new_account_info, &handle, &err);
		local_result = 1;
	}
	else {
		if(emf_account_modify(account_id, new_account_info, &err)) {
#ifdef __FEATURE_AUTO_POLLING__
			int  old_check_interval = old_account_info->check_interval;
			if( old_check_interval == 0 && new_account_info->check_interval > 0) {
				if(!emf_add_polling_alarm( account_id,new_account_info->check_interval))
					EM_DEBUG_EXCEPTION("emf_add_polling_alarm[ CHANGEACCOUNT] : start auto poll failed >>> ");
				
			}
			else if( (old_check_interval > 0) && (new_account_info->check_interval == 0)) {
				if(!emf_remove_polling_alarm(account_id))
					EM_DEBUG_EXCEPTION("emf_remove_polling_alarm[ CHANGEACCOUNT] : start auto poll failed >>> ");
			}
			else if(old_check_interval != new_account_info->check_interval) {
				if(!emf_remove_polling_alarm(account_id)) {
					EM_DEBUG_EXCEPTION("emf_remove_polling_alarm[ CHANGEACCOUNT] : start auto poll failed >>> ");
					goto FINISH_OFF;
				}		
				if(!emf_add_polling_alarm( account_id,new_account_info->check_interval))
					EM_DEBUG_EXCEPTION("emf_add_polling_alarm[ CHANGEACCOUNT] : start auto poll failed >>> ");
			}
#endif
			emf_account_update_accountinfo_to_contact(old_account_info,new_account_info);
			local_result = 1;
		}
		else {
			EM_DEBUG_EXCEPTION("emf_account_modify failed [%d]", err);
			goto FINISH_OFF;
		}
		
	}
	
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter for result failed");

	if(with_validation) {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter for handle failed");
	}
		
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");

FINISH_OFF:
	if ( local_result == 0 ) {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");

		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
				
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");
	}
	EM_SAFE_FREE(new_account_info);		
	EM_SAFE_FREE(old_account_info);
	EM_DEBUG_FUNC_END();
}

void stb_account_validate(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();

	int account_id = 0;
	int local_result = 0;
	int err_code = 0;
	int handle = 0;
	
	/* account_id */
	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));;
	local_result = emf_account_validate(account_id, (unsigned*)&handle,&err_code);

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_LOG("ipcEmailAPI_AddParameter failed ");

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err_code, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
				
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter result failed ");
				
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

	EM_DEBUG_FUNC_END();
}

void stb_account_get_refer(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int local_result = 0;
	int account_id = 0;
	emf_account_t* account = NULL;
	char* local_stream = NULL;
	int size = 0;

	EM_DEBUG_LOG("account_id");
	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));

	account = emf_get_account_reference(account_id);
	if(account != NULL) {
		EM_DEBUG_LOG("emf_get_account_reference success");
		local_result = 1;
		
		/* result */
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
		
		/* Address */
		local_stream = em_convert_account_to_byte_stream(account, &size);

		EM_NULL_CHECK_FOR_VOID(local_stream);

		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");

		if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");
		 	EM_SAFE_FREE(local_stream);
			return;
		}

	}
	else {
		EM_DEBUG_LOG("emf_get_account_reference failed");
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
		 	EM_SAFE_FREE(local_stream);
			return;
		}
	}

 	EM_SAFE_FREE(local_stream);
	EM_DEBUG_FUNC_END();
}

void stb_account_get_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int local_result = 0;
	int counter = 0;
	char* local_stream = NULL;
	emf_account_t* account_list;
	int count;
	int size = 0;
	int err = EMF_ERROR_NONE;

	EM_DEBUG_LOG("begin");
	
	if(emf_account_get_list(&account_list, &count, &err)) {
		EM_DEBUG_LOG("emf_account_get_list success");
		local_result = 1;
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");

		EM_DEBUG_LOG("Count [%d]", count);
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &count, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter count failed ");

		for(counter=0; counter<count; counter++) {
			EM_DEBUG_LOG("Name - %s", account_list[counter].account_name);

			local_stream = em_convert_account_to_byte_stream(account_list+counter, &size);

			EM_NULL_CHECK_FOR_VOID(local_stream);

			if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
				EM_DEBUG_EXCEPTION("Add  Param mailbox failed  ");

			size = 0;
		 	EM_SAFE_FREE(local_stream);
		}

		emf_account_free(&account_list, count, NULL);  
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}

	else {
		EM_DEBUG_LOG("emf_account_get_list failed");
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed : err");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}

 	EM_SAFE_FREE(local_stream);
	EM_DEBUG_FUNC_END();
}
	
void stb_modify_mail_flag(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	int mail_id = 0;
	int i_flag = 0;
	int onserver = 0;
	int sticky_flag = 0;
	emf_mail_flag_t new_flag = { 0 };

	EM_DEBUG_LOG("mail_id");
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("mail_id[%d]", mail_id);

	EM_DEBUG_LOG("i_flag");
	memcpy(&i_flag, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
	EM_DEBUG_LOG("i_flag[%d]", i_flag);

	EM_DEBUG_LOG("Sticky flag");
	memcpy(&sticky_flag, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), sizeof(int));
	EM_DEBUG_LOG(">>> STICKY flag Value [ %d] ", sticky_flag);

	EM_DEBUG_LOG("onserver");
	memcpy(&onserver, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 3), sizeof(int));
	EM_DEBUG_LOG("onserver[%d]", onserver);

	EM_DEBUG_LOG("Flag Change 1>>> ");
	if(!em_convert_mail_int_to_flag(i_flag, &new_flag, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_int_to_flag failed");
		return;
	}
	if(emf_mail_modify_flag(mail_id, new_flag, onserver, sticky_flag, &err))		
		EM_DEBUG_LOG("emf_mail_modify_flag - success");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_modify_mail_extra_flag(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	int mail_id = 0;
	int buffer_size = 0;
	emf_extra_flag_t new_flag= { 0 };
	char* local_stream = NULL;

	EM_DEBUG_LOG("mail_id");
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));

	EM_DEBUG_LOG("extra_flag");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 1);

	if(buffer_size > 0) {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), buffer_size);
			em_convert_byte_stream_to_extra_flags(local_stream, &new_flag);
			EM_SAFE_FREE(local_stream);
		}

		if(emf_mail_modify_extra_flag(mail_id, new_flag, &err))		
			EM_DEBUG_LOG("emf_mail_modify_extra_flag - success");
		
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
		
			if (!ipcEmailStub_ExecuteAPI(a_hAPI))
				EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
		
	}
	EM_DEBUG_FUNC_END();
}


void stb_mail_save_to_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int local_result = 0;
	char* local_head_stream = NULL;
	char* local_body_stream = NULL;
	char* local_info_stream = NULL;
	char* local_stream = NULL;
	int from_composer = 0;
	emf_mail_t *mail = NULL;
	emf_mailbox_t *mailbox = NULL;
	int err = EMF_ERROR_NONE;
	emf_meeting_request_t *meeting_req = NULL;

	EM_DEBUG_LOG("head");

	mail = (emf_mail_t*)em_core_malloc(sizeof(emf_mail_t));

	if(mail == NULL) {
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_head_stream = (char*)em_core_malloc(buffer_size+1);
		if(local_head_stream) {
			memcpy(local_head_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			mail->head = (emf_mail_head_t*)em_core_malloc(sizeof(emf_mail_head_t));
			
			if(NULL == mail->head) {
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_convert_byte_stream_to_mail_head(local_head_stream, mail->head);
		}
		else {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("Body - mod");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 1);

	if(buffer_size > 0)	 {
		local_body_stream = (char*)em_core_malloc(buffer_size+1);
		if(local_body_stream) {
			memcpy(local_body_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), buffer_size);
			mail->body = (emf_mail_body_t*)em_core_malloc(sizeof(emf_mail_body_t));

			if(NULL == mail->body)		 {
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_convert_byte_stream_to_mail_body(local_body_stream, mail->body);
		}
		else {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("Info");

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 2);

	if(buffer_size > 0)	 {
		local_info_stream = (char*)em_core_malloc(buffer_size+1);
		if(local_info_stream) {
			memcpy(local_info_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), buffer_size);
			mail->info = (emf_mail_info_t*)em_core_malloc(sizeof(emf_mail_info_t));

			if(NULL == mail->info) {
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_convert_byte_stream_to_mail_info(local_info_stream, mail->info);
		}
		else {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 3);

	if(buffer_size > 0) {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 3), buffer_size);
			mailbox = (emf_mailbox_t*)em_core_malloc(sizeof(emf_mailbox_t));
			if(!mailbox) {
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_convert_byte_stream_to_mailbox(local_stream, mailbox);
		}
		else {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("from_composer");
	memcpy(&from_composer, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 4), sizeof(int));

	EM_DEBUG_LOG("calling emf_mail_save_to_mailbox");

	if ( mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_REQUEST 
		|| mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_RESPONSE 
		|| mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 5);

		if(buffer_size > 0) {
			local_stream = (char*)em_core_malloc(buffer_size+1);
			if(local_stream) {
				memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 5), buffer_size);
				meeting_req = (emf_meeting_request_t*)em_core_malloc(sizeof(emf_meeting_request_t));
				if(meeting_req) {
					err = EMF_ERROR_OUT_OF_MEMORY;
					goto FINISH_OFF;
				}
				em_convert_byte_stream_to_meeting_req(local_stream, meeting_req);
			}
			else {
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
		}
	}
	
	EM_DEBUG_LOG("calling emf_mail_save_to_mailbox with meeting request");
	if(emf_mail_save_to_mailbox(mail, mailbox, meeting_req, from_composer, &err)) {	/*  successfully saving */
		local_result = 1;
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter result failed ");
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &(mail->info->uid), sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter mail_idFail ");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}

FINISH_OFF:
	if ( local_result == 0 ) {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}

	EM_SAFE_FREE(local_head_stream);
	EM_SAFE_FREE(local_body_stream);
	EM_SAFE_FREE(local_info_stream); 
	EM_SAFE_FREE(local_stream);
	
	if(mail)
		em_core_mail_free(&mail, 1, &err);

	if(mailbox)
		em_core_mailbox_free(&mailbox, 1, &err);

	em_core_flush_memory();
	EM_DEBUG_FUNC_END();
}

void stb_mail_get_mailbox_count(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int local_result = 0;
	char* local_stream = NULL;
	emf_mailbox_t mailbox;
	int total_count = 0;
	int unseen= 0;

	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	{
		local_stream = (char*)em_core_malloc(buffer_size+1);
		if(!local_stream) {
			EM_DEBUG_EXCEPTION("EMF_ERROR_OUT_OF_MEMORY");
			goto FINISH_OFF;
		}
		memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
		em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
		EM_SAFE_FREE(local_stream);
	}

	/*get the Mailbox Count */
	if (!emf_mailbox_get_mail_count(&mailbox, &total_count, &unseen, NULL)) {
		EM_DEBUG_EXCEPTION("emf_mailbox_get_mail_count - failed");
		goto FINISH_OFF;
	}
	else {
		EM_DEBUG_LOG("emf_mailbox_get_mail_count - success");
		local_result = 1;
	}

FINISH_OFF:

	if(local_result) {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");

		/* Totol count of mails */
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &total_count, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed  ");

		/* Unread Mail */
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &unseen, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed  ");

		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}
	else {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");
	}

	EM_DEBUG_FUNC_END();
}


/* sowmya.kr, 10-May-2010, changes for API improvement */
void stb_mailbox_sync_header(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	emf_mailbox_t mailbox, *pointer_mailbox = NULL;
	int handle = -1;
	
	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0) {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(!local_stream) {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
		em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
		EM_SAFE_FREE(local_stream);
		pointer_mailbox = &mailbox;
	}

  if(emf_mailbox_sync_header(pointer_mailbox, (unsigned*)&handle, &err)) {
		EM_DEBUG_LOG("emf_mailbox_sync_header success ");
  }

FINISH_OFF:
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_mailbox_download_body(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	emf_mailbox_t mailbox;
	int mail_id = 0;
	int has_attachment = 0;
	unsigned int handle = 0;
	
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	{
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	/* Mail Id */
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));

	/* Has Attachment */
	memcpy(&has_attachment, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), sizeof(int));

	/*Download Body */
	if (!emf_mail_download_body(&mailbox, mail_id, 1, has_attachment, &handle, &err)) {
		EM_DEBUG_EXCEPTION("emf_mail_download_body - failed");
		goto FINISH_OFF;
	}

	err = EMF_ERROR_NONE;

FINISH_OFF:

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
	EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

	EM_DEBUG_FUNC_END();
}



void stb_mail_get_mail(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	emf_mailbox_t mailbox;
	int mail_id = 0;
	emf_mail_t* mail = NULL;
	int size =0;
	char* local_head_stream = NULL;
	char* local_body_stream = NULL;
	char* local_info_stream = NULL;
	int err=EMF_ERROR_NONE;
	
	EM_DEBUG_LOG("mail_id");
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	
	if(emf_mail_get_mail(&mailbox, mail_id, &mail, &err))
		EM_DEBUG_LOG("success");

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION(" stb_mail_get_mail ipcEmailAPI_AddParameter local_result failed ");
	if(EMF_ERROR_NONE == err) {		
		/* Head */
		EM_DEBUG_LOG("convert mail head");
		local_head_stream = em_convert_mail_head_to_byte_stream(mail->head, &size);

		EM_NULL_CHECK_FOR_VOID(local_head_stream);

		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_head_stream, size))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed  ");

		/* Body */
		EM_DEBUG_LOG("convert mail Body");
		local_body_stream = em_convert_mail_body_to_byte_stream(mail->body, &size);

		if ( !local_body_stream ) {
			EM_SAFE_FREE(local_head_stream);
			return;
		}
			
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_body_stream, size))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed  ");
	
		/* Info */
		EM_DEBUG_LOG("convert mail Info");
		local_info_stream = em_convert_mail_info_to_byte_stream(mail->info, &size);

		if ( !local_info_stream ) {
			EM_SAFE_FREE(local_head_stream);
			EM_SAFE_FREE(local_body_stream);
			return;
		}

		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_info_stream, size))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed  ");
	}
	
	EM_DEBUG_LOG("Before Execute API");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
			EM_SAFE_FREE(local_head_stream);
			EM_SAFE_FREE(local_body_stream);
			EM_SAFE_FREE(local_info_stream);
			return;
		}

	EM_SAFE_FREE(local_head_stream);
	EM_SAFE_FREE(local_body_stream);
	EM_SAFE_FREE(local_info_stream);
	
	EM_DEBUG_FUNC_END();
}

void stb_mailbox_create(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int 		buffer_size 			= 0;
	int 		err = EMF_ERROR_NONE;
	char 		*local_stream  = NULL;
	int 		on_server 		= 0;
	emf_mailbox_t mailbox;
	int handle = 0; /* Added for cancelling mailbox creating  */

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);

	if(buffer_size > 0) {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			memcpy(&on_server, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
					
			if (mailbox.name)
				EM_DEBUG_LOG("Mailbox name - %s", mailbox.name);
			
			if(emf_mailbox_create(&mailbox, on_server, (unsigned*)&handle, &err))
				err = EMF_ERROR_NONE;
			
			if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed 1");
			if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed 2");
			
			if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
				EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
				return;
			}
		}
	}	
	EM_DEBUG_FUNC_END();
}


void stb_mailbox_delete(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int 		buffer_size 			= 0;
	int		 err = EMF_ERROR_NONE;
	char 	*local_stream  = NULL;
	int 		on_server		= 0;
	emf_mailbox_t mailbox;
	int handle = 0; /* Added for cancelling mailbox deleting */

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);
	
	EM_DEBUG_LOG("size [%d]", buffer_size);

	if(buffer_size > 0) {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			memcpy(&on_server, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
			
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);

			if (mailbox.name)
				EM_DEBUG_LOG("Mailbox name - %s", mailbox.name);
			if(emf_mailbox_delete(&mailbox, on_server, (unsigned*)&handle, &err))
				err = EMF_ERROR_NONE;
			
			if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
			if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
			
			if (!ipcEmailStub_ExecuteAPI(a_hAPI))
				EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");
			
		}
	}	
	EM_DEBUG_FUNC_END();
}

void stb_mailbox_update(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int 		buffer_size 			= 0;
	int		 err = EMF_ERROR_NONE;
	char 	*old_mailbox_stream  = NULL;
	char 	*new_mailbox_stream  = NULL;
	int 		on_server		= 0;
	emf_mailbox_t *old_mailbox = NULL;
	emf_mailbox_t *new_mailbox = NULL;
	int handle = 0; /* Added for cancelling mailbox updating */

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	if(buffer_size > 0)	  {
		old_mailbox_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(old_mailbox_stream);
		if(old_mailbox_stream) {
			memcpy(old_mailbox_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			old_mailbox = (emf_mailbox_t*)em_core_malloc(sizeof(emf_mailbox_t));
			if ( old_mailbox == NULL ) {
				EM_DEBUG_EXCEPTION("em_core_malloc failed.");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			em_convert_byte_stream_to_mailbox(old_mailbox_stream, old_mailbox);
			EM_SAFE_FREE(old_mailbox_stream);
		}
	}
	else {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	if(buffer_size > 0)	  {
		new_mailbox_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(new_mailbox_stream);
		if(new_mailbox_stream) {
			memcpy(new_mailbox_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), buffer_size);
			new_mailbox = (emf_mailbox_t*)em_core_malloc(sizeof(emf_mailbox_t));
			if ( new_mailbox == NULL ) {
				EM_DEBUG_EXCEPTION("em_core_malloc failed.");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_convert_byte_stream_to_mailbox(new_mailbox_stream, new_mailbox);
			EM_SAFE_FREE(new_mailbox_stream);
		}
	}
	else {
		EM_DEBUG_EXCEPTION("INVALID PARAM : old mailbox");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	memcpy(&on_server, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), sizeof(int));
	
	if(emf_mailbox_update(old_mailbox, new_mailbox, on_server, (unsigned*)&handle, &err))
		err = EMF_ERROR_NONE;

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");
			

FINISH_OFF:
	EM_SAFE_FREE(old_mailbox_stream);
	EM_SAFE_FREE(new_mailbox_stream);

	emf_mailbox_free(&old_mailbox, 1, NULL);
	emf_mailbox_free(&new_mailbox, 1, NULL);
	EM_DEBUG_FUNC_END();
}


void stb_mailbox_set_mail_slot_size(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size  = 0;
	int	err    = EMF_ERROR_NONE;
	int handle = 0; 
	int account_id = 0, mail_slot_size = 0;
	char *mailbox_name = NULL;


	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("account_id[%d]", account_id);

	memcpy(&mail_slot_size, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
	EM_DEBUG_LOG("mail_slot_size[%d]", mail_slot_size);

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 2);
	EM_DEBUG_LOG("mailbox name string size[%d]", buffer_size);
	if(buffer_size > 0)	  {
		mailbox_name = (char*)em_core_malloc(buffer_size + 1);
		memcpy(mailbox_name, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), buffer_size);
		EM_DEBUG_LOG("mailbox_name [%s]", mailbox_name);
	}
	
	if(emf_mailbox_set_mail_slot_size(account_id, mailbox_name, mail_slot_size, (unsigned*)&handle, &err))
		err = EMF_ERROR_NONE;
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed 1");

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed 2");
	
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

	EM_SAFE_FREE(mailbox_name);
	EM_DEBUG_FUNC_END();
}

void stb_mail_send(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	char* local_option_stream = NULL;
	char* local_stream = NULL;
	emf_mailbox_t* mailbox = NULL;
	emf_option_t sending_option;
	int mail_id;
	int handle;
	int err = EMF_ERROR_NONE;
	char* mailbox_name = NULL;

	/* Mailbox */
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0) {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			mailbox = (emf_mailbox_t*)em_core_malloc(sizeof(emf_mailbox_t));
			em_convert_byte_stream_to_mailbox(local_stream, mailbox);

			if (!em_storage_get_mailboxname_by_mailbox_type(mailbox->account_id,EMF_MAILBOX_TYPE_DRAFT,&mailbox_name, false, &err))  {
				EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d]", err);
				err = em_storage_get_emf_error_from_em_storage_error(err);
				goto FINISH_OFF;
			}
			mailbox->name = mailbox_name;
		}
	}

	/* Mail_id */
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
	EM_DEBUG_LOG("mail_id [%d]", mail_id);

	/* Sending Option */
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 2);

	if(buffer_size > 0) {
		local_option_stream = (char*)em_core_malloc(buffer_size+1);
		
		if(NULL == local_option_stream) {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		if(local_option_stream) {
			memcpy(local_option_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), buffer_size);
			em_convert_byte_stream_to_option(local_option_stream, &sending_option);
		}
	}

	if(emf_mail_send(mailbox, mail_id, &sending_option, (unsigned*)&handle, &err)) {
		err = EMF_ERROR_NONE;
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter result failed ");
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter result failed ");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}
	else {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}

FINISH_OFF:	
	EM_SAFE_FREE(local_stream);
	EM_SAFE_FREE(local_option_stream);
	EM_SAFE_FREE(mailbox);
	EM_SAFE_FREE(mailbox_name);
	
	EM_DEBUG_FUNC_END();
}

void stb_mailbox_get_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	int counter = 0;
	char* local_stream = NULL;
	int account_id;
	emf_mailbox_t* mailbox_list = NULL;
	int count = 0;
	int size = 0;

	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));

	if(emf_mailbox_get_list(account_id, &mailbox_list, &count, &err))
		EM_DEBUG_LOG("emf_mailbox_get_list - success");
		
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");

	if(EMF_ERROR_NONE == err) {
		EM_DEBUG_LOG("Count [%d]", count);
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &count, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");

		for(counter=0; counter<count; counter++) {
			EM_DEBUG_LOG("Name - %s", mailbox_list[counter].name);

			local_stream = em_convert_mailbox_to_byte_stream(mailbox_list+counter, &size);

			EM_NULL_CHECK_FOR_VOID(local_stream);

			if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
				EM_DEBUG_EXCEPTION("Add  Param mailbox failed  ");

			EM_SAFE_FREE(local_stream);
			size = 0;
		}
	}

	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}


void stb_update_mail_old(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int mail_id;
	int buffer_size = 0;
	int local_result = 0;
	char* stream_for_head = NULL;	
	char* stream_for_body = NULL;	
	char* local_info_stream = NULL;
	emf_mail_t *mail = NULL;
	int err = EMF_ERROR_NONE;
	char* stream_for_meeting_request = NULL;
	emf_meeting_request_t *meeting_req = NULL;

	mail = (emf_mail_t*)em_core_malloc(sizeof(emf_mail_t));

	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));;
	
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 1);
		
	if(buffer_size > 0)	 {
		stream_for_head = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(stream_for_head);
		if(stream_for_head) {
			memcpy(stream_for_head, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), buffer_size);
			mail->head = (emf_mail_head_t*)em_core_malloc(sizeof(emf_mail_head_t));
			if(NULL == mail->head)			 {
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_convert_byte_stream_to_mail_head(stream_for_head, mail->head);
		}
		else {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 2);
	if ( buffer_size == sizeof(int) ) {	
		EM_DEBUG_LOG(">>>>> Mail body is Null");
		mail->body = NULL;
	}
	else if(buffer_size > 0) {	
		stream_for_body = (char*)em_core_malloc(buffer_size+1);
		if(stream_for_body) {
			memcpy(stream_for_body, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), buffer_size);
			mail->body = (emf_mail_body_t*)em_core_malloc(sizeof(emf_mail_body_t));
			if(NULL == mail->body) {
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_convert_byte_stream_to_mail_body(stream_for_body,  mail->body);
		}
		else {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}	

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 3);
	if(buffer_size > 0)	 {
		local_info_stream = (char*)em_core_malloc(buffer_size+1);
		if(local_info_stream) {
			memcpy(local_info_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 3), buffer_size);
			mail->info = (emf_mail_info_t*)em_core_malloc(sizeof(emf_mail_info_t));
			if(NULL == mail->info)		 {
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_convert_byte_stream_to_mail_info(local_info_stream,  mail->info);
		}
		else {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}	

	EM_DEBUG_LOG("meeting request[%d]", mail->info->is_meeting_request);

	if ( mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_REQUEST
		|| mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_RESPONSE 
		|| mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 4);

		if(buffer_size > 0) {
			stream_for_meeting_request = (char*)em_core_malloc(buffer_size+1);
			if(stream_for_meeting_request) {
				memcpy(stream_for_meeting_request, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 4), buffer_size);
				meeting_req = (emf_meeting_request_t*)em_core_malloc(sizeof(emf_meeting_request_t));
				if(meeting_req) {
					em_convert_byte_stream_to_meeting_req(stream_for_meeting_request, meeting_req);
				}
			}
			else {
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
		}
	}

	EM_DEBUG_LOG("calling emf_mail_update with meeting request");
	if(emf_mail_update(mail_id, mail, meeting_req, 0, &err)) {
		local_result = 1;

		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
				
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}

FINISH_OFF:
	if ( local_result == 0 ) {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
				
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}

	EM_SAFE_FREE(stream_for_head);
	EM_SAFE_FREE(stream_for_body);
	EM_SAFE_FREE(local_info_stream);
	
	/* Caution!! Don't free mail here. mail will be freed after using in event loop. 
	if (mail)
		em_core_mail_free(&mail, 1, &err_code);
	EM_SAFE_FREE(meeting_req);
	*/

	em_core_flush_memory();
	EM_DEBUG_FUNC_END();
}

void stb_mail_delete_all(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	char* local_stream = NULL;
	emf_mailbox_t *mailbox = NULL;
	int with_server = 0;
	int err = EMF_ERROR_NONE;

	/* Mailbox */
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	{
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			mailbox = (emf_mailbox_t*)em_core_malloc(sizeof(emf_mailbox_t));
			em_convert_byte_stream_to_mailbox(local_stream, mailbox);
		}
	}

	/* with_server */
	memcpy(&with_server, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));

	emf_mail_delete_all(mailbox, with_server, NULL, &err);
	
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

	EM_SAFE_FREE(local_stream);
	EM_SAFE_FREE(mailbox);

	EM_DEBUG_FUNC_END();
}

void stb_mail_delete(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	emf_mailbox_t *mailbox = NULL;
	int with_server = 0;
	int counter = 0;
	int num;

	/* Mailbox */
	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			/* Memory is not yet allocated for mailbox */
			mailbox = (emf_mailbox_t*)em_core_malloc(sizeof(emf_mailbox_t));
			em_convert_byte_stream_to_mailbox(local_stream, mailbox);
		}
		EM_DEBUG_LOG("account_id [%d]", mailbox->account_id);
		EM_DEBUG_LOG("mailbox name - %s", mailbox->name);

		/* Number of mail_ids */
		memcpy(&num, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
		EM_DEBUG_LOG("number of mails [%d]", num);

		int mail_ids[num];	
		memcpy(mail_ids, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), num * sizeof(int));
		for(counter = 0; counter < num; counter++)
			EM_DEBUG_LOG("mail_ids [%d]", mail_ids[counter]);
		
		/* with_server */
		memcpy(&with_server, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 3), sizeof(int));
		EM_DEBUG_LOG("with_Server [%d]", with_server);

		emf_mail_delete(mailbox->account_id, mailbox, mail_ids, num, with_server, NULL, &err);
	}
	else
		err = EMF_ERROR_IPC_CRASH;
	
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");

	EM_SAFE_FREE(local_stream);
	EM_SAFE_FREE(mailbox);	

	EM_DEBUG_FUNC_END();	
}
void stb_clear_mail_data (HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	
	if(emf_clear_mail_data(&err)) {
		EM_DEBUG_LOG(">>> stb_clear_mail_data Success [ %d]  >> ", err);
	}
	
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed : err");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_rule_add(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_rule_stream = NULL;
	emf_rule_t rule = { 0 };

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	{
		local_rule_stream = (char*)em_core_malloc(buffer_size + 1);
		EM_NULL_CHECK_FOR_VOID(local_rule_stream);
		if(local_rule_stream) {
			memcpy(local_rule_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			em_convert_byte_stream_to_rule(local_rule_stream, &rule);
			EM_DEBUG_LOG("account ID  [%d]", rule.account_id);

			if(emf_filter_add(&rule, &err))
				err = EMF_ERROR_NONE;
			
			if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
			if (!ipcEmailStub_ExecuteAPI(a_hAPI))
				EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	
		}
	}	

	EM_SAFE_FREE(local_rule_stream);

	EM_DEBUG_FUNC_END();
}

void stb_rule_get(HIPC_API a_hAPI)
{
	int err = EMF_ERROR_NONE;
	int filter_id = 0;
	emf_rule_t* rule = NULL;
	int size =0;
	char* local_rule_stream = NULL;

	EM_DEBUG_LOG(" filter_id");
	memcpy(&filter_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));

	if(emf_filter_get(filter_id, &rule, &err)) {
		err = EMF_ERROR_NONE;
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
		
		/* Rule */
		local_rule_stream = em_convert_rule_to_byte_stream(rule, &size);

		EM_NULL_CHECK_FOR_VOID(local_rule_stream);

		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_rule_stream, size))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed  ");

		if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
			EM_SAFE_FREE( local_rule_stream );
			return;
		}
	}
	else {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");

		if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
			EM_SAFE_FREE( local_rule_stream );			
			return;
		}
	}
	EM_SAFE_FREE( local_rule_stream );	
	EM_DEBUG_FUNC_END();
}

void stb_rule_get_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	/* int buffer_size = 0; */
	int err = EMF_ERROR_NONE;
	int counter = 0;
	char* local_stream = NULL;
	int count = 0;
	int size = 0;

	emf_rule_t* filtering_list = NULL;

	if(emf_filter_get_list(&filtering_list, &count, &err)) {
		EM_DEBUG_LOG("emf_filter_get_list - success");
		err = EMF_ERROR_NONE;
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");

		EM_DEBUG_LOG("Count [%d]", count);
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &count, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");

		for(counter=0; counter<count; counter++) {
			EM_DEBUG_LOG("Value - %s", filtering_list[counter].value);

			local_stream = em_convert_rule_to_byte_stream(filtering_list+counter, &size);

			EM_NULL_CHECK_FOR_VOID(local_stream);

			if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed  ");

			EM_SAFE_FREE(local_stream);
			size = 0;
		}

		if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
			return;
		}
	}

	else {
		EM_DEBUG_EXCEPTION("emf_filter_get_list - failed");
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
		
		if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
			return;
		}
	}
	EM_DEBUG_FUNC_END();
}

void stb_rule_find(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_rule_stream = NULL;
	emf_rule_t rule = { 0 };


	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_rule_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_rule_stream);
		if(local_rule_stream) {
			memcpy(local_rule_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			em_convert_byte_stream_to_rule(local_rule_stream, &rule);
			EM_SAFE_FREE(local_rule_stream);
			EM_DEBUG_LOG("account ID  [%d]", rule.account_id);
		
			if(emf_filter_find(&rule, &err))
				err = EMF_ERROR_NONE;
			
			if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
			if (!ipcEmailStub_ExecuteAPI(a_hAPI))
				EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	
		}
	}	
	EM_DEBUG_FUNC_END();
}

void stb_rule_change(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int filter_id = 0;
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* rule_stream = NULL;
	emf_rule_t  *rule = NULL;

	/* filter_id */
	memcpy(&filter_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));;
	
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	
	if(buffer_size > 0)  {
		rule_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(rule_stream);
		if(rule_stream) {
			memcpy(rule_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), buffer_size);
			rule = (emf_rule_t*)em_core_malloc(sizeof(emf_rule_t));

			if(NULL == rule)			 {
				EM_SAFE_FREE(rule_stream);
				return;
			}
			
			em_convert_byte_stream_to_rule(rule_stream, rule);
			EM_SAFE_FREE(rule_stream);
		}
	}

	if(emf_filter_change(filter_id, rule, &err))
		err = EMF_ERROR_NONE;

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
			
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");

	EM_SAFE_FREE(rule);	
	EM_DEBUG_FUNC_END();	
}

void stb_mail_move_all_mails(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* src_mailbox_stream = NULL;	
	char* dst_mailbox_stream = NULL;
	emf_mailbox_t dst_mailbox;
	emf_mailbox_t src_mailbox;	

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("buffer_size [%d]", buffer_size);
	
	if(buffer_size > 0)	 {
		src_mailbox_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(src_mailbox_stream);
		if(src_mailbox_stream) {
			memcpy(src_mailbox_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			/* Convert Byte stream to mailbox structure */
			EM_DEBUG_LOG("Before em_convert_byte_stream_to_mailbox");
			em_convert_byte_stream_to_mailbox(src_mailbox_stream, &src_mailbox);
			EM_SAFE_FREE(src_mailbox_stream);
			if (src_mailbox.name)
				EM_DEBUG_LOG("Mailbox name - %s", src_mailbox.name);
			else
				EM_DEBUG_EXCEPTION(">>>> Mailbox Information is NULL >> ");
			
		}
	}

	buffer_size =0;
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG(" size [%d]", buffer_size);
	
	if(buffer_size > 0)	 {
		dst_mailbox_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(dst_mailbox_stream);
		if(dst_mailbox_stream) {
			memcpy(dst_mailbox_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), buffer_size);
			em_convert_byte_stream_to_mailbox(dst_mailbox_stream, &dst_mailbox);
			EM_SAFE_FREE(dst_mailbox_stream);
			if (dst_mailbox.name)
				EM_DEBUG_LOG("Mailbox name - %s", dst_mailbox.name);
			else
				EM_DEBUG_EXCEPTION(">>>> Mailbox Information is NULL >> ");
		}
	}
	
	if(emf_mail_move_all_mails(&src_mailbox, &dst_mailbox, &err))
		EM_DEBUG_LOG("emf_mail_move_all_mails:Success");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int))) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
		return;
	}
	
	if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");
		return;
	}
	EM_DEBUG_FUNC_END();
}

void stb_set_flags_field(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	emf_flags_field_type field_type = 0;
	void *temp_buffer = NULL;
	int account_id;
	int value = 0;
	int onserver = 0;
	int num = 0;
	int counter = 0;
	int *mail_ids = NULL;

	/* account_id */
	temp_buffer = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0);
	if(!temp_buffer) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed ");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	memcpy(&account_id, temp_buffer, sizeof(int));
	EM_DEBUG_LOG("account_id [%d]", account_id);

	/* Number of mail_ids */
	temp_buffer = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1);
	if(!temp_buffer) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed ");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	memcpy(&num, temp_buffer, sizeof(int));
	EM_DEBUG_LOG("number of mails [%d]", num);
	
	/* mail_id */
	mail_ids = em_core_malloc(sizeof(int) * num);
	
	if(!mail_ids) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed ");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	temp_buffer = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2);
	if(!temp_buffer) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed ");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	memcpy(mail_ids, temp_buffer, num * sizeof(int));
	
	for(counter=0; counter < num; counter++) {
		EM_DEBUG_LOG("mailids [%d]", mail_ids[counter]);
	}

	/* field type */
	temp_buffer = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 3);
	if(!temp_buffer) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed ");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	memcpy(&field_type, temp_buffer, sizeof(int));
	EM_DEBUG_LOG("field_type [%d]", field_type);

	/* value */
	temp_buffer = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 4);
	if(!temp_buffer) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed ");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	memcpy(&value, temp_buffer, sizeof(int));
	EM_DEBUG_LOG("value [%d]", value);
	
	/*  on server */
	temp_buffer = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 5);
	if(!temp_buffer) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed ");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	memcpy(&onserver, temp_buffer, sizeof(int));
	EM_DEBUG_LOG("onserver [%d]", value);

	if(emf_mail_set_flags_field(account_id, mail_ids, num, field_type, value, onserver, &err))	
		EM_DEBUG_LOG("emf_mail_set_flags_field - success");

FINISH_OFF:

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
		
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_add_mail(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int  buffer_size = 0;
	int  local_result = 0;
	int  result_mail_data_count = 0;
	int  result_attachment_data_count = 0;
	int  parameter_index = 0;
	int  sync_server = 0;
	int  err = EMF_ERROR_NONE;
	char *mail_data_stream = NULL;
	char *attachment_data_list_stream = NULL;
	char *meeting_request_stream = NULL;
	char *parameter_data = NULL;
	emf_mail_data_t *result_mail_data = NULL;
	emf_attachment_data_t *result_attachment_data = NULL;
	emf_meeting_request_t *result_meeting_request = NULL;
	
	EM_DEBUG_LOG("emf_mail_data_t");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, parameter_index);

	if(buffer_size > 0)	 {
		mail_data_stream = (char*)em_core_malloc(buffer_size + 1);
		if(!mail_data_stream) {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		parameter_data = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, parameter_index++);

		if(!parameter_data) {
			EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}

		memcpy(mail_data_stream, parameter_data, buffer_size);

		em_convert_byte_stream_to_mail_data(mail_data_stream, &result_mail_data, &result_mail_data_count);
		if(!result_mail_data) {
			EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_mail_data failed");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}
	}

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, parameter_index);
	EM_DEBUG_LOG("emf_attachment_data_t buffer_size[%d]", buffer_size);

	if(buffer_size > 0)	 {
		attachment_data_list_stream = (char*)em_core_malloc(buffer_size + 1);
		if(!attachment_data_list_stream) {
			EM_DEBUG_EXCEPTION("em_core_malloc failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		parameter_data = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, parameter_index++);

		if(!parameter_data) {
			EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}

		memcpy(attachment_data_list_stream, parameter_data, buffer_size);

		em_convert_byte_stream_to_attachment_data(attachment_data_list_stream, &result_attachment_data, &result_attachment_data_count);

		EM_DEBUG_LOG("result_attachment_data_count[%d]", result_attachment_data_count);

		if(result_attachment_data_count && !result_attachment_data) {
			EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_attachment_data failed");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("emf_meeting_request_t");

	if ( result_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_REQUEST 
		|| result_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_RESPONSE 
		|| result_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, parameter_index);

		if(buffer_size > 0) {
			meeting_request_stream = (char*)em_core_malloc(buffer_size + 1);
			if(!meeting_request_stream) {
				EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_mail_data failed");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			parameter_data = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, parameter_index++);

			if(!parameter_data) {
				EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed");
				err = EMF_ERROR_ON_PARSING;
				goto FINISH_OFF;
			}

			memcpy(meeting_request_stream, parameter_data, buffer_size);
			result_meeting_request = (emf_meeting_request_t*)em_core_malloc(sizeof(emf_meeting_request_t));
			if(result_meeting_request)
				em_convert_byte_stream_to_meeting_req(meeting_request_stream, result_meeting_request);
		}
	}

	EM_DEBUG_LOG("sync_server");
	memcpy(&sync_server, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, parameter_index++), sizeof(int));

	if( (err = emf_add_mail(result_mail_data, result_attachment_data, result_attachment_data_count, result_meeting_request, sync_server)) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emf_add_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	local_result = 1;
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &(result_mail_data->mail_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &(result_mail_data->thread_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");

FINISH_OFF:
	if ( local_result == 0 ) {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");
	}

	EM_SAFE_FREE(mail_data_stream);
	EM_SAFE_FREE(attachment_data_list_stream);
	EM_SAFE_FREE(meeting_request_stream); 

	if(result_mail_data)
		em_core_free_mail_data(&result_mail_data, 1, NULL);

	if(result_attachment_data)
		em_core_free_attachment_data(&result_attachment_data, result_attachment_data_count, NULL);

	if(result_meeting_request)
		em_storage_free_meeting_request(&result_meeting_request, 1, NULL);

	em_core_flush_memory();
	EM_DEBUG_FUNC_END();
}


void stb_update_mail(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int  buffer_size = 0;
	int  local_result = 0;
	int  result_mail_data_count = 0;
	int  result_attachment_data_count = 0;
	int  parameter_index = 0;
	int  sync_server = 0;
	int  err = EMF_ERROR_NONE;
	char *mail_data_stream = NULL;
	char *attachment_data_list_stream = NULL;
	char *meeting_request_stream = NULL;
	char *parameter_data = NULL;
	emf_mail_data_t *result_mail_data = NULL;
	emf_attachment_data_t *result_attachment_data = NULL;
	emf_meeting_request_t *result_meeting_request = NULL;

	EM_DEBUG_LOG("emf_mail_data_t");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, parameter_index);

	if(buffer_size > 0)	 {
		mail_data_stream = (char*)em_core_malloc(buffer_size + 1);
		if(!mail_data_stream) {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		parameter_data = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, parameter_index++);

		if(!parameter_data) {
			EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}

		memcpy(mail_data_stream, parameter_data, buffer_size);

		em_convert_byte_stream_to_mail_data(mail_data_stream, &result_mail_data, &result_mail_data_count);
		if(!result_mail_data) {
			EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_mail_data failed");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}
	}

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, parameter_index);
	EM_DEBUG_LOG("emf_attachment_data_t buffer_size[%d]", buffer_size);

	if(buffer_size > 0)	 {
		attachment_data_list_stream = (char*)em_core_malloc(buffer_size + 1);
		if(!attachment_data_list_stream) {
			EM_DEBUG_EXCEPTION("em_core_malloc failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		parameter_data = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, parameter_index++);

		if(!parameter_data) {
			EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}

		memcpy(attachment_data_list_stream, parameter_data, buffer_size);

		em_convert_byte_stream_to_attachment_data(attachment_data_list_stream, &result_attachment_data, &result_attachment_data_count);

		EM_DEBUG_LOG("result_attachment_data_count[%d]", result_attachment_data_count);

		if(result_attachment_data_count && !result_attachment_data) {
			EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_attachment_data failed");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG("emf_meeting_request_t");

	if ( result_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_REQUEST
		|| result_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_RESPONSE
		|| result_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, parameter_index);

		if(buffer_size > 0) {
			meeting_request_stream = (char*)em_core_malloc(buffer_size + 1);
			if(!meeting_request_stream) {
				EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_mail_data failed");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			parameter_data = ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, parameter_index++);

			if(!parameter_data) {
				EM_DEBUG_EXCEPTION("ipcEmailAPI_GetParameter failed");
				err = EMF_ERROR_ON_PARSING;
				goto FINISH_OFF;
			}

			memcpy(meeting_request_stream, parameter_data, buffer_size);
			result_meeting_request = (emf_meeting_request_t*)em_core_malloc(sizeof(emf_meeting_request_t));
			if(result_meeting_request)
				em_convert_byte_stream_to_meeting_req(meeting_request_stream, result_meeting_request);
		}
	}

	EM_DEBUG_LOG("sync_server");
	memcpy(&sync_server, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, parameter_index++), sizeof(int));

	if( (err = emf_update_mail(result_mail_data, result_attachment_data, result_attachment_data_count, result_meeting_request, sync_server)) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emf_update_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	local_result = 1;
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &(result_mail_data->mail_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &(result_mail_data->thread_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");

FINISH_OFF:
	if ( local_result == 0 ) {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");
	}

	EM_SAFE_FREE(mail_data_stream);
	EM_SAFE_FREE(attachment_data_list_stream);
	EM_SAFE_FREE(meeting_request_stream);

	if(result_mail_data)
		em_core_free_mail_data(&result_mail_data, 1, NULL);

	if(result_attachment_data)
		em_core_free_attachment_data(&result_attachment_data, result_attachment_data_count, NULL);

	if(result_meeting_request)
		em_storage_free_meeting_request(&result_meeting_request, 1, NULL);

	em_core_flush_memory();
	EM_DEBUG_FUNC_END();
}


void stb_move_thread_to_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0, thread_id = 0, move_always_flag = 0;
	int err = EMF_ERROR_NONE;
	char *target_mailbox_name = NULL;

	memcpy(&thread_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("thread_id [%d]", thread_id);
	
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("mailbox stream size [%d]", buffer_size);
	
	if(buffer_size > 0) {
		target_mailbox_name = (char*)em_core_malloc(buffer_size + 1);
		EM_NULL_CHECK_FOR_VOID(target_mailbox_name);
		if(target_mailbox_name) {
			memcpy(target_mailbox_name, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), buffer_size);
			/* Convert Byte stream to mailbox structure */
			if (target_mailbox_name)
				EM_DEBUG_LOG("Mailbox name - %s", target_mailbox_name);
			else
				EM_DEBUG_EXCEPTION("Mailbox name is NULL");
		}
	}

	memcpy(&move_always_flag, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), sizeof(int));
	EM_DEBUG_LOG("move_always_flag [%d]", move_always_flag);
	
	if(emf_mail_move_thread_to_mailbox(thread_id, target_mailbox_name, move_always_flag, &err))
		EM_DEBUG_LOG("emf_mail_move_thread_to_mailbox success");

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int))) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter fail");
		EM_SAFE_FREE(target_mailbox_name);
		return;
	}

	if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI fail");
		EM_SAFE_FREE(target_mailbox_name);
		return;
	}

	EM_SAFE_FREE(target_mailbox_name);
	EM_DEBUG_FUNC_END();
}

void stb_delete_thread(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int thread_id = 0, delete_always_flag = 0;
	unsigned int handle = 0;
	int err = EMF_ERROR_NONE;

	memcpy(&thread_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("thread_id [%d]", thread_id);

	memcpy(&delete_always_flag, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
	EM_DEBUG_LOG("delete_always_flag [%d]", delete_always_flag);
	
	if(emf_mail_delete_thread(thread_id, delete_always_flag, &handle, &err))
		EM_DEBUG_LOG("emf_mail_delete_thread success");

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int))) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter fail");
		return;
	}

	if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI fail");
		return;
	}
	EM_DEBUG_FUNC_END();
}

void stb_modify_seen_flag_of_thread(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int thread_id = 0, seen_flag = 0, on_server = 0;
	unsigned int handle = 0;
	int err = EMF_ERROR_NONE;

	memcpy(&thread_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("thread_id [%d]", thread_id);

	memcpy(&seen_flag, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
	EM_DEBUG_LOG("seen_flag [%d]", seen_flag);

	memcpy(&on_server, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), sizeof(int));
	EM_DEBUG_LOG("on_server [%d]", on_server);
	
	if(emf_mail_modify_seen_flag_of_thread(thread_id, seen_flag, on_server, &handle, &err))
		EM_DEBUG_LOG("emf_mail_modify_seen_flag_of_thread success");

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int))) {
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter fail");
		return;
	}

	if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI fail");
		return;
	}
	EM_DEBUG_FUNC_END();
}


void stb_mail_move(HIPC_API a_hAPI)
{
	int err = EMF_ERROR_NONE;
	int buffer_size = 0, num = 0, counter = 0;
	char *local_stream = NULL;
	emf_mailbox_t mailbox;

	/* Number of mail_ids */
	memcpy(&num, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("number of mails [%d]", num);

	/* mail_id */
	int mail_ids[num];	
	memcpy(mail_ids, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), num * sizeof(int));
	
	for(counter = 0; counter < num; counter++)
		EM_DEBUG_LOG("mailids [%d]", mail_ids[counter]);
	
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 2);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	
	if(buffer_size > 0) {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), buffer_size);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
			
			if (mailbox.name)
				EM_DEBUG_LOG("mailbox.name [%s]", mailbox.name);
			else
				EM_DEBUG_EXCEPTION("mailbox.name is NULL");
			
			if(emf_mail_move(mail_ids, num, &mailbox, EMF_MOVED_BY_COMMAND, 0, &err))
				EM_DEBUG_LOG("emf_mail_move success");
		}
	}
	
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter fail");
	
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI fail");
	EM_DEBUG_FUNC_END();
}

void stb_rule_delete(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int filter_id = 0;
	int err = EMF_ERROR_NONE;
	
	/* filter_id */
	memcpy(&filter_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));;
	
	if(emf_filter_delete(filter_id, &err))
		err = EMF_ERROR_NONE;

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
				
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_mail_add_attachment(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	emf_mailbox_t mailbox;
	int mail_id = -1;
	char* attachment_stream = NULL;
	emf_attachment_info_t* attachment = NULL;

	/* mailbox */
	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	/* mail_id */
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));

	/* attachment */
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 2);

	if(buffer_size > 0)	 {
		attachment_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(attachment_stream);
		if(attachment_stream) {
			memcpy(attachment_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), buffer_size);
			em_convert_byte_stream_to_attachment_info(attachment_stream, 1, &attachment);
			EM_SAFE_FREE(attachment_stream);
		}
	}

	emf_mail_add_attachment(&mailbox, mail_id, attachment, &err);
	
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
	if(EMF_ERROR_NONE == err) {
		EM_DEBUG_LOG("emf_mail_add_attachment -Success");
		
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &(attachment->attachment_id), sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter attachment_id failed ");
	}
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

	EM_SAFE_FREE(attachment);		
	EM_DEBUG_FUNC_END();
}

void stb_mail_get_attachment(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	emf_mailbox_t mailbox;
	int mail_id = -1;
	char* attachment_no = NULL;
	char* attachment_stream = NULL;
	emf_attachment_info_t* attachment = NULL;
	int size = 0;

	/* mailbox */
	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	/* mail_id */
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));

	/* attachment_no */
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 2);
	if(buffer_size > 0)	 {
		attachment_no = (char*)em_core_malloc(buffer_size+1);
		if(attachment_no) {
			memcpy(attachment_no, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), buffer_size);
		}
	}
	emf_mail_get_attachment(&mailbox, mail_id, attachment_no, &attachment, &err);

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
	if(EMF_ERROR_NONE == err) {
		EM_DEBUG_LOG("emf_mail_get_attachment - Success");
		/* attachment */
		attachment_stream = em_convert_attachment_info_to_byte_stream(attachment, &size);

		EM_NULL_CHECK_FOR_VOID(attachment_stream);
		
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, attachment_stream, size))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed  ");
	}
		if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
			EM_SAFE_FREE(attachment_no);		
			EM_SAFE_FREE(attachment_stream); 
			return;
		}

	EM_SAFE_FREE(attachment_no);		
	EM_SAFE_FREE(attachment_stream); 
	EM_DEBUG_FUNC_END();
}

void stb_mailbox_get_imap_mailbox_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	int account_id = 0;
	unsigned int handle = 0;  

	/* account_id */
	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));

	if(emf_mailbox_get_imap_mailbox_list(account_id, "", &handle, &err))
		err = EMF_ERROR_NONE;
		
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_LOG("ipcAPI_AddParameter local_result failed ");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_LOG("ipcAPI_AddParameter local_result failed ");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_LOG("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_mail_delete_attachment(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	emf_mailbox_t mailbox;
	int mail_id = -1;
	char* attachment_no = NULL;

	/* mailbox */
	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	/* mail_id */
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));

	/* attachment_no */
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 2);
	if(buffer_size > 0)	 {
		attachment_no = (char*)em_core_malloc(buffer_size+1);
		if(attachment_no) {
			memcpy(attachment_no, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), buffer_size);
		}
	}

	emf_mail_delete_attachment(&mailbox, mail_id, attachment_no, &err);
	
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

	EM_SAFE_FREE(attachment_no);		
	EM_DEBUG_FUNC_END();
}

void stb_download_attachment(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	emf_mailbox_t mailbox;
	int mail_id = 0;
	char *nth = NULL;
	unsigned int handle = 0;  
	
	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	EM_DEBUG_LOG("mail_id");
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));

	/* nth */
	EM_DEBUG_LOG("nth");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 2);
	if(buffer_size > 0)	 {
		nth = (char*)em_core_malloc(buffer_size+1);
		if(nth) {
			memcpy(nth, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), buffer_size);
		}
	}

	if(emf_mail_download_attachment(&mailbox, mail_id, nth, &handle, &err)) {
		err = EMF_ERROR_NONE;

		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
		EM_DEBUG_LOG(">>>>>>>>>> HANDLE = %d", handle);
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter handle failed ");		
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}
	else {
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
		/* Download handle - 17-Apr-09 */
		EM_DEBUG_LOG(">>>>>>>>>> HANDLE = %d", handle);
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter handle failed ");		
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}

	EM_SAFE_FREE(nth);		
	EM_DEBUG_FUNC_END();
}

void stb_get_info(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	char* local_info_stream = NULL;
	emf_mailbox_t mailbox = { 0 };
	int mail_id = 0;
	emf_mail_info_t* info = NULL;
	int size =0;

	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	EM_DEBUG_LOG("mail_id");
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));

	emf_mail_get_info(&mailbox, mail_id, &info, &err);

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
	if(EMF_ERROR_NONE == err) {
		EM_DEBUG_LOG("emf_mail_get_info - success");		
		/* Info */
		EM_DEBUG_LOG("emf_mail_get_info - convert mail Info");
		local_info_stream = em_convert_mail_info_to_byte_stream(info, &size);

		EM_NULL_CHECK_FOR_VOID(local_info_stream);

		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_info_stream, size))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed  ");
	}
	
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

	EM_SAFE_FREE(local_info_stream); 
	EM_DEBUG_FUNC_END();
}

void stb_get_header_info(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	emf_mailbox_t mailbox;
	int mail_id = 0;
	emf_mail_head_t* head = NULL;
	int size =0;
	char* local_head_stream = NULL;
	
	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	EM_DEBUG_LOG("mail_id");
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));

	emf_mail_get_head(&mailbox, mail_id, &head, &err);

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
		
	if(EMF_ERROR_NONE == err) {
		EM_DEBUG_LOG("success");			
		/* Head */
		EM_DEBUG_LOG("convert mail head");
		local_head_stream = em_convert_mail_head_to_byte_stream(head, &size);

		EM_NULL_CHECK_FOR_VOID(local_head_stream);

		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_head_stream, size))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
	}
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");

 	EM_SAFE_FREE(local_head_stream);	
	EM_DEBUG_FUNC_END();
}

void stb_get_mail_body_info(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	char* local_body_stream = NULL;
	emf_mailbox_t mailbox = { 0 };
	emf_mail_body_t* body = NULL;
	int size =0;
	int mail_id = 0;
	
	EM_DEBUG_LOG("mailbox");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			memcpy(local_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	EM_DEBUG_LOG("mail_id");
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));

	emf_mail_get_body(&mailbox, mail_id, &body, &err);

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");

	if(EMF_ERROR_NONE == err) {
		EM_DEBUG_LOG("success");
		/* Body */
		EM_DEBUG_LOG("convert mail Body[%p]", body);

		local_body_stream = em_convert_mail_body_to_byte_stream(body, &size);

		/* EM_NULL_CHECK_FOR_VOID(local_body_stream); */
		if ( !local_body_stream ) {
			EM_DEBUG_EXCEPTION("local_body_stream is NULL");
		}
		else {
			if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, local_body_stream, size))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
		}
	}	
	
	EM_DEBUG_LOG("Before ipcEmailStub_ExecuteAPI");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
		EM_SAFE_FREE(local_body_stream); 
		return;
	}
	
	EM_SAFE_FREE(local_body_stream); 
	EM_DEBUG_FUNC_END();
}

void stb_mail_send_saved(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_option_stream = NULL;
	emf_option_t sending_option;
	int account_id = 0;
	/* unsigned *handle = NULL; */
	
	/* account_id */
	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	
	/* Sending Option */
	EM_DEBUG_LOG("Sending option");
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 1);

	if(buffer_size > 0)  {
		local_option_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_option_stream);
		if(local_option_stream) {
			memcpy(local_option_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), buffer_size);
			em_convert_byte_stream_to_option(local_option_stream, &sending_option);
			/* EM_SAFE_FREE(local_option_stream); */
		}
	}

	EM_DEBUG_LOG("calling emf_mail_send_saved");
	if(emf_mail_send_saved(account_id, &sending_option, NULL, &err))
		err = EMF_ERROR_NONE;
		
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter result failed ");
		
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}	

void stb_mail_send_report(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_mail_stream = NULL;
	char* local_info_stream = NULL;
	emf_mail_t  mail = { 0 };

	/* head */
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	
	if(buffer_size > 0)	 {
		local_mail_stream = (char*)em_core_malloc(buffer_size+1);
		if(local_mail_stream) {
			memcpy(local_mail_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			/* Convert Byte stream to Mail Head structure */
			EM_DEBUG_LOG("Before em_convert_byte_stream_to_mail");
			mail.head = (emf_mail_head_t*)em_core_malloc(sizeof(emf_mail_head_t));
			/* EM_NULL_CHECK_FOR_VOID(mail.head); */
			if(NULL == mail.head)			 {
				EM_SAFE_FREE(local_mail_stream);
				return;
			}
			
			em_convert_byte_stream_to_mail_head(local_mail_stream, mail.head);
			EM_SAFE_FREE(local_mail_stream);
		}
	}

	/* body */
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 1);
	if(buffer_size > 0)	 {
		local_mail_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_mail_stream);
		if(local_mail_stream) {
			memcpy(local_mail_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), buffer_size);
			/* Convert Byte stream to Mail Body structure */
			EM_DEBUG_LOG("Before em_convert_byte_stream_to_mail");
			mail.body = (emf_mail_body_t*)em_core_malloc(sizeof(emf_mail_body_t));
			/* EM_NULL_CHECK_FOR_VOID(mail.body); */
			if(NULL == mail.body)			 {
				EM_SAFE_FREE(local_mail_stream);
				return;
			}
			
			em_convert_byte_stream_to_mail_body(local_mail_stream,  mail.body);
			EM_SAFE_FREE(local_mail_stream);
		}
	}	

	/* info */
	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 2);
	if(buffer_size > 0)	 {
		local_info_stream = (char*)em_core_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_info_stream);
		if(local_info_stream) {
			memcpy(local_info_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), buffer_size);
			/* Convert Byte stream to Mail Info structure */
			EM_DEBUG_LOG("Before em_convert_byte_stream_to_mail");
			mail.info = (emf_mail_info_t*)em_core_malloc(sizeof(emf_mail_info_t));
			/* EM_NULL_CHECK_FOR_VOID(mail.info); */

			if(NULL == mail.info)			 {
				EM_SAFE_FREE(local_info_stream);
				return;
			}
			
			em_convert_byte_stream_to_mail_info(local_info_stream,  mail.info);
			EM_SAFE_FREE(local_info_stream);
		}
	}	

	if(emf_mail_send_report(&mail, NULL, &err))
		err = EMF_ERROR_NONE;

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
				
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

	EM_SAFE_FREE(mail.head);
	EM_SAFE_FREE(mail.info);
	EM_SAFE_FREE(mail.body);	
	EM_DEBUG_FUNC_END();
}

void stb_mail_send_retry(HIPC_API a_hAPI)
{

	EM_DEBUG_FUNC_BEGIN();
	int mail_id = 0;
	int timeout_in_sec = 0;
	int err = EMF_ERROR_NONE;

	/* Mail_id */
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("mail_id [%d]", mail_id);

	/* timeout_in_sec */
	memcpy(&timeout_in_sec, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
	EM_DEBUG_LOG("mail_id [%d]", timeout_in_sec);

	if(emf_mail_send_retry(mail_id, timeout_in_sec,&err))
		EM_DEBUG_LOG("emf_mailbox_get_list - success");	

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
				
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_get_event_queue_status(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int on_sending = 0;
	int on_receiving = 0;
	
	/*get the network status */
	emf_get_event_queue_status(&on_sending, &on_receiving);
	
	/* on_sending */
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &on_sending, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");

	/* on_receving */
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &on_receiving, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");

	EM_DEBUG_LOG("stb_get_event_queue_status - Before Execute API");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");

	EM_DEBUG_FUNC_END();
}

void stb_cancel_job(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id = 0;
	int handle = 0;
	int err = EMF_ERROR_NONE;

	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("account_id [%d]", account_id);

	memcpy(&handle, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
	EM_DEBUG_LOG("handle [%d]", handle);	

	if(emf_cancel_job(account_id, handle, &err))
		err = EMF_ERROR_NONE;

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_get_pending_job(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	emf_action_t action = -1;
	int account_id = 0;
	int mail_id = -1;
	int err = EMF_ERROR_NONE;
	emf_event_status_type_t status = 0;

	memcpy(&action, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("action [%d]", action);

	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
	EM_DEBUG_LOG("account_id [%d]", account_id);

	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 2), sizeof(int));
	EM_DEBUG_LOG("mail_id [%d]", mail_id);

	if(emf_get_pending_job(action, account_id, mail_id, &status))
		err = EMF_ERROR_NONE;
	else
		err = EMF_ERROR_UNKNOWN;

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter local_result failed ");
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &status, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter status failed ");
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_print_receiving_event_queue_via_debug_msg(HIPC_API a_hAPI)
{
	emf_event_t *event_queue = NULL;
	int event_active_queue = 0, err, i;

	em_core_get_receiving_event_queue(&event_queue, &event_active_queue, &err);	

	EM_DEBUG_LOG("======================================================================");
	EM_DEBUG_LOG("Event active index [%d]", event_active_queue);
	EM_DEBUG_LOG("======================================================================");
	for(i = 1; i < 32; i++)
		EM_DEBUG_LOG("event[%d] : type[%d], account_id[%d], arg[%d], status[%d]", i, event_queue[i].type, event_queue[i].account_id, event_queue[i].event_param_data_4, event_queue[i].status);
	EM_DEBUG_LOG("======================================================================");
	EM_DEBUG_LOG("======================================================================");
	
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");

	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");
	EM_DEBUG_FUNC_END();
}

void stb_send_mail_cancel_job(HIPC_API a_hAPI)
{

	EM_DEBUG_FUNC_BEGIN();
	int mail_id = 0;
	int err = EMF_ERROR_NONE;
	int account_id = 0;


	/* account_id */
	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("account_id [%d]", account_id);

	
	/* Mail_id */
	memcpy(&mail_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 1), sizeof(int));
	EM_DEBUG_LOG("mail_id [%d]", mail_id);


	if(emf_mail_send_mail_cancel_job(account_id,mail_id,&err))
		EM_DEBUG_LOG("success");		

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed");
				
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed");
	EM_DEBUG_FUNC_END();
}

void stb_find_mail_on_server(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_FUNC_END();
}

void stb_account_create_with_validation(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int local_result = 0;
	unsigned int handle = 0;  
	char* local_account_stream = NULL;
	emf_account_t *account;
	int err = EMF_ERROR_NONE;

	buffer_size = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);

	if(buffer_size > 0)	 {
		local_account_stream = (char*)em_core_malloc(buffer_size+1);
		if(local_account_stream) {
			memcpy(local_account_stream, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), buffer_size);
			/* Convert account stream to structure */
			EM_DEBUG_LOG("Before Convert_ByteStream_To_Account");
			
			account = em_core_account_get_new_account_ref(); 
			em_convert_byte_stream_to_account(local_account_stream, account);
			EM_SAFE_FREE(local_account_stream);
		
			account->account_id = NEW_ACCOUNT_ID;

			if (account->account_name)
				EM_DEBUG_LOG("Account name - %s", account->account_name);
			if(account->email_addr)
				EM_DEBUG_LOG("Email Address - %s", account->email_addr);
		
			if(emf_account_validate_and_create(account, &handle, &err)) {	
				
#ifdef __FEATURE_AUTO_POLLING__
				/*  start auto polling, if check_interval not zero */
				if(account->check_interval > 0) {
					if(!emf_add_polling_alarm( account->account_id,account->check_interval))
						EM_DEBUG_EXCEPTION("emf_add_polling_alarm[NOTI_ACCOUNT_ADD] : start auto poll failed >>> ");
				}
#endif /*  __FEATURE_AUTO_POLLING__ */
				/*  add account details to contact DB  */
				/*  emf_account_insert_accountinfo_to_contact(account); */

				local_result = 1;
				if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
					EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");		
				if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
					EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
				if (!ipcEmailStub_ExecuteAPI(a_hAPI))
					EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
				}
			else {
				EM_DEBUG_EXCEPTION("emf_account_validate_and_create fail ");
				goto FINISH_OFF;
			}
		}
		else {
			err = EMF_ERROR_INVALID_PARAM;
			goto FINISH_OFF;
		}
	}	
	else {
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
FINISH_OFF:
	if ( local_result == 0 ) {			
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed : local_result ");
		if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed : err");
		if (!ipcEmailStub_ExecuteAPI(a_hAPI))
			EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	}
	EM_DEBUG_FUNC_END();
}

void stb_account_backup(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();

#ifdef __FEATURE_BACKUP_ACCOUNT__	
	char *file_path = NULL;
	int local_result = 0, err_code = 0, handle = 0, file_path_length = 0;
	
	/* file_path_length */
	file_path_length = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);
	if(file_path_length > 0) {
		EM_DEBUG_LOG("file_path_length [%d]", file_path_length);
		file_path = em_core_malloc(file_path_length + 1);
		memcpy(file_path, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), file_path_length);;
		EM_DEBUG_LOG("file_path [%s]", file_path);
		local_result = em_core_backup_accounts((const char*)file_path, &err_code);
	}

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_LOG("ipcEmailAPI_AddParameter failed ");

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err_code, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
				
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter result failed ");
				
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

	EM_SAFE_FREE(file_path);
#endif /*  __FEATURE_BACKUP_ACCOUNT__ */
	EM_DEBUG_FUNC_END();
}

void stb_account_restore(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
#ifdef __FEATURE_BACKUP_ACCOUNT__
	char *file_path = NULL;
	int local_result = 0, err_code = 0, handle = 0, file_path_length = 0;
	
	/* file_path_length */
	file_path_length = ipcEmailAPI_GetParameterLength(a_hAPI, ePARAMETER_IN, 0);
	if(file_path_length > 0)  {
		EM_DEBUG_LOG("file_path_length [%d]", file_path_length);
		file_path = em_core_malloc(file_path_length + 1);
		memcpy(file_path, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), file_path_length);
		EM_DEBUG_LOG("file_path [%s]", file_path);
		local_result = em_core_restore_accounts((const char*)file_path, &err_code);
	}

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_LOG("ipcEmailAPI_AddParameter failed ");

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err_code, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
				
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter result failed ");
				
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");

	EM_SAFE_FREE(file_path);
#endif /*  __FEATURE_BACKUP_ACCOUNT__ */
	EM_DEBUG_FUNC_END();
}


void stb_account_get_password_length(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id = 0;
	int local_result = 0;
	int err_code = 0;
	int password_length = 0;
	
	/* account_id */
	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));;
	local_result = em_storage_get_password_length_of_account(account_id, &password_length,&err_code);

	EM_DEBUG_LOG("password_length [%d]", password_length);

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_LOG("ipcEmailAPI_AddParameter failed ");

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err_code, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");
				
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &password_length, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter result failed ");
				
	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");
	EM_DEBUG_FUNC_END();
}


void stb_ping_service(HIPC_API a_hAPI)
{
	int err = EMF_ERROR_NONE;
	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");

	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");	
	EM_DEBUG_FUNC_END();
}

void stb_update_notification_bar_for_unread_mail(HIPC_API a_hAPI)
{
	int err = EMF_ERROR_NONE, account_id;

	/* account_id */
	memcpy(&account_id, ipcEmailAPI_GetParameter(a_hAPI, ePARAMETER_IN, 0), sizeof(int));
	EM_DEBUG_LOG("account_id [%d]", account_id);

	if(!em_core_finalize_sync(account_id, &err)) {
		EM_DEBUG_EXCEPTION("em_core_finalize_sync failed [%d]", err);
	}

	if(!ipcEmailAPI_AddParameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter failed ");

	if (!ipcEmailStub_ExecuteAPI(a_hAPI))
		EM_DEBUG_EXCEPTION("ipcEmailStub_ExecuteAPI failed  ");	
	EM_DEBUG_FUNC_END();
}


void stb_API_mapper(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int nAPIID = ipcEmailAPI_GetAPIID(a_hAPI);
	
	switch(nAPIID) {
		case _EMAIL_API_ADD_ACCOUNT:
			stb_account_create(a_hAPI);
			break;

		case _EMAIL_API_ADD_MAIL_OLD:
			stb_mail_save_to_mailbox(a_hAPI);
			break;

		case _EMAIL_API_GET_MAIL:
			stb_mail_get_mail(a_hAPI);
			break;

		case _EMAIL_API_ADD_MAILBOX:
			stb_mailbox_create(a_hAPI);
			break;

		case _EMAIL_API_DELETE_MAILBOX:
			stb_mailbox_delete(a_hAPI);
			break;
		case _EMAIL_API_UPDATE_MAILBOX:
			stb_mailbox_update(a_hAPI);
			break;

		case _EMAIL_API_SET_MAIL_SLOT_SIZE:
			stb_mailbox_set_mail_slot_size(a_hAPI);
			break;

		case _EMAIL_API_SEND_MAIL:
			stb_mail_send(a_hAPI);
			break;
		
		case _EMAIL_API_GET_MAILBOX_COUNT:
			stb_mail_get_mailbox_count(a_hAPI);
			break;

		case _EMAIL_API_GET_MAILBOX_LIST:
			stb_mailbox_get_list(a_hAPI);
			break;

		case _EMAIL_API_GET_SUBMAILBOX_LIST:
			break;
			
		case _EMAIL_API_SYNC_HEADER:
			stb_mailbox_sync_header(a_hAPI);
			break;
			
		case _EMAIL_API_DOWNLOAD_BODY:
			stb_mailbox_download_body(a_hAPI);
			break;

		case _EMAIL_API_CLEAR_DATA:
			stb_clear_mail_data (a_hAPI);
			break;
			
		case _EMAIL_API_UPDATE_MAIL_OLD:
			stb_update_mail_old(a_hAPI);
			break;
			
		case _EMAIL_API_DELETE_ALL_MAIL:
			stb_mail_delete_all(a_hAPI);
			break;
			
		case _EMAIL_API_DELETE_MAIL:
			stb_mail_delete(a_hAPI);
			break;

		case _EMAIL_API_MODIFY_MAIL_FLAG:
			stb_modify_mail_flag(a_hAPI);
			break;

		case _EMAIL_API_MODIFY_MAIL_EXTRA_FLAG:
			stb_modify_mail_extra_flag(a_hAPI);
			break;		

 		case _EMAIL_API_ADD_RULE:
			stb_rule_add(a_hAPI);
			break;

		case _EMAIL_API_GET_RULE:
			stb_rule_get(a_hAPI);
			break;

		case _EMAIL_API_GET_RULE_LIST:
			stb_rule_get_list(a_hAPI);
			break;

		case _EMAIL_API_FIND_RULE:
			stb_rule_find(a_hAPI);
			break;

		case _EMAIL_API_UPDATE_RULE:
			stb_rule_change(a_hAPI);
			break;
			
		case _EMAIL_API_DELETE_RULE:
			stb_rule_delete(a_hAPI);
			break;

		case _EMAIL_API_MOVE_MAIL:
			stb_mail_move(a_hAPI);
			break;

		case _EMAIL_API_MOVE_ALL_MAIL:
			stb_mail_move_all_mails(a_hAPI);
			break;	

		case _EMAIL_API_SET_FLAGS_FIELD:
			stb_set_flags_field(a_hAPI);
			break;

		case _EMAIL_API_ADD_MAIL:
			stb_add_mail(a_hAPI);
			break;

		case _EMAIL_API_UPDATE_MAIL:
			stb_update_mail(a_hAPI);
			break;

		case _EMAIL_API_MOVE_THREAD_TO_MAILBOX:
			stb_move_thread_to_mailbox(a_hAPI);
			break;

		case _EMAIL_API_DELETE_THREAD:
			stb_delete_thread(a_hAPI);
			break;

		case _EMAIL_API_MODIFY_SEEN_FLAG_OF_THREAD:
			stb_modify_seen_flag_of_thread(a_hAPI);
			break;

		case _EMAIL_API_DELETE_ACCOUNT:
			stb_account_delete(a_hAPI);
			break;

		case _EMAIL_API_UPDATE_ACCOUNT:
			stb_account_update(a_hAPI);
			break;

		case _EMAIL_API_ADD_ATTACHMENT:
			stb_mail_add_attachment(a_hAPI);
			break;

		case _EMAIL_API_GET_IMAP_MAILBOX_LIST:
			stb_mailbox_get_imap_mailbox_list(a_hAPI);
			break;
			
		case _EMAIL_API_GET_ATTACHMENT:
			stb_mail_get_attachment(a_hAPI);
			break;

		case _EMAIL_API_DELETE_ATTACHMENT:
			stb_mail_delete_attachment(a_hAPI);
			break;

		case _EMAIL_API_DOWNLOAD_ATTACHMENT:
			stb_download_attachment(a_hAPI);
			break;

		case _EMAIL_API_GET_INFO:
			stb_get_info(a_hAPI);
			break;

		case _EMAIL_API_GET_HEADER_INFO:
			stb_get_header_info(a_hAPI);
			break;

		case _EMAIL_API_GET_BODY_INFO:
			stb_get_mail_body_info(a_hAPI);
			break;

		case _EMAIL_API_GET_ACCOUNT_LIST:
			stb_account_get_list(a_hAPI);
			break;

		case _EMAIL_API_MAIL_SEND_SAVED:
			stb_mail_send_saved(a_hAPI);
			break;

		case _EMAIL_API_MAIL_SEND_REPORT:
			stb_mail_send_report(a_hAPI);
			break;

		case _EMAIL_API_CANCEL_JOB:
			stb_cancel_job(a_hAPI);
			break;

		case _EMAIL_API_GET_PENDING_JOB:
			stb_get_pending_job(a_hAPI);
			break;

		case _EMAIL_API_NETWORK_GET_STATUS:
			stb_get_event_queue_status(a_hAPI);
			break;

		case _EMAIL_API_SEND_RETRY:
			stb_mail_send_retry(a_hAPI);
			break;

		case _EMAIL_API_VALIDATE_ACCOUNT :
			stb_account_validate(a_hAPI);
			break;

		case _EMAIL_API_SEND_MAIL_CANCEL_JOB :
			stb_send_mail_cancel_job(a_hAPI);
			break;

		case _EMAIL_API_FIND_MAIL_ON_SERVER :
			stb_find_mail_on_server(a_hAPI);
			break;

		case _EMAIL_API_ADD_ACCOUNT_WITH_VALIDATION :
			stb_account_create_with_validation(a_hAPI);
			break;

		case _EMAIL_API_BACKUP_ACCOUNTS:
			stb_account_backup(a_hAPI);
			break;

		case _EMAIL_API_RESTORE_ACCOUNTS:
			stb_account_restore(a_hAPI);
			break;
		
		case _EMAIL_API_GET_PASSWORD_LENGTH_OF_ACCOUNT:
			stb_account_get_password_length(a_hAPI);
			break;

		case _EMAIL_API_PRINT_RECEIVING_EVENT_QUEUE :
			stb_print_receiving_event_queue_via_debug_msg(a_hAPI);
			break;
		
		case _EMAIL_API_PING_SERVICE :
			stb_ping_service(a_hAPI);
			break;

		case _EMAIL_API_UPDATE_NOTIFICATION_BAR_FOR_UNREAD_MAIL :
			stb_update_notification_bar_for_unread_mail(a_hAPI);
			break;
	}
	EM_DEBUG_FUNC_END();
}




void hibernation_enter_callback()
{
	EM_DEBUG_FUNC_BEGIN();
	em_storage_db_close(NULL);
	EM_DEBUG_FUNC_END();
}

void hibernation_leave_callback()
{
	EM_DEBUG_FUNC_BEGIN();
	em_storage_db_open(NULL);
	EM_DEBUG_FUNC_END();
}

EXPORT_API int main(int argc, char *argv[])
{
	/* Do the Email Engine Initialization 
       1. Create all DB tables and load the email engine */
	EM_DEBUG_LOG("Email service begin");
	int err = 0, ret;

	signal(SIGPIPE, SIG_IGN); /*  to ignore signal 13(SIGPIPE) */

	emf_init(&err);
	EM_DEBUG_LOG("emf_emn_noti_init Start");
#ifdef USE_OMA_EMN
	emf_emn_noti_init();
#endif

	EM_DEBUG_LOG("ipcEmailStub_Initialize Start");
	
	ret = ipcEmailStub_Initialize(stb_API_mapper);

	if(ret == true)
		EM_DEBUG_LOG("ipcEmailStub_Initialize Success");
	else
		EM_DEBUG_EXCEPTION("ipcEmailStub_Initialize failed");

	/* Start auto polling */
#ifdef __FEATURE_AUTO_POLLING__
	emf_auto_polling(&err);
#endif

	int fd_HibernationNoti;

    fd_HibernationNoti = heynoti_init();

    if(fd_HibernationNoti == -1)
    	EM_DEBUG_EXCEPTION("heynoti_init failed");
    else	 {	
		EM_DEBUG_LOG("heynoti_init Success");
		ret = heynoti_subscribe(fd_HibernationNoti, "HIBERNATION_ENTER", hibernation_enter_callback, (void *)fd_HibernationNoti);
		EM_DEBUG_LOG("heynoti_subscribe returns %d", ret);
	    ret = heynoti_subscribe(fd_HibernationNoti, "HIBERNATION_LEAVE", hibernation_leave_callback, (void *)fd_HibernationNoti);
	    EM_DEBUG_LOG("heynoti_subscribe returns %d", ret);
		ret = heynoti_attach_handler(fd_HibernationNoti);
		EM_DEBUG_LOG("heynoti_attach_handler returns %d", ret);
	}
	
	GMainLoop *mainloop;

	mainloop = g_main_loop_new (NULL, 0);

	g_type_init();

	g_main_loop_run (mainloop);

	if(fd_HibernationNoti != -1) {
		heynoti_unsubscribe(fd_HibernationNoti, "HIBERNATION_ENTER", hibernation_enter_callback);
		heynoti_unsubscribe(fd_HibernationNoti, "HIBERNATION_LEAVE", hibernation_leave_callback);
		heynoti_close(fd_HibernationNoti);
	}
	
	EM_DEBUG_FUNC_END();
	return 0;
}

