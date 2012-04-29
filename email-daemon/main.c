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
#include <glib-object.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <heynoti/heynoti.h>
#include <signal.h>
#include <time.h>

#include "email-daemon.h"
#include "email-ipc.h"
#include "email-utilities.h"
#include "email-debug-log.h"
#include "email-daemon-auto-poll.h"   
#include "email-daemon-account.h"
#include "email-convert.h"
#include "email-internal-types.h" 
#include "email-types.h"
#include "email-core-account.h" 
#include "email-core-mail.h" 
#include "email-core-smtp.h"
#include "email-core-event.h" 
#include "email-core-global.h" 
#include "email-core-mailbox.h" 
#include "email-core-utils.h"
#include "email-storage.h"    

void stb_create_account(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int local_result = 0;
	char* local_account_stream = NULL;
	emf_account_t account;
	int err = EMF_ERROR_NONE;

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);

	if(buffer_size <= 0)	{
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_account_stream = (char*)em_malloc(buffer_size+1);
	if(!local_account_stream) {
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_account_stream);
	/* Convert account stream to structure */
	EM_DEBUG_LOG("Before Convert_ByteStream_To_Account");
	em_convert_byte_stream_to_account(local_account_stream, &account);
	EM_SAFE_FREE(local_account_stream);

	if (account.account_name)
		EM_DEBUG_LOG("Account name - %s", account.account_name);
	if(account.email_addr)
		EM_DEBUG_LOG("Email Address - %s", account.email_addr);
	
	if(!emdaemon_create_account(&account, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_create_account fail ");
		goto FINISH_OFF;
	}

#ifdef __FEATURE_AUTO_POLLING__
	/* start auto polling, if check_interval not zero */
	if(account.check_interval > 0) {
		if(!emdaemon_add_polling_alarm( account.account_id,account.check_interval))
			EM_DEBUG_EXCEPTION("emdaemon_add_polling_alarm[NOTI_ACCOUNT_ADD] : start auto poll failed >>> ");
	}
#endif
	/* add account details to contact DB */
	emdaemon_insert_accountinfo_to_contact(&account);
	local_result = 1;
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
	
	EM_DEBUG_LOG("[3] APPID[%d], APIID [%d]", emipc_get_app_id(a_hAPI), emipc_get_api_id(a_hAPI));
	EM_DEBUG_LOG("account id[%d]", account.account_id);
	
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &(account.account_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

FINISH_OFF:
	if ( local_result == 0 ) {			
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed : local_result ");
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed : err");
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}
	EM_DEBUG_FUNC_END();
}

void stb_delete_account(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id = 0;
	int local_result = 0;
	int err = EMF_ERROR_NONE;
	
	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	if(!emdaemon_delete_account(account_id, &err)) {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_LOG("emipc_add_parameter failed ");
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
				
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
		goto FINISH_OFF;
	}

#ifdef __FEATURE_AUTO_POLLING__
	/* stop auto polling for this acount */
	if(emdaemon_check_auto_polling_started(account_id)) {
		if(!emdaemon_remove_polling_alarm(account_id))
			EM_DEBUG_EXCEPTION("emdaemon_remove_polling_alarm[ NOTI_ACCOUNT_DELETE] : remove auto poll failed >>> ");
	}
#endif
	local_result = 1;

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
			
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

FINISH_OFF:

	EM_DEBUG_FUNC_END();
}

void stb_update_account(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id = 0, buffer_size = 0, local_result = 0, with_validation = 0;
	char* local_account_stream = NULL;
	emf_account_t  *new_account_info = NULL;
	int err = EMF_ERROR_NONE;
	unsigned int handle = 0;  

	/* filter_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	
	if(buffer_size > 0)	 {
		local_account_stream = (char*)em_malloc(buffer_size+1);
		if(local_account_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, buffer_size, local_account_stream);
			/* Convert Byte stream to account structure */
			EM_DEBUG_LOG("Before Convert_ByteStream_To_account");
			new_account_info = (emf_account_t*)em_malloc(sizeof(emf_account_t));

			if(NULL == new_account_info)			 {
				EM_SAFE_FREE(local_account_stream);
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			em_convert_byte_stream_to_account(local_account_stream, new_account_info);
			EM_SAFE_FREE(local_account_stream);
		}
		else {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
			
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &with_validation);
	}

	if ( NULL == new_account_info ) {
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	emf_account_t  *old_account_info = NULL;
	
	if(!emdaemon_get_account(account_id, GET_FULL_DATA, &old_account_info, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_get_account failed ");
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
		emdaemon_validate_account_and_update(account_id, new_account_info, &handle, &err);
		local_result = 1;
	}
	else {
		if(emdaemon_update_account(account_id, new_account_info, &err)) {
#ifdef __FEATURE_AUTO_POLLING__
			int  old_check_interval = old_account_info->check_interval;
			if( old_check_interval == 0 && new_account_info->check_interval > 0) {
				if(!emdaemon_add_polling_alarm( account_id,new_account_info->check_interval))
					EM_DEBUG_EXCEPTION("emdaemon_add_polling_alarm[ CHANGEACCOUNT] : start auto poll failed >>> ");
				
			}
			else if( (old_check_interval > 0) && (new_account_info->check_interval == 0)) {
				if(!emdaemon_remove_polling_alarm(account_id))
					EM_DEBUG_EXCEPTION("emdaemon_remove_polling_alarm[ CHANGEACCOUNT] : start auto poll failed >>> ");
			}
			else if(old_check_interval != new_account_info->check_interval) {
				if(!emdaemon_remove_polling_alarm(account_id)) {
					EM_DEBUG_EXCEPTION("emdaemon_remove_polling_alarm[ CHANGEACCOUNT] : start auto poll failed >>> ");
					goto FINISH_OFF;
				}		
				if(!emdaemon_add_polling_alarm( account_id,new_account_info->check_interval))
					EM_DEBUG_EXCEPTION("emdaemon_add_polling_alarm[ CHANGEACCOUNT] : start auto poll failed >>> ");
			}
#endif
			emdaemon_update_accountinfo_to_contact(old_account_info,new_account_info);
			local_result = 1;
		}
		else {
			EM_DEBUG_EXCEPTION("emdaemon_update_account failed [%d]", err);
			goto FINISH_OFF;
		}
		
	}
	
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter for result failed");

	if(with_validation) {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter for handle failed");
	}
		
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

FINISH_OFF:
	if ( local_result == 0 ) {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
				
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
	}
	EM_SAFE_FREE(new_account_info);		
	EM_SAFE_FREE(old_account_info);
	EM_DEBUG_FUNC_END();
}

void stb_validate_account(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();

	int account_id = 0;
	int local_result = 0;
	int err_code = 0;
	int handle = 0;
	
	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	local_result = emdaemon_validate_account(account_id, (unsigned*)&handle,&err_code);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_LOG("emipc_add_parameter failed ");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err_code, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
				
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter result failed ");
				
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

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
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);

	account = emdaemon_get_account_reference(account_id);
	if(account != NULL) {
		EM_DEBUG_LOG("emdaemon_get_account_reference success");
		local_result = 1;
		
		/* result */
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
		
		/* Address */
		local_stream = em_convert_account_to_byte_stream(account, &size);

		EM_NULL_CHECK_FOR_VOID(local_stream);

		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

		if (!emipc_execute_stub_api(a_hAPI)) {
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
		 	EM_SAFE_FREE(local_stream);
			return;
		}

	}
	else {
		EM_DEBUG_LOG("emdaemon_get_account_reference failed");
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
		if (!emipc_execute_stub_api(a_hAPI)) {
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
		 	EM_SAFE_FREE(local_stream);
			return;
		}
	}

 	EM_SAFE_FREE(local_stream);
	EM_DEBUG_FUNC_END();
}

void stb_get_account_list(HIPC_API a_hAPI)
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
	
	if(emdaemon_get_account_list(&account_list, &count, &err)) {
		EM_DEBUG_LOG("emdaemon_get_account_list success");
		local_result = 1;
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");

		EM_DEBUG_LOG("Count [%d]", count);
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &count, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter count failed ");

		for(counter=0; counter<count; counter++) {
			EM_DEBUG_LOG("Name - %s", account_list[counter].account_name);

			local_stream = em_convert_account_to_byte_stream(account_list+counter, &size);

			EM_NULL_CHECK_FOR_VOID(local_stream);

			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
				EM_DEBUG_EXCEPTION("Add  Param mailbox failed  ");

			size = 0;
		 	EM_SAFE_FREE(local_stream);
		}

		emdaemon_free_account(&account_list, count, NULL);  
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}

	else {
		EM_DEBUG_LOG("emdaemon_get_account_list failed");
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed : err");
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
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
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mail_id);
	EM_DEBUG_LOG("mail_id[%d]", mail_id);

	EM_DEBUG_LOG("i_flag");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &i_flag);
	EM_DEBUG_LOG("i_flag[%d]", i_flag);

	EM_DEBUG_LOG("Sticky flag");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &sticky_flag);
	EM_DEBUG_LOG(">>> STICKY flag Value [ %d] ", sticky_flag);

	EM_DEBUG_LOG("onserver");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 3, sizeof(int), &onserver);
	EM_DEBUG_LOG("onserver[%d]", onserver);

	EM_DEBUG_LOG("Flag Change 1>>> ");
	if(!em_convert_mail_int_to_flag(i_flag, &new_flag, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_int_to_flag failed");
		return;
	}
	if(emdaemon_modify_flag(mail_id, new_flag, onserver, sticky_flag, &err))		
		EM_DEBUG_LOG("emdaemon_modify_flag - success");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
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
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mail_id);

	EM_DEBUG_LOG("extra_flag");
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);

	if(buffer_size > 0) {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, buffer_size, local_stream);
			em_convert_byte_stream_to_extra_flags(local_stream, &new_flag);
			EM_SAFE_FREE(local_stream);
		}

		if(emdaemon_modify_extra_flag(mail_id, new_flag, &err))		
			EM_DEBUG_LOG("emdaemon_modify_extra_flag - success");
		
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
		
			if (!emipc_execute_stub_api(a_hAPI))
				EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
		
	}
	EM_DEBUG_FUNC_END();
}

void stb_get_mail_count_of_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int local_result = 0;
	char* local_stream = NULL;
	emf_mailbox_t mailbox;
	int total_count = 0;
	int unseen= 0;

	EM_DEBUG_LOG("mailbox");
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	{
		local_stream = (char*)em_malloc(buffer_size+1);
		if(!local_stream) {
			EM_DEBUG_EXCEPTION("EMF_ERROR_OUT_OF_MEMORY");
			goto FINISH_OFF;
		}
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
		em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
		EM_SAFE_FREE(local_stream);
	}

	/*get the Mailbox Count */
	if (!emdaemon_get_mail_count_of_mailbox(&mailbox, &total_count, &unseen, NULL)) {
		EM_DEBUG_EXCEPTION("emdaemon_get_mail_count_of_mailbox - failed");
		goto FINISH_OFF;
	}
	else {
		EM_DEBUG_LOG("emdaemon_get_mail_count_of_mailbox - success");
		local_result = 1;
	}

FINISH_OFF:

	if(local_result) {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");

		/* Totol count of mails */
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &total_count, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");

		/* Unread Mail */
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &unseen, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");

		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}
	else {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
	}

	EM_DEBUG_FUNC_END();
}


/* sowmya.kr, 10-May-2010, changes for API improvement */
void stb_sync_header(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	emf_mailbox_t mailbox, *pointer_mailbox = NULL;
	int handle = -1;
	
	EM_DEBUG_LOG("mailbox");
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0) {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(!local_stream) {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
		em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
		EM_SAFE_FREE(local_stream);
		pointer_mailbox = &mailbox;
	}

  if(emdaemon_sync_header(pointer_mailbox, (unsigned*)&handle, &err)) {
		EM_DEBUG_LOG("emdaemon_sync_header success ");
  }

FINISH_OFF:
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_download_body(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_stream = NULL;
	emf_mailbox_t mailbox;
	int mail_id = 0;
	int has_attachment = 0;
	unsigned int handle = 0;
	
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	{
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	/* Mail Id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mail_id);

	/* Has Attachment */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &has_attachment);

	/*Download Body */
	if (!emdaemon_download_body(&mailbox, mail_id, 1, has_attachment, &handle, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_download_body - failed");
		goto FINISH_OFF;
	}

	err = EMF_ERROR_NONE;

FINISH_OFF:

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
	EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_DEBUG_FUNC_END();
}


void stb_create_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int 		buffer_size 			= 0;
	int 		err = EMF_ERROR_NONE;
	char 		*local_stream  = NULL;
	int 		on_server 		= 0;
	emf_mailbox_t mailbox;
	int handle = 0; /* Added for cancelling mailbox creating  */

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);

	if(buffer_size > 0) {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &on_server);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
					
			if (mailbox.name)
				EM_DEBUG_LOG("Mailbox name - %s", mailbox.name);
			
			if(emdaemon_add_mailbox(&mailbox, on_server, (unsigned*)&handle, &err))
				err = EMF_ERROR_NONE;
			
			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed 1");
			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed 2");
			
			if (!emipc_execute_stub_api(a_hAPI)) {
				EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
				return;
			}
		}
	}	
	EM_DEBUG_FUNC_END();
}


void stb_delete_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int 		buffer_size 			= 0;
	int		 err = EMF_ERROR_NONE;
	char 	*local_stream  = NULL;
	int 		on_server		= 0;
	emf_mailbox_t mailbox;
	int handle = 0; /* Added for cancelling mailbox deleting */

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	
	EM_DEBUG_LOG("size [%d]", buffer_size);

	if(buffer_size > 0) {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &on_server);
			
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);

			if (mailbox.name)
				EM_DEBUG_LOG("Mailbox name - %s", mailbox.name);
			if(emdaemon_delete_mailbox(&mailbox, on_server, (unsigned*)&handle, &err))
				err = EMF_ERROR_NONE;
			
			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			
			if (!emipc_execute_stub_api(a_hAPI))
				EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
			
		}
	}	
	EM_DEBUG_FUNC_END();
}

void stb_update_mailbox(HIPC_API a_hAPI)
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

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	if(buffer_size > 0)	  {
		old_mailbox_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(old_mailbox_stream);
		if(old_mailbox_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, old_mailbox_stream);
			old_mailbox = (emf_mailbox_t*)em_malloc(sizeof(emf_mailbox_t));
			if ( old_mailbox == NULL ) {
				EM_DEBUG_EXCEPTION("em_malloc failed.");
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

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	if(buffer_size > 0)	  {
		new_mailbox_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(new_mailbox_stream);
		if(new_mailbox_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, buffer_size, new_mailbox_stream);
			new_mailbox = (emf_mailbox_t*)em_malloc(sizeof(emf_mailbox_t));
			if ( new_mailbox == NULL ) {
				EM_DEBUG_EXCEPTION("em_malloc failed.");
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
	
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &on_server);
	
	if(emdaemon_update_mailbox(old_mailbox, new_mailbox, on_server, (unsigned*)&handle, &err))
		err = EMF_ERROR_NONE;

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
			

FINISH_OFF:
	EM_SAFE_FREE(old_mailbox_stream);
	EM_SAFE_FREE(new_mailbox_stream);

	emdaemon_free_mailbox(&old_mailbox, 1, NULL);
	emdaemon_free_mailbox(&new_mailbox, 1, NULL);
	EM_DEBUG_FUNC_END();
}


void stb_set_mail_slot_size_of_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size  = 0;
	int	err    = EMF_ERROR_NONE;
	int handle = 0; 
	int account_id = 0, mail_slot_size = 0;
	char *mailbox_name = NULL;


	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id[%d]", account_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mail_slot_size);
	EM_DEBUG_LOG("mail_slot_size[%d]", mail_slot_size);

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 2);
	EM_DEBUG_LOG("mailbox name string size[%d]", buffer_size);
	if(buffer_size > 0)	  {
		mailbox_name = (char*)em_malloc(buffer_size + 1);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, buffer_size, mailbox_name);
		EM_DEBUG_LOG("mailbox_name [%s]", mailbox_name);
	}
	
	if(emdaemon_set_mail_slot_size_of_mailbox(account_id, mailbox_name, mail_slot_size, (unsigned*)&handle, &err))
		err = EMF_ERROR_NONE;
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 1");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 2");
	
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_SAFE_FREE(mailbox_name);
	EM_DEBUG_FUNC_END();
}

void stb_send_mail(HIPC_API a_hAPI)
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
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0) {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
			mailbox = (emf_mailbox_t*)em_malloc(sizeof(emf_mailbox_t));
			em_convert_byte_stream_to_mailbox(local_stream, mailbox);

			if (!emstorage_get_mailboxname_by_mailbox_type(mailbox->account_id,EMF_MAILBOX_TYPE_DRAFT,&mailbox_name, false, &err))  {
				EM_DEBUG_EXCEPTION("emstorage_get_mailboxname_by_mailbox_type failed [%d]", err);
		
				goto FINISH_OFF;
			}
			mailbox->name = mailbox_name;
		}
	}

	/* Mail_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mail_id);
	EM_DEBUG_LOG("mail_id [%d]", mail_id);

	/* Sending Option */
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 2);

	if(buffer_size > 0) {
		local_option_stream = (char*)em_malloc(buffer_size+1);
		
		if(NULL == local_option_stream) {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		
		if(local_option_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, buffer_size, local_option_stream);
			em_convert_byte_stream_to_option(local_option_stream, &sending_option);
		}
	}

	if(emdaemon_send_mail(mailbox, mail_id, &sending_option, (unsigned*)&handle, &err)) {
		err = EMF_ERROR_NONE;
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter result failed ");
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter result failed ");
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}
	else {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}

FINISH_OFF:	
	EM_SAFE_FREE(local_stream);
	EM_SAFE_FREE(local_option_stream);
	EM_SAFE_FREE(mailbox);
	EM_SAFE_FREE(mailbox_name);
	
	EM_DEBUG_FUNC_END();
}

void stb_get_mailbox_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	int counter = 0;
	char* local_stream = NULL;
	int account_id;
	emf_mailbox_t* mailbox_list = NULL;
	int count = 0;
	int size = 0;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);

	if(emdaemon_get_mailbox_list(account_id, &mailbox_list, &count, &err))
		EM_DEBUG_LOG("emdaemon_get_mailbox_list - success");
		
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	if(EMF_ERROR_NONE == err) {
		EM_DEBUG_LOG("Count [%d]", count);
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &count, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

		for(counter=0; counter<count; counter++) {
			EM_DEBUG_LOG("Name - %s", mailbox_list[counter].name);

			local_stream = em_convert_mailbox_to_byte_stream(mailbox_list+counter, &size);

			EM_NULL_CHECK_FOR_VOID(local_stream);

			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
				EM_DEBUG_EXCEPTION("Add  Param mailbox failed  ");

			EM_SAFE_FREE(local_stream);
			size = 0;
		}
	}

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_delete_all_mails(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	char* local_stream = NULL;
	emf_mailbox_t *mailbox = NULL;
	int with_server = 0;
	int err = EMF_ERROR_NONE;

	/* Mailbox */
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	{
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
			mailbox = (emf_mailbox_t*)em_malloc(sizeof(emf_mailbox_t));
			em_convert_byte_stream_to_mailbox(local_stream, mailbox);
		}
	}

	/* with_server */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &with_server);

	emdaemon_delete_mail_all(mailbox, with_server, NULL, &err);
	
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_SAFE_FREE(local_stream);
	EM_SAFE_FREE(mailbox);

	EM_DEBUG_FUNC_END();
}

void stb_delete_mail(HIPC_API a_hAPI)
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
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
			/* Memory is not yet allocated for mailbox */
			mailbox = (emf_mailbox_t*)em_malloc(sizeof(emf_mailbox_t));
			em_convert_byte_stream_to_mailbox(local_stream, mailbox);
		}
		EM_DEBUG_LOG("account_id [%d]", mailbox->account_id);
		EM_DEBUG_LOG("mailbox name - %s", mailbox->name);

		/* Number of mail_ids */
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &num);
		EM_DEBUG_LOG("number of mails [%d]", num);

		int mail_ids[num];	
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, num * sizeof(int), mail_ids);
		for(counter = 0; counter < num; counter++)
			EM_DEBUG_LOG("mail_ids [%d]", mail_ids[counter]);
		
		/* with_server */
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 3, sizeof(int), &with_server);
		EM_DEBUG_LOG("with_Server [%d]", with_server);

		emdaemon_delete_mail(mailbox->account_id, mailbox, mail_ids, num, with_server, NULL, &err);
	}
	else
		err = EMF_ERROR_IPC_CRASH;
	
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	EM_SAFE_FREE(local_stream);
	EM_SAFE_FREE(mailbox);	

	EM_DEBUG_FUNC_END();	
}
void stb_clear_mail_data (HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	
	if(emdaemon_clear_all_mail_data(&err)) {
		EM_DEBUG_LOG(">>> stb_clear_mail_data Success [ %d]  >> ", err);
	}
	
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed : err");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_add_rule(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_rule_stream = NULL;
	emf_rule_t rule = { 0 };

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	{
		local_rule_stream = (char*)em_malloc(buffer_size + 1);
		EM_NULL_CHECK_FOR_VOID(local_rule_stream);
		if(local_rule_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_rule_stream);
			em_convert_byte_stream_to_rule(local_rule_stream, &rule);
			EM_DEBUG_LOG("account ID  [%d]", rule.account_id);

			if(emdaemon_add_filter(&rule, &err))
				err = EMF_ERROR_NONE;
			
			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
			if (!emipc_execute_stub_api(a_hAPI))
				EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	
		}
	}	

	EM_SAFE_FREE(local_rule_stream);

	EM_DEBUG_FUNC_END();
}

void stb_get_rule(HIPC_API a_hAPI)
{
	int err = EMF_ERROR_NONE;
	int filter_id = 0;
	emf_rule_t* rule = NULL;
	int size =0;
	char* local_rule_stream = NULL;

	EM_DEBUG_LOG(" filter_id");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &filter_id);

	if(emdaemon_get_filter(filter_id, &rule, &err)) {
		err = EMF_ERROR_NONE;
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
		
		/* Rule */
		local_rule_stream = em_convert_rule_to_byte_stream(rule, &size);

		EM_NULL_CHECK_FOR_VOID(local_rule_stream);

		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, local_rule_stream, size))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");

		if (!emipc_execute_stub_api(a_hAPI)) {
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
			EM_SAFE_FREE( local_rule_stream );
			return;
		}
	}
	else {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

		if (!emipc_execute_stub_api(a_hAPI)) {
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
			EM_SAFE_FREE( local_rule_stream );			
			return;
		}
	}
	EM_SAFE_FREE( local_rule_stream );	
	EM_DEBUG_FUNC_END();
}

void stb_get_rule_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	/* int buffer_size = 0; */
	int err = EMF_ERROR_NONE;
	int counter = 0;
	char* local_stream = NULL;
	int count = 0;
	int size = 0;

	emf_rule_t* filtering_list = NULL;

	if(emdaemon_get_filter_list(&filtering_list, &count, &err)) {
		EM_DEBUG_LOG("emdaemon_get_filter_list - success");
		err = EMF_ERROR_NONE;
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

		EM_DEBUG_LOG("Count [%d]", count);
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &count, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

		for(counter=0; counter<count; counter++) {
			EM_DEBUG_LOG("Value - %s", filtering_list[counter].value);

			local_stream = em_convert_rule_to_byte_stream(filtering_list+counter, &size);

			EM_NULL_CHECK_FOR_VOID(local_stream);

			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");

			EM_SAFE_FREE(local_stream);
			size = 0;
		}

		if (!emipc_execute_stub_api(a_hAPI)) {
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
			return;
		}
	}

	else {
		EM_DEBUG_EXCEPTION("emdaemon_get_filter_list - failed");
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
		
		if (!emipc_execute_stub_api(a_hAPI)) {
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
			return;
		}
	}
	EM_DEBUG_FUNC_END();
}

void stb_find_rule(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* local_rule_stream = NULL;
	emf_rule_t rule = { 0 };


	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_rule_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_rule_stream);
		if(local_rule_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_rule_stream);
			em_convert_byte_stream_to_rule(local_rule_stream, &rule);
			EM_SAFE_FREE(local_rule_stream);
			EM_DEBUG_LOG("account ID  [%d]", rule.account_id);
		
			if(emdaemon_find_filter(&rule, &err))
				err = EMF_ERROR_NONE;
			
			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
			if (!emipc_execute_stub_api(a_hAPI))
				EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	
		}
	}	
	EM_DEBUG_FUNC_END();
}

void stb_update_change(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int filter_id = 0;
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* rule_stream = NULL;
	emf_rule_t  *rule = NULL;

	/* filter_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &filter_id);
	
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	
	if(buffer_size > 0)  {
		rule_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(rule_stream);
		if(rule_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, buffer_size, rule_stream);
			rule = (emf_rule_t*)em_malloc(sizeof(emf_rule_t));

			if(NULL == rule)			 {
				EM_SAFE_FREE(rule_stream);
				return;
			}
			
			em_convert_byte_stream_to_rule(rule_stream, rule);
			EM_SAFE_FREE(rule_stream);
		}
	}

	if(emdaemon_update_filter(filter_id, rule, &err))
		err = EMF_ERROR_NONE;

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	EM_SAFE_FREE(rule);	
	EM_DEBUG_FUNC_END();	
}

void stb_move_all_mails(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMF_ERROR_NONE;
	char* src_mailbox_stream = NULL;	
	char* dst_mailbox_stream = NULL;
	emf_mailbox_t dst_mailbox;
	emf_mailbox_t src_mailbox;	

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("buffer_size [%d]", buffer_size);
	
	if(buffer_size > 0)	 {
		src_mailbox_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(src_mailbox_stream);
		if(src_mailbox_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, src_mailbox_stream);
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
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG(" size [%d]", buffer_size);
	
	if(buffer_size > 0)	 {
		dst_mailbox_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(dst_mailbox_stream);
		if(dst_mailbox_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, buffer_size, dst_mailbox_stream);
			em_convert_byte_stream_to_mailbox(dst_mailbox_stream, &dst_mailbox);
			EM_SAFE_FREE(dst_mailbox_stream);
			if (dst_mailbox.name)
				EM_DEBUG_LOG("Mailbox name - %s", dst_mailbox.name);
			else
				EM_DEBUG_EXCEPTION(">>>> Mailbox Information is NULL >> ");
		}
	}
	
	if(emdaemon_move_mail_all_mails(&src_mailbox, &dst_mailbox, &err))
		EM_DEBUG_LOG("emdaemon_move_mail_all_mails:Success");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
		return;
	}
	
	if (!emipc_execute_stub_api(a_hAPI)) {
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
		return;
	}
	EM_DEBUG_FUNC_END();
}

void stb_set_flags_field(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	emf_flags_field_type field_type = 0;
	int account_id;
	int value = 0;
	int onserver = 0;
	int num = 0;
	int counter = 0;
	int *mail_ids = NULL;

	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id [%d]", account_id);

	/* Number of mail_ids */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &num);
	EM_DEBUG_LOG("number of mails [%d]", num);
	
	/* mail_id */
	mail_ids = em_malloc(sizeof(int) * num);
	
	if(!mail_ids) {
		EM_DEBUG_EXCEPTION("em_malloc failed ");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, num * sizeof(int), mail_ids);
	
	for(counter=0; counter < num; counter++)
		EM_DEBUG_LOG("mailids [%d]", mail_ids[counter]);

	/* field type */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 3, sizeof(int), &field_type);
	EM_DEBUG_LOG("field_type [%d]", field_type);

	/* value */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 4, sizeof(int), &value);
	EM_DEBUG_LOG("value [%d]", value);
	
	/*  on server */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 5, sizeof(int), &onserver);
	EM_DEBUG_LOG("onserver [%d]", onserver);

	if(emdaemon_set_flags_field(account_id, mail_ids, num, field_type, value, onserver, &err))	
		EM_DEBUG_LOG("emdaemon_set_flags_field - success");

FINISH_OFF:

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
		
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
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
	emf_mail_data_t *result_mail_data = NULL;
	emf_attachment_data_t *result_attachment_data = NULL;
	emf_meeting_request_t *result_meeting_request = NULL;
	
	EM_DEBUG_LOG("emf_mail_data_t");
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, parameter_index);

	if(buffer_size > 0)	 {
		mail_data_stream = (char*)em_malloc(buffer_size + 1);
		if(!mail_data_stream) {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, parameter_index++, buffer_size, mail_data_stream);
		em_convert_byte_stream_to_mail_data(mail_data_stream, &result_mail_data, &result_mail_data_count);
		if(!result_mail_data) {
			EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_mail_data failed");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}
	}

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, parameter_index);
	EM_DEBUG_LOG("emf_attachment_data_t buffer_size[%d]", buffer_size);

	if(buffer_size > 0)	 {
		attachment_data_list_stream = (char*)em_malloc(buffer_size + 1);
		if(!attachment_data_list_stream) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		emipc_get_parameter(a_hAPI, ePARAMETER_IN, parameter_index++, buffer_size, attachment_data_list_stream);
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
		buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, parameter_index);

		if(buffer_size > 0) {
			meeting_request_stream = (char*)em_malloc(buffer_size + 1);
			if(!meeting_request_stream) {
				EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_mail_data failed");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, parameter_index++, buffer_size, meeting_request_stream);
			result_meeting_request = (emf_meeting_request_t*)em_malloc(sizeof(emf_meeting_request_t));
			if(result_meeting_request)
				em_convert_byte_stream_to_meeting_req(meeting_request_stream, result_meeting_request);
		}
	}

	EM_DEBUG_LOG("sync_server");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, parameter_index++, sizeof(int), &sync_server);

	if( (err = emdaemon_add_mail(result_mail_data, result_attachment_data, result_attachment_data_count, result_meeting_request, sync_server)) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emdaemon_add_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	local_result = 1;
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &(result_mail_data->mail_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &(result_mail_data->thread_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

FINISH_OFF:
	if ( local_result == 0 ) {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
	}

	EM_SAFE_FREE(mail_data_stream);
	EM_SAFE_FREE(attachment_data_list_stream);
	EM_SAFE_FREE(meeting_request_stream); 

	if(result_mail_data)
		emcore_free_mail_data(&result_mail_data, 1, NULL);

	if(result_attachment_data)
		emcore_free_attachment_data(&result_attachment_data, result_attachment_data_count, NULL);

	if(result_meeting_request)
		emstorage_free_meeting_request(&result_meeting_request, 1, NULL);

	em_flush_memory();
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
	emf_mail_data_t *result_mail_data = NULL;
	emf_attachment_data_t *result_attachment_data = NULL;
	emf_meeting_request_t *result_meeting_request = NULL;

	EM_DEBUG_LOG("emf_mail_data_t");
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, parameter_index);

	if(buffer_size > 0)	 {
		mail_data_stream = (char*)em_malloc(buffer_size + 1);
		if(!mail_data_stream) {
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, parameter_index++, buffer_size,mail_data_stream);
		em_convert_byte_stream_to_mail_data(mail_data_stream, &result_mail_data, &result_mail_data_count);
		if(!result_mail_data) {
			EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_mail_data failed");
			err = EMF_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}
	}

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, parameter_index);
	EM_DEBUG_LOG("emf_attachment_data_t buffer_size[%d]", buffer_size);

	if(buffer_size > 0)	 {
		attachment_data_list_stream = (char*)em_malloc(buffer_size + 1);
		if(!attachment_data_list_stream) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMF_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		emipc_get_parameter(a_hAPI, ePARAMETER_IN, parameter_index++, buffer_size, attachment_data_list_stream);
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
		buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, parameter_index);

		if(buffer_size > 0) {
			meeting_request_stream = (char*)em_malloc(buffer_size + 1);
			if(!meeting_request_stream) {
				EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_mail_data failed");
				err = EMF_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, parameter_index++, buffer_size, meeting_request_stream);
			result_meeting_request = (emf_meeting_request_t*)em_malloc(sizeof(emf_meeting_request_t));
			if(result_meeting_request)
				em_convert_byte_stream_to_meeting_req(meeting_request_stream, result_meeting_request);
		}
	}

	EM_DEBUG_LOG("sync_server");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, parameter_index++, sizeof(int), &sync_server);

	if( (err = emdaemon_update_mail(result_mail_data, result_attachment_data, result_attachment_data_count, result_meeting_request, sync_server)) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emdaemon_update_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	local_result = 1;
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &(result_mail_data->mail_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &(result_mail_data->thread_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

FINISH_OFF:
	if ( local_result == 0 ) {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
	}

	EM_SAFE_FREE(mail_data_stream);
	EM_SAFE_FREE(attachment_data_list_stream);
	EM_SAFE_FREE(meeting_request_stream);

	if(result_mail_data)
		emcore_free_mail_data(&result_mail_data, 1, NULL);

	if(result_attachment_data)
		emcore_free_attachment_data(&result_attachment_data, result_attachment_data_count, NULL);

	if(result_meeting_request)
		emstorage_free_meeting_request(&result_meeting_request, 1, NULL);

	em_flush_memory();
	EM_DEBUG_FUNC_END();
}


void stb_move_thread_to_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0, thread_id = 0, move_always_flag = 0;
	int err = EMF_ERROR_NONE;
	char *target_mailbox_name = NULL;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &thread_id);
	EM_DEBUG_LOG("thread_id [%d]", thread_id);
	
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("mailbox stream size [%d]", buffer_size);
	
	if(buffer_size > 0) {
		target_mailbox_name = (char*)em_malloc(buffer_size + 1);
		EM_NULL_CHECK_FOR_VOID(target_mailbox_name);
		if(target_mailbox_name) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, buffer_size, target_mailbox_name);
			/* Convert Byte stream to mailbox structure */
			if (target_mailbox_name)
				EM_DEBUG_LOG("Mailbox name - %s", target_mailbox_name);
			else
				EM_DEBUG_EXCEPTION("Mailbox name is NULL");
		}
	}

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &move_always_flag);
	EM_DEBUG_LOG("move_always_flag [%d]", move_always_flag);
	
	if(emdaemon_move_mail_thread_to_mailbox(thread_id, target_mailbox_name, move_always_flag, &err))
		EM_DEBUG_LOG("emdaemon_move_mail_thread_to_mailbox success");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter fail");
		EM_SAFE_FREE(target_mailbox_name);
		return;
	}

	if (!emipc_execute_stub_api(a_hAPI)) {
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api fail");
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

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &thread_id);
	EM_DEBUG_LOG("thread_id [%d]", thread_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &delete_always_flag);
	EM_DEBUG_LOG("delete_always_flag [%d]", delete_always_flag);
	
	if(emdaemon_delete_mail_thread(thread_id, delete_always_flag, &handle, &err))
		EM_DEBUG_LOG("emdaemon_delete_mail_thread success");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter fail");
		return;
	}

	if (!emipc_execute_stub_api(a_hAPI)) {
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api fail");
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

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &thread_id);
	EM_DEBUG_LOG("thread_id [%d]", thread_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &seen_flag);
	EM_DEBUG_LOG("seen_flag [%d]", seen_flag);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &on_server);
	EM_DEBUG_LOG("on_server [%d]", on_server);
	
	if(emdaemon_modify_seen_flag_of_thread(thread_id, seen_flag, on_server, &handle, &err))
		EM_DEBUG_LOG("emdaemon_modify_seen_flag_of_thread success");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter fail");
		return;
	}

	if (!emipc_execute_stub_api(a_hAPI)) {
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api fail");
		return;
	}
	EM_DEBUG_FUNC_END();
}


void stb_move_mail(HIPC_API a_hAPI)
{
	int err = EMF_ERROR_NONE;
	int buffer_size = 0, num = 0, counter = 0;
	char *local_stream = NULL;
	emf_mailbox_t mailbox;

	/* Number of mail_ids */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &num);
	EM_DEBUG_LOG("number of mails [%d]", num);

	/* mail_id */
	int mail_ids[num];	
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, num * sizeof(int), mail_ids);
	
	for(counter = 0; counter < num; counter++)
		EM_DEBUG_LOG("mailids [%d]", mail_ids[counter]);
	
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 2);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	
	if(buffer_size > 0) {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, buffer_size, local_stream);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
			
			if (mailbox.name)
				EM_DEBUG_LOG("mailbox.name [%s]", mailbox.name);
			else
				EM_DEBUG_EXCEPTION("mailbox.name is NULL");
			
			if(emdaemon_move_mail(mail_ids, num, &mailbox, EMF_MOVED_BY_COMMAND, 0, &err))
				EM_DEBUG_LOG("emdaemon_move_mail success");
		}
	}
	
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter fail");
	
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api fail");
	EM_DEBUG_FUNC_END();
}

void stb_delete_rule(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int filter_id = 0;
	int err = EMF_ERROR_NONE;
	
	/* filter_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &filter_id);
	
	if(emdaemon_delete_filter(filter_id, &err))
		err = EMF_ERROR_NONE;

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
				
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_add_attachment(HIPC_API a_hAPI)
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
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	/* mail_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mail_id);

	/* attachment */
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 2);

	if(buffer_size > 0)	 {
		attachment_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(attachment_stream);
		if(attachment_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, buffer_size, attachment_stream);
			em_convert_byte_stream_to_attachment_info(attachment_stream, 1, &attachment);
			EM_SAFE_FREE(attachment_stream);
		}
	}

	emdaemon_add_attachment(&mailbox, mail_id, attachment, &err);
	
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
	if(EMF_ERROR_NONE == err) {
		EM_DEBUG_LOG("emdaemon_add_attachment -Success");
		
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &(attachment->attachment_id), sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter attachment_id failed ");
	}
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_SAFE_FREE(attachment);		
	EM_DEBUG_FUNC_END();
}

void stb_get_attachment(HIPC_API a_hAPI)
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
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	/* mail_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mail_id);

	/* attachment_no */
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 2);
	if(buffer_size > 0)	 {
		attachment_no = (char*)em_malloc(buffer_size+1);
		if(attachment_no) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, buffer_size, attachment_no);
		}
	}
	emdaemon_get_attachment(&mailbox, mail_id, attachment_no, &attachment, &err);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
	if(EMF_ERROR_NONE == err) {
		EM_DEBUG_LOG("emdaemon_get_attachment - Success");
		/* attachment */
		attachment_stream = em_convert_attachment_info_to_byte_stream(attachment, &size);

		EM_NULL_CHECK_FOR_VOID(attachment_stream);
		
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, attachment_stream, size))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
	}
		if (!emipc_execute_stub_api(a_hAPI)) {
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
			EM_SAFE_FREE(attachment_no);		
			EM_SAFE_FREE(attachment_stream); 
			return;
		}

	EM_SAFE_FREE(attachment_no);		
	EM_SAFE_FREE(attachment_stream); 
	EM_DEBUG_FUNC_END();
}

void stb_get_imap_mailbox_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;
	int account_id = 0;
	unsigned int handle = 0;  

	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);

	if(emdaemon_get_imap_mailbox_list(account_id, "", &handle, &err))
		err = EMF_ERROR_NONE;
		
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_LOG("ipcAPI_AddParameter local_result failed ");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_LOG("ipcAPI_AddParameter local_result failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_LOG("emipc_execute_stub_api failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_delete_attachment(HIPC_API a_hAPI)
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
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	/* mail_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mail_id);

	/* attachment_no */
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 2);
	if(buffer_size > 0)	 {
		attachment_no = (char*)em_malloc(buffer_size+1);
		if(attachment_no) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, buffer_size, attachment_no);
		}
	}

	emdaemon_delete_mail_attachment(&mailbox, mail_id, attachment_no, &err);
	
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

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
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_stream);
		if(local_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_stream);
			em_convert_byte_stream_to_mailbox(local_stream, &mailbox);
			EM_SAFE_FREE(local_stream);
		}
	}

	EM_DEBUG_LOG("mail_id");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mail_id);

	/* nth */
	EM_DEBUG_LOG("nth");
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 2);
	if(buffer_size > 0)	 {
		nth = (char*)em_malloc(buffer_size+1);
		if(nth) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, buffer_size, nth);
		}
	}

	if(emdaemon_download_attachment(&mailbox, mail_id, nth, &handle, &err)) {
		err = EMF_ERROR_NONE;

		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
		EM_DEBUG_LOG(">>>>>>>>>> HANDLE = %d", handle);
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter handle failed ");		
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}
	else {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
		/* Download handle - 17-Apr-09 */
		EM_DEBUG_LOG(">>>>>>>>>> HANDLE = %d", handle);
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter handle failed ");		
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}

	EM_SAFE_FREE(nth);		
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
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	
	/* Sending Option */
	EM_DEBUG_LOG("Sending option");
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);

	if(buffer_size > 0)  {
		local_option_stream = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(local_option_stream);
		if(local_option_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, buffer_size, local_option_stream);
			em_convert_byte_stream_to_option(local_option_stream, &sending_option);
			/* EM_SAFE_FREE(local_option_stream); */
		}
	}

	EM_DEBUG_LOG("calling emdaemon_send_mail_saved");
	if(emdaemon_send_mail_saved(account_id, &sending_option, NULL, &err))
		err = EMF_ERROR_NONE;
		
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter result failed ");
		
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	EM_DEBUG_FUNC_END();
}	

void stb_add_read_receipt(HIPC_API a_hAPI){
	EM_DEBUG_FUNC_BEGIN();
	int read_mail_id = 0;
	int receipt_mail_id = 0;
	int err = EMF_ERROR_NONE;

	/* read_mail_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &read_mail_id);
	EM_DEBUG_LOG("read_mail_id [%d]", read_mail_id);

	if( (err = emcore_add_read_receipt(read_mail_id, &receipt_mail_id)) != EMF_ERROR_NONE )
		EM_DEBUG_EXCEPTION("emcore_add_read_receipt failed [%d]", err);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	if (err == EMF_ERROR_NONE) {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &receipt_mail_id, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
	}

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	EM_DEBUG_FUNC_END("err [%d]", err);
}

void stb_retry_sending_mail(HIPC_API a_hAPI)
{

	EM_DEBUG_FUNC_BEGIN();
	int mail_id = 0;
	int timeout_in_sec = 0;
	int err = EMF_ERROR_NONE;

	/* Mail_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mail_id);
	EM_DEBUG_LOG("mail_id [%d]", mail_id);

	/* timeout_in_sec */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &timeout_in_sec);
	EM_DEBUG_LOG("timeout_in_sec [%d]", timeout_in_sec);

	if(emdaemon_send_mail_retry(mail_id, timeout_in_sec,&err))
		EM_DEBUG_LOG("emdaemon_get_mailbox_list - success");	

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
				
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_get_event_queue_status(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int on_sending = 0;
	int on_receiving = 0;
	
	/*get the network status */
	emdaemon_get_event_queue_status(&on_sending, &on_receiving);
	
	/* on_sending */
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &on_sending, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	/* on_receving */
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &on_receiving, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	EM_DEBUG_LOG("stb_get_event_queue_status - Before Execute API");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	EM_DEBUG_FUNC_END();
}

void stb_cancel_job(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id = 0;
	int handle = 0;
	int err = EMF_ERROR_NONE;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id [%d]", account_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &handle);
	EM_DEBUG_LOG("handle [%d]", handle);	

	if(emdaemon_cancel_job(account_id, handle, &err))
		err = EMF_ERROR_NONE;

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
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

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &action);
	EM_DEBUG_LOG("action [%d]", action);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id [%d]", account_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &mail_id);
	EM_DEBUG_LOG("mail_id [%d]", mail_id);

	if(emdaemon_get_pending_job(action, account_id, mail_id, &status))
		err = EMF_ERROR_NONE;
	else
		err = EMF_ERROR_UNKNOWN;

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &status, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter status failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	EM_DEBUG_FUNC_END();
}

void stb_print_receiving_event_queue_via_debug_msg(HIPC_API a_hAPI)
{
	emf_event_t *event_queue = NULL;
	int event_active_queue = 0, err, i;

	emcore_get_receiving_event_queue(&event_queue, &event_active_queue, &err);	

	EM_DEBUG_LOG("======================================================================");
	EM_DEBUG_LOG("Event active index [%d]", event_active_queue);
	EM_DEBUG_LOG("======================================================================");
	for(i = 1; i < 32; i++)
		EM_DEBUG_LOG("event[%d] : type[%d], account_id[%d], arg[%d], status[%d]", i, event_queue[i].type, event_queue[i].account_id, event_queue[i].event_param_data_4, event_queue[i].status);
	EM_DEBUG_LOG("======================================================================");
	EM_DEBUG_LOG("======================================================================");
	
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
	EM_DEBUG_FUNC_END();
}

void stb_cancel_send_mail_job(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int mail_id = 0;
	int err = EMF_ERROR_NONE;
	int account_id = 0;

	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id [%d]", account_id);
	
	/* Mail_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mail_id);
	EM_DEBUG_LOG("mail_id [%d]", mail_id);

	if(emdaemon_cancel_sending_mail_job(account_id,mail_id,&err))
		EM_DEBUG_LOG("success");		

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
				
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
	EM_DEBUG_FUNC_END();
}

void stb_search_mail_on_server(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int                    i = 0;
	int                    err = EMF_ERROR_NONE;
	int                    account_id = 0;
	int                    buffer_size = 0;
	int                    search_filter_count = 0;
	char                  *mailbox_name = NULL;
	char                  *stream_for_search_filter_list = NULL;
	unsigned int           job_handle = 0;
	email_search_filter_t *search_filter_list = NULL;

	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id [%d]", account_id);

	/* mailbox_name */
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);
	if(buffer_size > 0)	 {
		mailbox_name = (char*)em_malloc(buffer_size + 1);
		EM_NULL_CHECK_FOR_VOID(mailbox_name);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, buffer_size, mailbox_name);
		EM_DEBUG_LOG("mailbox_name [%s]", mailbox_name);
	}

	/* search_filter_list */
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 2);

	if(buffer_size > 0)	 {
		stream_for_search_filter_list = (char*)em_malloc(buffer_size+1);
		EM_NULL_CHECK_FOR_VOID(stream_for_search_filter_list);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, buffer_size, stream_for_search_filter_list);
		em_convert_byte_stream_to_search_filter(stream_for_search_filter_list, &search_filter_list, &search_filter_count);
		EM_SAFE_FREE(stream_for_search_filter_list);
	}

	if( (err = emdaemon_search_mail_on_server(account_id ,mailbox_name, search_filter_list, search_filter_count, &job_handle)) == EMF_ERROR_NONE)
		EM_DEBUG_LOG("success");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &job_handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	if(search_filter_list) {
		for(i = 0; i < search_filter_count; i++) {
			switch(search_filter_list[i].search_filter_type) {
				case EMAIL_SEARCH_FILTER_TYPE_BCC              :
				case EMAIL_SEARCH_FILTER_TYPE_CC               :
				case EMAIL_SEARCH_FILTER_TYPE_FROM             :
				case EMAIL_SEARCH_FILTER_TYPE_KEYWORD          :
				case EMAIL_SEARCH_FILTER_TYPE_SUBJECT          :
				case EMAIL_SEARCH_FILTER_TYPE_TO               :
				case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       :
					EM_SAFE_FREE(search_filter_list[i].search_filter_key_value.string_type_key_value);
					break;
				default :
					break;
			}
		}
	}

	EM_SAFE_FREE(mailbox_name);
	EM_SAFE_FREE(search_filter_list);

	EM_DEBUG_FUNC_END();
}

void stb_create_account_with_validation(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int local_result = 0;
	unsigned int handle = 0;  
	char* local_account_stream = NULL;
	emf_account_t *account;
	int err = EMF_ERROR_NONE;

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);

	if(buffer_size > 0)	 {
		local_account_stream = (char*)em_malloc(buffer_size+1);
		if(local_account_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_account_stream);
			/* Convert account stream to structure */
			EM_DEBUG_LOG("Before Convert_ByteStream_To_Account");
			
			account = emcore_get_new_account_reference(); 
			em_convert_byte_stream_to_account(local_account_stream, account);
			EM_SAFE_FREE(local_account_stream);
		
			account->account_id = NEW_ACCOUNT_ID;

			if (account->account_name)
				EM_DEBUG_LOG("Account name - %s", account->account_name);
			if(account->email_addr)
				EM_DEBUG_LOG("Email Address - %s", account->email_addr);
		
			if(emdaemon_validate_account_and_create(account, &handle, &err)) {	
				
#ifdef __FEATURE_AUTO_POLLING__
				/*  start auto polling, if check_interval not zero */
				if(account->check_interval > 0) {
					if(!emdaemon_add_polling_alarm( account->account_id,account->check_interval))
						EM_DEBUG_EXCEPTION("emdaemon_add_polling_alarm[NOTI_ACCOUNT_ADD] : start auto poll failed >>> ");
				}
#endif /*  __FEATURE_AUTO_POLLING__ */
				/*  add account details to contact DB  */
				/*  emdaemon_insert_accountinfo_to_contact(account); */

				local_result = 1;
				if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
					EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");		
				if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
					EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
				if (!emipc_execute_stub_api(a_hAPI))
					EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
				}
			else {
				EM_DEBUG_EXCEPTION("emdaemon_validate_account_and_create fail ");
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
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed : local_result ");
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed : err");
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}
	EM_DEBUG_FUNC_END();
}

void stb_backup_account(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();

#ifdef __FEATURE_BACKUP_ACCOUNT__	
	char *file_path = NULL;
	int local_result = 0, err_code = 0, handle = 0, file_path_length = 0;
	
	/* file_path_length */
	file_path_length = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	if(file_path_length > 0) {
		EM_DEBUG_LOG("file_path_length [%d]", file_path_length);
		file_path = em_malloc(file_path_length + 1);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, file_path_length, file_path);
		EM_DEBUG_LOG("file_path [%s]", file_path);
		local_result = emcore_backup_accounts((const char*)file_path, &err_code);
	}

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_LOG("emipc_add_parameter failed ");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err_code, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
				
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter result failed ");
				
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_SAFE_FREE(file_path);
#endif /*  __FEATURE_BACKUP_ACCOUNT__ */
	EM_DEBUG_FUNC_END();
}

void stb_restore_account(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
#ifdef __FEATURE_BACKUP_ACCOUNT__
	char *file_path = NULL;
	int local_result = 0, err_code = 0, handle = 0, file_path_length = 0;
	
	/* file_path_length */
	file_path_length = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	if(file_path_length > 0)  {
		EM_DEBUG_LOG("file_path_length [%d]", file_path_length);
		file_path = em_malloc(file_path_length + 1);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, file_path_length, file_path);
		EM_DEBUG_LOG("file_path [%s]", file_path);
		local_result = emcore_restore_accounts((const char*)file_path, &err_code);
	}

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_LOG("emipc_add_parameter failed ");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err_code, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
				
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter result failed ");
				
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_SAFE_FREE(file_path);
#endif /*  __FEATURE_BACKUP_ACCOUNT__ */
	EM_DEBUG_FUNC_END();
}


void stb_get_password_length(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int account_id = 0;
	int local_result = 0;
	int err_code = 0;
	int password_length = 0;
	
	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	local_result = emstorage_get_password_length_of_account(account_id, &password_length,&err_code);

	EM_DEBUG_LOG("password_length [%d]", password_length);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
		EM_DEBUG_LOG("emipc_add_parameter failed ");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err_code, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
				
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &password_length, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter result failed ");
				
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	EM_DEBUG_FUNC_END();
}


void stb_ping_service(HIPC_API a_hAPI)
{
	int err = EMF_ERROR_NONE;
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");	
	EM_DEBUG_FUNC_END();
}

void stb_update_notification_bar_for_unread_mail(HIPC_API a_hAPI)
{
	int err = EMF_ERROR_NONE, account_id;

	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id [%d]", account_id);

	if(!emcore_finalize_sync(account_id, &err)) {
		EM_DEBUG_EXCEPTION("emcore_finalize_sync failed [%d]", err);
	}

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");	
	EM_DEBUG_FUNC_END();
}


void stb_API_mapper(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int nAPIID = emipc_get_api_id(a_hAPI);
	
	switch(nAPIID) {
		case _EMAIL_API_ADD_ACCOUNT:
			stb_create_account(a_hAPI);
			break;

		case _EMAIL_API_ADD_MAILBOX:
			stb_create_mailbox(a_hAPI);
			break;

		case _EMAIL_API_DELETE_MAILBOX:
			stb_delete_mailbox(a_hAPI);
			break;
		case _EMAIL_API_UPDATE_MAILBOX:
			stb_update_mailbox(a_hAPI);
			break;

		case _EMAIL_API_SET_MAIL_SLOT_SIZE:
			stb_set_mail_slot_size_of_mailbox(a_hAPI);
			break;

		case _EMAIL_API_SEND_MAIL:
			stb_send_mail(a_hAPI);
			break;
		
		case _EMAIL_API_GET_MAILBOX_COUNT:
			stb_get_mail_count_of_mailbox(a_hAPI);
			break;

		case _EMAIL_API_GET_MAILBOX_LIST:
			stb_get_mailbox_list(a_hAPI);
			break;
			
		case _EMAIL_API_SYNC_HEADER:
			stb_sync_header(a_hAPI);
			break;
			
		case _EMAIL_API_DOWNLOAD_BODY:
			stb_download_body(a_hAPI);
			break;

		case _EMAIL_API_CLEAR_DATA:
			stb_clear_mail_data (a_hAPI);
			break;
			
		case _EMAIL_API_DELETE_ALL_MAIL:
			stb_delete_all_mails(a_hAPI);
			break;
			
		case _EMAIL_API_DELETE_MAIL:
			stb_delete_mail(a_hAPI);
			break;

		case _EMAIL_API_MODIFY_MAIL_FLAG:
			stb_modify_mail_flag(a_hAPI);
			break;

		case _EMAIL_API_MODIFY_MAIL_EXTRA_FLAG:
			stb_modify_mail_extra_flag(a_hAPI);
			break;		

 		case _EMAIL_API_ADD_RULE:
			stb_add_rule(a_hAPI);
			break;

		case _EMAIL_API_GET_RULE:
			stb_get_rule(a_hAPI);
			break;

		case _EMAIL_API_GET_RULE_LIST:
			stb_get_rule_list(a_hAPI);
			break;

		case _EMAIL_API_FIND_RULE:
			stb_find_rule(a_hAPI);
			break;

		case _EMAIL_API_UPDATE_RULE:
			stb_update_change(a_hAPI);
			break;
			
		case _EMAIL_API_DELETE_RULE:
			stb_delete_rule(a_hAPI);
			break;

		case _EMAIL_API_MOVE_MAIL:
			stb_move_mail(a_hAPI);
			break;

		case _EMAIL_API_MOVE_ALL_MAIL:
			stb_move_all_mails(a_hAPI);
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
			stb_delete_account(a_hAPI);
			break;

		case _EMAIL_API_UPDATE_ACCOUNT:
			stb_update_account(a_hAPI);
			break;

		case _EMAIL_API_ADD_ATTACHMENT:
			stb_add_attachment(a_hAPI);
			break;

		case _EMAIL_API_GET_IMAP_MAILBOX_LIST:
			stb_get_imap_mailbox_list(a_hAPI);
			break;
			
		case _EMAIL_API_GET_ATTACHMENT:
			stb_get_attachment(a_hAPI);
			break;

		case _EMAIL_API_DELETE_ATTACHMENT:
			stb_delete_attachment(a_hAPI);
			break;

		case _EMAIL_API_DOWNLOAD_ATTACHMENT:
			stb_download_attachment(a_hAPI);
			break;

		case _EMAIL_API_GET_ACCOUNT_LIST:
			stb_get_account_list(a_hAPI);
			break;

		case _EMAIL_API_SEND_SAVED:
			stb_mail_send_saved(a_hAPI);
			break;

		case _EMAIL_API_ADD_READ_RECEIPT:
			stb_add_read_receipt(a_hAPI);
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
			stb_retry_sending_mail(a_hAPI);
			break;

		case _EMAIL_API_VALIDATE_ACCOUNT :
			stb_validate_account(a_hAPI);
			break;

		case _EMAIL_API_SEND_MAIL_CANCEL_JOB :
			stb_cancel_send_mail_job(a_hAPI);
			break;

		case _EMAIL_API_SEARCH_MAIL_ON_SERVER :
			stb_search_mail_on_server(a_hAPI);
			break;

		case _EMAIL_API_ADD_ACCOUNT_WITH_VALIDATION :
			stb_create_account_with_validation(a_hAPI);
			break;

		case _EMAIL_API_BACKUP_ACCOUNTS:
			stb_backup_account(a_hAPI);
			break;

		case _EMAIL_API_RESTORE_ACCOUNTS:
			stb_restore_account(a_hAPI);
			break;
		
		case _EMAIL_API_GET_PASSWORD_LENGTH_OF_ACCOUNT:
			stb_get_password_length(a_hAPI);
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
	emstorage_db_close(NULL);
	EM_DEBUG_FUNC_END();
}

void hibernation_leave_callback()
{
	EM_DEBUG_FUNC_BEGIN();
	emstorage_db_open(NULL);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int main(int argc, char *argv[])
{
	/* Do the email-service Initialization 
       1. Create all DB tables and load the email engine */
	EM_DEBUG_LOG("Email service begin");
	int err = 0, ret;

	signal(SIGPIPE, SIG_IGN); /*  to ignore signal 13(SIGPIPE) */

	emdaemon_initialize(&err);

#ifdef USE_OMA_EMN
	EM_DEBUG_LOG("emdaemon_initialize_emn Start");
	emdaemon_initialize_emn();
#endif

	EM_DEBUG_LOG("ipcEmailStub_Initialize Start");
	
	ret = emipc_initialize_stub(stb_API_mapper);

	if(ret == true)
		EM_DEBUG_LOG("ipcEmailStub_Initialize Success");
	else
		EM_DEBUG_EXCEPTION("ipcEmailStub_Initialize failed");

	/* Start auto polling */
#ifdef __FEATURE_AUTO_POLLING__
	emdaemon_start_auto_polling(&err);
#endif

	int fd_HibernationNoti;

    fd_HibernationNoti = heynoti_init();

    if(fd_HibernationNoti == -1)
    	EM_DEBUG_EXCEPTION("heynoti_init failed");
    else {
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

