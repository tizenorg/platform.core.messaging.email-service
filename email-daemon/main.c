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


/* Email service Main .c */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <glib.h>
#include <glib-object.h>
#include <sys/stat.h>
#include <sys/types.h>
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
#include "email-core-smime.h"
#include "email-core-cert.h"
#include "email-core-task-manager.h"
#include "email-core-signal.h"
#include "email-storage.h"

void stb_create_account(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int local_result = 0;
	char* local_account_stream = NULL;
	email_account_t account;
	int err = EMAIL_ERROR_NONE;

	/* Initialize the email_account_t */
	memset(&account, 0x00, sizeof(email_account_t));

	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	if(buffer_size <= 0)	{
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	local_account_stream = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 0);
	/* Convert account stream to structure */
	em_convert_byte_stream_to_account(local_account_stream, buffer_size, &account);


	EM_DEBUG_LOG("Account name - %s", account.account_name);
	EM_DEBUG_LOG("Email Address - %s", account.user_email_address);

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

	/* pass result to app */
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
	int err = EMAIL_ERROR_NONE;

	/* account_id */
	account_id = *((int*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 0));
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
	email_account_t new_account_info = {0};
	email_account_t old_account_info = {0};
	int err = EMAIL_ERROR_NONE;
	int handle = 0;

	account_id = *((int*)emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 0));

	/* get account structure from stream */
	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	local_account_stream = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 1);
	em_convert_byte_stream_to_account(local_account_stream, buffer_size, &new_account_info);

	/*get validation flag */
	with_validation = *((int*)emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 2));


	if(!emdaemon_get_account(account_id, EMAIL_ACC_GET_OPT_FULL_DATA, &old_account_info, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_get_account failed ");
		goto FINISH_OFF;
	}


	if( EM_SAFE_STRLEN (new_account_info.incoming_server_password) == 0 ) {
		EM_SAFE_FREE(new_account_info.incoming_server_password); /* be allocated but has zero length */
		EM_DEBUG_LOG("old_account_info->incoming_server_password [%s]", old_account_info.incoming_server_password);
		new_account_info.incoming_server_password = EM_SAFE_STRDUP(old_account_info.incoming_server_password);
		if(new_account_info.incoming_server_password == NULL) {
				EM_DEBUG_EXCEPTION("allocation for new_account_info->password failed");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
		}


	if( EM_SAFE_STRLEN (new_account_info.outgoing_server_password) == 0 ) {
		EM_SAFE_FREE(new_account_info.outgoing_server_password);
		if(old_account_info.outgoing_server_password) {
			new_account_info.outgoing_server_password = strdup (old_account_info.outgoing_server_password);
			if(new_account_info.outgoing_server_password == NULL) {
				EM_DEBUG_EXCEPTION("allocation for new_account_info->outgoing_server_password failed");
				err = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}
		}
	}

	if(with_validation) {
		emdaemon_validate_account_and_update(account_id, &new_account_info, &handle, &err);
		local_result = 1;
	}
	else {
		if(emdaemon_update_account(account_id, &new_account_info, &err)) {
			emdaemon_update_accountinfo_to_contact(&old_account_info, &new_account_info);
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

	emcore_free_account(&old_account_info);

	emcore_free_account(&new_account_info);

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
	local_result = emdaemon_validate_account(account_id, &handle,&err_code);

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


void stb_get_account_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();

	int local_result = 0;
	int i= 0;
	char* local_stream = NULL;
	email_account_t* account_list;
	int count;
	int size = 0;
	int err = EMAIL_ERROR_NONE;

	if(emdaemon_get_account_list(&account_list, &count, &err)) {
		EM_DEBUG_LOG("emdaemon_get_account_list success");
		local_result = 1;
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");

		EM_DEBUG_LOG("Count [%d]", count);
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &count, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter count failed ");

		for(i=0; i<count; i++) {
			EM_DEBUG_LOG("Name - %s", account_list[i].account_name);

			local_stream = em_convert_account_to_byte_stream(account_list+i, &size);

			if (!local_stream) {
				EM_DEBUG_EXCEPTION ("INVALID PARAM: local_stream NULL ");
				emcore_free_account_list(&account_list, count, NULL);
				return;
			}

			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
				EM_DEBUG_EXCEPTION("Add  Param mailbox failed  ");

			size = 0;
		 	EM_SAFE_FREE(local_stream);
		}

		emcore_free_account_list(&account_list, count, NULL);
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

/* sowmya.kr, 10-May-2010, changes for API improvement */
void stb_sync_header(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int handle = -1;
	int account_id = 0, maibox_id = 0;

	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id [%d]", account_id);

	/* maibox_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &maibox_id);
	EM_DEBUG_LOG("maibox_id [%d]", maibox_id);

	if(emdaemon_sync_header(account_id, maibox_id, &handle, &err)) {
		EM_DEBUG_LOG("emdaemon_sync_header success ");
	}

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
	int err = EMAIL_ERROR_NONE;
	int mail_id = 0;
	int attachment_count = 0;
	int handle = 0;
	int account_id = 0;

	/* Account Id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);

	/* Mail Id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mail_id);

	/* Has Attachment */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &attachment_count);

	/*Download Body */
	if (!emdaemon_download_body(account_id, mail_id, 1, attachment_count, &handle, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_download_body - failed");
		goto FINISH_OFF;
	}

	err = EMAIL_ERROR_NONE;

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
	int 		    buffer_size 			= 0;
	int 		    err = EMAIL_ERROR_NONE;
	char 		   *local_stream  = NULL;
	int 		    on_server 		= 0;
	email_mailbox_t mailbox = {0};
	int             handle = 0; /* Added for cancelling mailbox creating  */

	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);

	if(buffer_size <= 0) {
		EM_DEBUG_EXCEPTION("buffer_size(%d) should be greater than 0", buffer_size);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_stream = emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 0);
	em_convert_byte_stream_to_mailbox(local_stream, buffer_size, &mailbox);
	EM_DEBUG_LOG("Mailbox name - %s", mailbox.mailbox_name);

	on_server = *((int*)emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 1));

	emdaemon_add_mailbox(&mailbox, on_server, &handle, &err);

FINISH_OFF:
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 1");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 2");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &mailbox.mailbox_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 3");

	if (!emipc_execute_stub_api(a_hAPI)) {
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
		return;
	}

	EM_DEBUG_FUNC_END();
}


void stb_delete_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int		 err = EMAIL_ERROR_NONE;
	int 	 on_server		= 0;
	int      handle = 0; /* Added for cancelling mailbox deleting */
	int      input_mailbox_id = 0;

	/* src_mailbox_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &input_mailbox_id);
	EM_DEBUG_LOG("input_mailbox_id [%d]", input_mailbox_id);

	if (input_mailbox_id > 0)
		EM_DEBUG_LOG("input_mailbox_id [%d]", input_mailbox_id);
	else
		EM_DEBUG_LOG("input_mailbox_id == 0");

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &on_server);

	if(emdaemon_delete_mailbox(input_mailbox_id, on_server, &handle, &err))
		err = EMAIL_ERROR_NONE;

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	EM_DEBUG_FUNC_END();
}

void stb_set_mailbox_type(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int	err = EMAIL_ERROR_NONE;
	int mailbox_id = 0;
	int mailbox_type = 0;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mailbox_id);
	EM_DEBUG_LOG("mailbox_id[%d]", mailbox_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mailbox_type);
	EM_DEBUG_LOG("mailbox_type[%d]", mailbox_type);

	if( (err = emdaemon_set_mailbox_type(mailbox_id, mailbox_type)) != EMAIL_ERROR_NONE)
		err = EMAIL_ERROR_NONE;

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 1");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_DEBUG_FUNC_END();
}

void stb_set_local_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int	err = EMAIL_ERROR_NONE;
	int mailbox_id = 0;
	int is_local_mailbox = 0;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mailbox_id);
	EM_DEBUG_LOG("mailbox_id[%d]", mailbox_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &is_local_mailbox);
	EM_DEBUG_LOG("is_local_mailbox[%d]", is_local_mailbox);

	if( (err = emdaemon_set_local_mailbox(mailbox_id, is_local_mailbox)) != EMAIL_ERROR_NONE)
		err = EMAIL_ERROR_NONE;

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 1");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_DEBUG_FUNC_END();
}

void stb_set_mail_slot_size_of_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int	err    = EMAIL_ERROR_NONE;
	int handle = 0;
	int account_id = 0;
	int mailbox_id = 0;
	int mail_slot_size = 0;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id[%d]", account_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mailbox_id);
	EM_DEBUG_LOG("mail_slot_size[%d]", mail_slot_size);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &mail_slot_size);
	EM_DEBUG_LOG("mail_slot_size[%d]", mail_slot_size);

	if(emdaemon_set_mail_slot_size_of_mailbox(account_id, mailbox_id, mail_slot_size, &handle, &err))
		err = EMAIL_ERROR_NONE;
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 1");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 2");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_DEBUG_FUNC_END();
}

void stb_rename_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size  = 0;
	int	err = EMAIL_ERROR_NONE;
	int handle = 0;
	int mailbox_id = 0;
	int on_server = 0;
	char *mailbox_path = NULL;
	char *mailbox_alias = NULL;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mailbox_id);
	EM_DEBUG_LOG("mailbox_id[%d]", mailbox_id);

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("mailbox_path string size[%d]", buffer_size);
	if(buffer_size > 0)	  {
		mailbox_path = (char*)em_malloc(buffer_size);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, buffer_size, mailbox_path);
		EM_DEBUG_LOG("mailbox_path [%s]", mailbox_path);
	}

	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 2);
	EM_DEBUG_LOG("mailbox_alias string size[%d]", buffer_size);
	if(buffer_size > 0)	  {
		mailbox_alias = (char*)em_malloc(buffer_size);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, buffer_size, mailbox_alias);
		EM_DEBUG_LOG("mailbox_alias [%s]", mailbox_alias);
	}

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 3, sizeof(int), &on_server);
	EM_DEBUG_LOG("on_server[%d]", on_server);

	if ((err = emdaemon_rename_mailbox(mailbox_id, mailbox_path, mailbox_alias, on_server, &handle)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emdaemon_rename_mailbox failed [%d]", err);
	}

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 1");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed 2");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_SAFE_FREE(mailbox_alias);
	EM_SAFE_FREE(mailbox_path);

	EM_DEBUG_FUNC_END();
}

void stb_send_mail(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	char* local_stream = NULL;
	int mail_id;
	int handle;
	int err = EMAIL_ERROR_NONE;

	/* Mail_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mail_id);
	EM_DEBUG_LOG("mail_id [%d]", mail_id);

	if(emdaemon_send_mail(mail_id, &handle, &err)) {
		err = EMAIL_ERROR_NONE;
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

	EM_SAFE_FREE(local_stream);

	EM_DEBUG_FUNC_END();
}


/* obsolete - there is no api calling this function */
void stb_get_mailbox_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int counter = 0;
	char* local_stream = NULL;
	int account_id;
	email_mailbox_t* mailbox_list = NULL;
	int count = 0;
	int size = 0;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);

	if(emdaemon_get_mailbox_list(account_id, &mailbox_list, &count, &err))
		EM_DEBUG_LOG("emdaemon_get_mailbox_list - success");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	if(EMAIL_ERROR_NONE == err) {
		EM_DEBUG_LOG("Count [%d]", count);
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &count, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

		for(counter=0; counter<count; counter++) {
			EM_DEBUG_LOG("Name - %s", mailbox_list[counter].mailbox_name);

			local_stream = em_convert_mailbox_to_byte_stream(mailbox_list+counter, &size);

			if (!local_stream) {
				EM_DEBUG_EXCEPTION ("INVALID PARAM: local_stream NULL ");
				emcore_free_mailbox_list(&mailbox_list, count);
				return;
			}

			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
				EM_DEBUG_EXCEPTION("Add  Param mailbox failed  ");

			EM_SAFE_FREE(local_stream);
			size = 0;
		}
	}

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	emcore_free_mailbox_list(&mailbox_list, count);

	EM_DEBUG_FUNC_END();
}

void stb_delete_all_mails(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int mailbox_id = 0;
	int from_server = 0;
	int err = EMAIL_ERROR_NONE;

	/* mailbox_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mailbox_id);

	/* from_server */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &from_server);

	emdaemon_delete_mail_all(mailbox_id, from_server, NULL, &err);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_DEBUG_FUNC_END();
}

void stb_delete_mail(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int mailbox_id = 0;
	int from_server = 0;
	int counter = 0;
	int num;
	int *mail_ids = NULL;

	/* Mailbox */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mailbox_id);

	EM_DEBUG_LOG("mailbox_id [%d]", mailbox_id);

	/* Number of mail_ids */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &num);
	EM_DEBUG_LOG("number of mails [%d]", num);

	if (num > 0) {
		mail_ids = em_malloc(sizeof(int) * num);
		if (!mail_ids) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, num * sizeof(int), mail_ids);

	for(counter = 0; counter < num; counter++)
		EM_DEBUG_LOG("mail_ids [%d]", mail_ids[counter]);

	/* from_server */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 3, sizeof(int), &from_server);
	EM_DEBUG_LOG("from_server [%d]", from_server);

	emdaemon_delete_mail(mailbox_id, mail_ids, num, from_server, NULL, &err);

FINISH_OFF:
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	EM_SAFE_FREE(mail_ids);

	EM_DEBUG_FUNC_END();
}
void stb_clear_mail_data (HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

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
	int err = EMAIL_ERROR_NONE;
	char* local_rule_stream = NULL;
	email_rule_t rule = { 0 };

	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	if(buffer_size <= 0) {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_rule_stream = (char*)	emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 0);
	if(!local_rule_stream) {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	em_convert_byte_stream_to_rule(local_rule_stream, buffer_size, &rule);
	EM_DEBUG_LOG("account ID  [%d]", rule.account_id);

	/* call add_filter handler */
	emdaemon_add_filter(&rule, &err);

FINISH_OFF:
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	emcore_free_rule(&rule);

	EM_DEBUG_FUNC_END();
}


/* obsolete - there is no api calling this function */
void stb_get_rule(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int filter_id = 0;
	email_rule_t* rule = NULL;
	int size =0;
	char* local_rule_stream = NULL;

	filter_id = *((int*)emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 0));

	emdaemon_get_filter(filter_id, &rule, &err);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	/* insert a rule if there exists a rule */
	if ( rule ) {
		local_rule_stream = em_convert_rule_to_byte_stream(rule, &size);
		if(!local_rule_stream) { /*prevent 26265*/
			EM_DEBUG_EXCEPTION("em_convert_rule_to_byte_stream failed");
			emcore_free_rule(rule);
			EM_SAFE_FREE(rule);
			return;
		}

		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, local_rule_stream, size))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");

		EM_SAFE_FREE(local_rule_stream);
		emcore_free_rule(rule);
		EM_SAFE_FREE(rule);
	}

	if (!emipc_execute_stub_api(a_hAPI)) {
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
		return;
	}

	EM_DEBUG_FUNC_END();
}

/* obsolete - there is no api calling this function */
void stb_get_rule_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	char* local_stream = NULL;
	int count = 0;
	int size = 0;
	email_rule_t* filtering_list = NULL;

	emdaemon_get_filter_list(&filtering_list, &count, &err);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	/* insert rules if there exist rules*/
	if( count > 0 ) {
		EM_DEBUG_LOG("num of rules [%d]", count);
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &count, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

		for(i=0; i<count; i++) {
			EM_DEBUG_LOG("Value - %s", filtering_list[i].value);

			local_stream = em_convert_rule_to_byte_stream(filtering_list+i, &size);

			if(!local_stream) break;

			if(!emipc_add_dynamic_parameter(a_hAPI, ePARAMETER_OUT, local_stream, size))
				EM_DEBUG_EXCEPTION("emipc_add_dynamic_parameter failed  ");

			size = 0;
		}
	}

	if (!emipc_execute_stub_api(a_hAPI)) {
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}

	EM_DEBUG_FUNC_END();
}

/* obsolete - there is no api calling this function */
void stb_find_rule(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int err = EMAIL_ERROR_NONE;
	char* local_rule_stream = NULL;
	email_rule_t rule = { 0 };


	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);

	if(buffer_size > 0)	 {
		local_rule_stream = (char*)em_malloc(buffer_size);
		EM_NULL_CHECK_FOR_VOID(local_rule_stream);
		if(local_rule_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, buffer_size, local_rule_stream);
			em_convert_byte_stream_to_rule(local_rule_stream, buffer_size, &rule);
			EM_SAFE_FREE(local_rule_stream);
			EM_DEBUG_LOG("account ID  [%d]", rule.account_id);

			if(emdaemon_find_filter(&rule, &err))
				err = EMAIL_ERROR_NONE;

			if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
			if (!emipc_execute_stub_api(a_hAPI))
				EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

		}
	}
	EM_DEBUG_FUNC_END();
}

void stb_update_rule(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();

	int filter_id = 0;
	int buffer_size = 0;
	int err = EMAIL_ERROR_NONE;
	char* rule_stream = NULL;
	email_rule_t rule = {0};


	/* get filter_id */
	filter_id = *((int*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 0));

	/* get rule */
	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, 1);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	if(buffer_size <= 0)  {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	rule_stream	= (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 1);
	if(!rule_stream) {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	em_convert_byte_stream_to_rule(rule_stream, buffer_size, &rule);

	/* call update handler */
	emdaemon_update_filter(filter_id, &rule, &err);

FINISH_OFF:
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	emcore_free_rule(&rule);

	EM_DEBUG_FUNC_END();
}

void stb_move_all_mails(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int src_mailbox_id = 0, dst_mailbox_id = 0;

	/* src_mailbox_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &src_mailbox_id);
	EM_DEBUG_LOG("src_mailbox_id [%d]", src_mailbox_id);

	if (src_mailbox_id > 0)
		EM_DEBUG_LOG("src_mailbox_id [%d]", src_mailbox_id);
	else
		EM_DEBUG_LOG("src_mailbox_id == 0");

	/* dst_mailbox_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &dst_mailbox_id);
	EM_DEBUG_LOG("dst_mailbox_id [%d]", dst_mailbox_id);

	if (dst_mailbox_id > 0)
		EM_DEBUG_LOG("dst_mailbox_id [%d]", dst_mailbox_id);
	else
		EM_DEBUG_LOG("dst_mailbox_id == 0");

	if(emdaemon_move_mail_all_mails(src_mailbox_id, dst_mailbox_id, &err))
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
	int err = EMAIL_ERROR_NONE;
	email_flags_field_type field_type = 0;
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
		err = EMAIL_ERROR_OUT_OF_MEMORY;
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

	if (mail_ids)
		EM_SAFE_FREE(mail_ids);

	EM_DEBUG_FUNC_END();
}

void stb_add_mail(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int  buffer_size = 0;
	int  local_result = 0;
	int  result_attachment_data_count = 0;
	int  param_index = 0;
	int  sync_server = 0;
	int  err = EMAIL_ERROR_NONE;
	email_mail_data_t result_mail_data = {0};
	email_attachment_data_t *result_attachment_data = NULL;
	email_meeting_request_t result_meeting_request = {0};


	/* email_mail_data_t */;
	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);
	EM_DEBUG_LOG("email_attachment_data_t buffer_size[%d]", buffer_size);

	/* mail_data */
	if(buffer_size > 0)	 {
		char *stream = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index++);
		em_convert_byte_stream_to_mail_data(stream, buffer_size, &result_mail_data);
	}

	/* attachment */
	if (result_mail_data.attachment_count > 0) {
		buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);
		EM_DEBUG_LOG("email_attachment_data_t buffer_size[%d]", buffer_size);

		if(buffer_size > 0)	 {
			char *stream = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index);
			em_convert_byte_stream_to_attachment_data(stream, buffer_size, &result_attachment_data, &result_attachment_data_count);

			EM_DEBUG_LOG("result_attachment_data_count[%d]", result_attachment_data_count);

			if(result_attachment_data_count && !result_attachment_data) {
				EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_attachment_data failed");
				err = EMAIL_ERROR_ON_PARSING;
				goto FINISH_OFF;
			}
		}
	}

	param_index++;

	/* meeting request */
	EM_DEBUG_LOG("email_meeting_request_t");
	if ( result_mail_data.meeting_request_status == EMAIL_MAIL_TYPE_MEETING_REQUEST
		|| result_mail_data.meeting_request_status == EMAIL_MAIL_TYPE_MEETING_RESPONSE
		|| result_mail_data.meeting_request_status == EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);

		if(buffer_size > 0) {
			char* stream = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index++);
			em_convert_byte_stream_to_meeting_req(stream, buffer_size, &result_meeting_request);
		}
	}

	EM_DEBUG_LOG("sync_server");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, param_index++, sizeof(int), &sync_server);

	if( (err = emdaemon_add_mail(&result_mail_data, result_attachment_data, result_attachment_data_count, &result_meeting_request, sync_server)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emdaemon_add_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	local_result = 1;
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &result_mail_data.mail_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &result_mail_data.thread_id, sizeof(int)))
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

	emcore_free_mail_data(&result_mail_data);

	if(result_attachment_data)
		emcore_free_attachment_data(&result_attachment_data, result_attachment_data_count, NULL);

	emstorage_free_meeting_request(&result_meeting_request);

	em_flush_memory();

	EM_DEBUG_FUNC_END();
}


void stb_update_mail(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int  buffer_size = 0;
	int  local_result = 0;
	int  result_attachment_data_count = 0;
	int  param_index = 0;
	int  sync_server = 0;
	int  *temp_buffer = NULL;
	int  err = EMAIL_ERROR_NONE;
	email_mail_data_t result_mail_data = {0};
	email_attachment_data_t *result_attachment_data = NULL;
	email_meeting_request_t result_meeting_request = {0};

	EM_DEBUG_LOG("email_mail_data_t");
	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);

	if(buffer_size > 0)	 {
		char* stream = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index++);
		em_convert_byte_stream_to_mail_data(stream, buffer_size, &result_mail_data);
	}

	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);

	EM_DEBUG_LOG("email_attachment_data_t buffer_size[%d]", buffer_size);

	if(buffer_size > 0)	 {
		char *stream = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index);
		em_convert_byte_stream_to_attachment_data(stream, buffer_size, &result_attachment_data, &result_attachment_data_count);

		EM_DEBUG_LOG("result_attachment_data_count[%d]", result_attachment_data_count);

		if(result_attachment_data_count && !result_attachment_data) {
			EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_attachment_data failed");
			err = EMAIL_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}
	}
	param_index++;

	EM_DEBUG_LOG("email_meeting_request_t");

	if ( result_mail_data.meeting_request_status == EMAIL_MAIL_TYPE_MEETING_REQUEST
		|| result_mail_data.meeting_request_status == EMAIL_MAIL_TYPE_MEETING_RESPONSE
		|| result_mail_data.meeting_request_status == EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);

		if(buffer_size > 0) {
			char* stream = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index++);
			em_convert_byte_stream_to_meeting_req(stream, buffer_size, &result_meeting_request);
		}
	}

	EM_DEBUG_LOG("sync_server");

	temp_buffer = emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index++);

	if(!temp_buffer) {
		EM_DEBUG_EXCEPTION("emipc_get_nth_parameter_data[%d] failed", param_index - 1);
		err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
		goto FINISH_OFF;
	}

	sync_server = *temp_buffer;

	if( (err = emdaemon_update_mail(&result_mail_data, result_attachment_data,
			result_attachment_data_count, &result_meeting_request, sync_server)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emdaemon_update_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	local_result = 1;
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &result_mail_data.mail_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &result_mail_data.thread_id, sizeof(int)))
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

	emcore_free_mail_data(&result_mail_data);

	if(result_attachment_data)
		emcore_free_attachment_data(&result_attachment_data, result_attachment_data_count, NULL);

	emstorage_free_meeting_request(&result_meeting_request);

	em_flush_memory();
	EM_DEBUG_FUNC_END();
}


void stb_move_thread_to_mailbox(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int mailbox_id = 0, thread_id = 0, move_always_flag = 0;
	int err = EMAIL_ERROR_NONE;
	char *target_mailbox_name = NULL;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &thread_id);
	EM_DEBUG_LOG("thread_id [%d]", thread_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mailbox_id);
	EM_DEBUG_LOG("mailbox_id [%d]", mailbox_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &move_always_flag);
	EM_DEBUG_LOG("move_always_flag [%d]", move_always_flag);

	if(emdaemon_move_mail_thread_to_mailbox(thread_id, mailbox_id, move_always_flag, &err))
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
	int handle = 0;
	int err = EMAIL_ERROR_NONE;

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
	int handle = 0;
	int err = EMAIL_ERROR_NONE;

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

void stb_expunge_mails_deleted_flagged(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();

	int mailbox_id = 0, on_server = 0;
	int handle = 0;
	int err = EMAIL_ERROR_NONE;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), (void*)&mailbox_id);
	EM_DEBUG_LOG("mailbox_id [%d]", mailbox_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), (void*)&on_server);
	EM_DEBUG_LOG("on_server [%d]", on_server);

	if( (err = emdaemon_expunge_mails_deleted_flagged(mailbox_id, on_server, &handle)) != EMAIL_ERROR_NONE)
		EM_DEBUG_LOG("emdaemon_expunge_mails_deleted_flagged success");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter fail");
		return;
	}

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &handle, sizeof(int))) {
		EM_DEBUG_LOG("ipcAPI_AddParameter local_result failed ");
		return;
	}

	if (!emipc_execute_stub_api(a_hAPI)) {
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api fail");
		return;
	}
	EM_DEBUG_FUNC_END();
}

void stb_update_mail_attribute(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int mail_id_count = 0, counter = 0;
	email_mail_attribute_type attribute;
	/* email_mail_attribute_value_t value; */

	/* mail_id_count */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mail_id_count);
	EM_DEBUG_LOG("mail_id_count [%d]", mail_id_count);

	/* mail_id_array */
	int mail_id_array[mail_id_count];
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, mail_id_count * sizeof(int), mail_id_array);

	for(counter = 0; counter < mail_id_count; counter++)
		EM_DEBUG_LOG("mail_id[%d] [%d]", counter, mail_id_array[counter]);

	/* attribute */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &attribute);
	EM_DEBUG_LOG("attribute [%d]", attribute);

	/* value */
	/*
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 3, sizeof(int), &value);
	EM_DEBUG_LOG("target_mailbox_id [%d]", mailbox_id);
	*/

	/*
	if(emdaemon_move_mail(mail_ids, num, mailbox_id, EMAIL_MOVED_BY_COMMAND, 0, &err))
		EM_DEBUG_LOG("emdaemon_move_mail success");
	*/

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter fail");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api fail");
	EM_DEBUG_FUNC_END();
}

void stb_move_mail(HIPC_API a_hAPI)
{
	int err = EMAIL_ERROR_NONE;
	int num = 0, counter = 0, mailbox_id = 0;

	/* Number of mail_ids */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &num);
	EM_DEBUG_LOG("number of mails [%d]", num);

	/* mail_id */
	int mail_ids[num];
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, num * sizeof(int), mail_ids);

	for(counter = 0; counter < num; counter++)
		EM_DEBUG_LOG("mailids [%d]", mail_ids[counter]);

	/* target_mailbox_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &mailbox_id);
	EM_DEBUG_LOG("target_mailbox_id [%d]", mailbox_id);

	if (mailbox_id > 0)
		EM_DEBUG_LOG("mailbox_id [%d]", mailbox_id);
	else
		EM_DEBUG_LOG("mailbox_id == 0");

	if(emdaemon_move_mail(mail_ids, num, mailbox_id, EMAIL_MOVED_BY_COMMAND, 0, &err))
		EM_DEBUG_LOG("emdaemon_move_mail success");

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
	int err = EMAIL_ERROR_NONE;

	/* filter_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &filter_id);

	if(emdaemon_delete_filter(filter_id, &err))
		err = EMAIL_ERROR_NONE;

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
	int err = EMAIL_ERROR_NONE;
	int mail_id = -1;
	int attachment_count = 0;
	char* attachment_stream = NULL;
	email_attachment_data_t* attachment = NULL;

	/* mail_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mail_id);

	/* attachment */
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);

	if(buffer_size > 0)	 {
		attachment_stream = (char*)em_malloc(buffer_size);
		EM_NULL_CHECK_FOR_VOID(attachment_stream);
		if(attachment_stream) {
			emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, buffer_size, attachment_stream);
			em_convert_byte_stream_to_attachment_data(attachment_stream, buffer_size, &attachment, &attachment_count);
			EM_SAFE_FREE(attachment_stream);
		}
	}

	if (!attachment) {
		EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_attachment_data failed  ");
		return;
	}

	emdaemon_add_attachment(mail_id, attachment, &err);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
	if(EMAIL_ERROR_NONE == err) {
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
	int err = EMAIL_ERROR_NONE;
	int attachment_id;
	char* attachment_stream = NULL;
	email_attachment_data_t* attachment = NULL;
	int size = 0;

	/* attachment_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &attachment_id);

	emdaemon_get_attachment(attachment_id, &attachment, &err);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");

	if(EMAIL_ERROR_NONE == err) {
		EM_DEBUG_LOG("emdaemon_get_attachment - Success");
		/* attachment */
		attachment_stream = em_convert_attachment_data_to_byte_stream(attachment, 1, &size);
		if(!attachment_stream) { /*prevent 26263*/
			emcore_free_attachment_data(&attachment, 1, &err);
			return;
		}
		EM_NULL_CHECK_FOR_VOID(attachment_stream);

		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, attachment_stream, size))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
	}

	if (!emipc_execute_stub_api(a_hAPI)) {
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}

	EM_SAFE_FREE(attachment_stream);
	emcore_free_attachment_data(&attachment, 1, &err);

	EM_DEBUG_FUNC_END();
}

void stb_get_imap_mailbox_list(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int account_id = 0;
	int handle = 0;

	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);

	if(emdaemon_get_imap_mailbox_list(account_id, "", &handle, &err))
		err = EMAIL_ERROR_NONE;

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
	int err = EMAIL_ERROR_NONE;
	int attachment_id = 0;

	/* attachment_index */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &attachment_id);

	emdaemon_delete_mail_attachment(attachment_id, &err);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_DEBUG_FUNC_END();
}

void stb_download_attachment(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	int mail_id = 0;
	int nth = 0;
	int handle = 0;
	int account_id = 0;

	EM_DEBUG_LOG("account_id");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);

	EM_DEBUG_LOG("mail_id");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mail_id);

	EM_DEBUG_LOG("nth");
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &nth);

	if(emdaemon_download_attachment(account_id, mail_id, nth, &handle, &err)) {
		err = EMAIL_ERROR_NONE;

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

	EM_DEBUG_FUNC_END();
}

void stb_mail_send_saved(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int account_id = 0;

	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);

	EM_DEBUG_LOG("calling emdaemon_send_mail_saved");
	if(emdaemon_send_mail_saved(account_id, NULL, &err))
		err = EMAIL_ERROR_NONE;

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
	int err = EMAIL_ERROR_NONE;

	/* read_mail_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &read_mail_id);
	EM_DEBUG_LOG("read_mail_id [%d]", read_mail_id);

	if( (err = emcore_add_read_receipt(read_mail_id, &receipt_mail_id)) != EMAIL_ERROR_NONE )
		EM_DEBUG_EXCEPTION("emcore_add_read_receipt failed [%d]", err);

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	if (err == EMAIL_ERROR_NONE) {
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
	int err = EMAIL_ERROR_NONE;

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
	int err = EMAIL_ERROR_NONE;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id [%d]", account_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &handle);
	EM_DEBUG_LOG("handle [%d]", handle);

	if(emdaemon_cancel_job(account_id, handle, &err))
		err = EMAIL_ERROR_NONE;

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");
	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");
	EM_DEBUG_FUNC_END();
}

void stb_get_pending_job(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	email_action_t action = -1;
	int account_id = 0;
	int mail_id = -1;
	int err = EMAIL_ERROR_NONE;
	email_event_status_type_t status = 0;

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &action);
	EM_DEBUG_LOG("action [%d]", action);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id [%d]", account_id);

	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &mail_id);
	EM_DEBUG_LOG("mail_id [%d]", mail_id);

	if(emdaemon_get_pending_job(action, account_id, mail_id, &status))
		err = EMAIL_ERROR_NONE;
	else
		err = EMAIL_ERROR_UNKNOWN;

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
	email_event_t *event_queue = NULL;
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
	int err = EMAIL_ERROR_NONE;
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
	int                    err = EMAIL_ERROR_NONE;
	int                    account_id = 0;
	int					   mailbox_id = 0;
	int                    buffer_size = 0;
	int                    search_filter_count = 0;
	char                  *stream_for_search_filter_list = NULL;
	unsigned int           job_handle = 0;
	email_search_filter_t *search_filter_list = NULL;

	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);
	EM_DEBUG_LOG("account_id [%d]", account_id);

	/* mailbox_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &mailbox_id);
	EM_DEBUG_LOG("mailbox_id [%d]", mailbox_id);

	/* search_filter_list */
	buffer_size = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 2);

	if(buffer_size > 0)	 {
		stream_for_search_filter_list = (char*)em_malloc(buffer_size);
		EM_NULL_CHECK_FOR_VOID(stream_for_search_filter_list);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, buffer_size, stream_for_search_filter_list);
		em_convert_byte_stream_to_search_filter(stream_for_search_filter_list, &search_filter_list, &search_filter_count);
		EM_SAFE_FREE(stream_for_search_filter_list);
	}

	if(!emdaemon_search_mail_on_server(account_id ,mailbox_id, search_filter_list, search_filter_count, &job_handle, &err))
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

	EM_SAFE_FREE(search_filter_list);

	EM_DEBUG_FUNC_END();
}

void stb_clear_result_of_search_mail_on_server(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int                      err = EMAIL_ERROR_NONE;
	int                      account_id = 0;

	/* account_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &account_id);

	EM_DEBUG_LOG("account_id [%d]", account_id);

	if (!emstorage_delete_mail_by_mailbox(account_id, EMAIL_SEARCH_RESULT_MAILBOX_NAME, true, &err)) 
		EM_DEBUG_EXCEPTION(" emstorage_get_mailbox_by_mailbox_type failed [%d]", err);

	if (!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	EM_DEBUG_FUNC_END();
}



void stb_add_account_with_validation(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int buffer_size = 0;
	int local_result = 0;
	int handle = 0;
	char* stream = NULL;
	email_account_t *account = NULL;
	int err = EMAIL_ERROR_NONE;

	/* get account info */
	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	EM_DEBUG_LOG("size [%d]", buffer_size);
	if(buffer_size <= 0) {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	stream =(char*)	emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, 0);
	if(!stream) {
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	account = em_malloc(sizeof(email_account_t));

	if(account == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	em_convert_byte_stream_to_account(stream, buffer_size, account);

	if((err = emcore_add_account_to_unvalidated_account_list(account)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_add_account_to_unvalidated_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if(!emdaemon_validate_account_and_create(account, &handle, &err)) {
		EM_DEBUG_EXCEPTION("emdaemon_validate_account_and_create fail [%d]", err);
		goto FINISH_OFF;
	}
#ifdef __FEATURE_AUTO_POLLING__
	/*  start auto polling, if check_interval not zero */
	if(account->check_interval > 0) {
		if(!emdaemon_add_polling_alarm( account->account_id, account->check_interval))
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

FINISH_OFF:
	if ( local_result == 0 ) { /* there is an error */
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &local_result, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed : local_result ");
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed : err");
		if (!emipc_execute_stub_api(a_hAPI))
			EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");
	}
/*	note: account is freed in thread_func_branch_command, which is run by other thread */
/*	emcore_free_account(account); */

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
		file_path = em_malloc(file_path_length);
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
		file_path = em_malloc(file_path_length);
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

void stb_add_certificate(HIPC_API a_hAPI)
{
	int err = EMAIL_ERROR_NONE;
	int cert_file_len = 0;
	int email_address_len = 0;
	char *cert_file_path = NULL;
	char *email_address = NULL;

	cert_file_len = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	if (cert_file_len > 0) {
		cert_file_path = em_malloc(cert_file_len + 1);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, cert_file_len, cert_file_path);
	}

	email_address_len = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 1);
	if (email_address_len > 0) {
		email_address = em_malloc(email_address_len + 1);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, email_address_len, email_address);
	}

	if (!emcore_add_public_certificate(cert_file_path, email_address, &err)) {
		EM_DEBUG_EXCEPTION("em_core_smime_add_certificate failed");
	}

	if (!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");

	if (EMAIL_ERROR_NONE == err) {
		EM_DEBUG_LOG("email_mail_add_attachment -Success");
	}

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_SAFE_FREE(cert_file_path);          
	EM_SAFE_FREE(email_address); 
	EM_DEBUG_FUNC_END();    
}

void stb_delete_certificate(HIPC_API a_hAPI)
{
	int err = EMAIL_ERROR_NONE;
	int email_address_len = 0;
	char *email_address = NULL;
	char temp_email_address[130] = {0, };

	email_address_len = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	if (email_address_len > 0) {
		EM_DEBUG_LOG("email address string length [%d]", email_address_len);
		email_address = em_malloc(email_address_len + 1);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, email_address_len, email_address);
		EM_DEBUG_LOG("email address [%s]", email_address);
	}

	SNPRINTF(temp_email_address, sizeof(temp_email_address), "<%s>", email_address);
	if (!emcore_delete_public_certificate(temp_email_address, &err)) {
		EM_DEBUG_EXCEPTION("em_core_smime_add_certificate failed");
	}

	if (!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");

	if (EMAIL_ERROR_NONE == err) {
		EM_DEBUG_LOG("email_mail_add_attachment -Success");
	}

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	
	EM_SAFE_FREE(email_address);
	EM_DEBUG_FUNC_END();    
}

void stb_verify_signature(HIPC_API a_hAPI)
{
	int err = EMAIL_ERROR_NONE;
	int verify = 0;
	int mail_id = 0;
	int count = 0;
	int attachment_tbl_count = 0;
	email_mail_data_t *mail_data = NULL;
	emstorage_attachment_tbl_t *attachment_tbl_list = NULL;

	err = emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &mail_id);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_get_parameter failed");
		goto FINISH_OFF;
	}

	if (!emcore_get_mail_data(mail_id, &mail_data)) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_data failed");
		goto FINISH_OFF;
	}

	if ((err = emstorage_get_attachment_list(mail_id, true, &attachment_tbl_list, &attachment_tbl_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_get_attachment_list failed : [%d]", err);
		goto FINISH_OFF;
	}

	if (attachment_tbl_count <= 0) {
		EM_DEBUG_EXCEPTION("Invalid signed mail");
		err = EMAIL_ERROR_INVALID_MAIL;
		goto FINISH_OFF;
	}

	for (count = 0; count < attachment_tbl_count; count++) {
		if (strcasestr(attachment_tbl_list[count].attachment_name, "p7s") == NULL)
			continue;

		break;	
	}

	if (!emcore_verify_signature(attachment_tbl_list[count].attachment_path, mail_data->file_path_mime_entity, &verify, &err)) {
		EM_DEBUG_EXCEPTION("emcore_verify_signature failed");
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &verify, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");

	if (verify) {
		EM_DEBUG_LOG("Verify S/MIME signed mail-Success");
	}

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");


	if (attachment_tbl_list)
		emstorage_free_attachment(&attachment_tbl_list, attachment_tbl_count, NULL);

	if (mail_data) {
		emcore_free_mail_data(mail_data);
		EM_SAFE_FREE(mail_data);
	}

	EM_DEBUG_FUNC_END();
}

void stb_verify_certificate(HIPC_API a_hAPI)
{
	int err = EMAIL_ERROR_NONE;
	int verify = 0;
	int cert_file_len = 0;
	char *cert_file_path = 0;

	cert_file_len = emipc_get_parameter_length(a_hAPI, ePARAMETER_IN, 0);
	if (cert_file_len > 0) {
		cert_file_path = em_malloc(cert_file_len + 1);
		emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, cert_file_len, cert_file_path);
	}

	if (!emcore_verify_certificate(cert_file_path, &verify, &err)) {
		EM_DEBUG_EXCEPTION("em_core_smime_add_certificate failed");
	}

	if (!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &verify, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter local_result failed ");

	if (verify) {
		EM_DEBUG_LOG("Verify S/MIME signed mail-Success");
	}

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_SAFE_FREE(cert_file_path);           
	EM_DEBUG_FUNC_END();    
}

void stb_ping_service(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");	
	EM_DEBUG_FUNC_END();
}

void stb_update_notification_bar_for_unread_mail(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE, account_id;

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

void stb_show_user_message(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int param_id = 0;
	int param_error = 0;
	email_action_t param_action = 0;

	/* param_id */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 0, sizeof(int), &param_id);
	EM_DEBUG_LOG("param_id [%d]", param_id);

	/* param_action */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 1, sizeof(int), &param_action);
	EM_DEBUG_LOG("param_action [%d]", param_action);

	/* param_error */
	emipc_get_parameter(a_hAPI, ePARAMETER_IN, 2, sizeof(int), &param_error);
	EM_DEBUG_LOG("param_error [%d]", param_error);

	if( (err = emcore_show_user_message(param_id, param_action, param_error)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_show_user_message failed [%d]", err);
	}

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed  ");

	EM_DEBUG_FUNC_END("err [%d]", err);
}

void stb_write_mime_file(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int i = 0;
	int exist_file_path = 0;
	int buffer_size = 0;
	int local_result = 0;
	int result_attachment_data_count = 0;
	int param_index = 0;
	int err = EMAIL_ERROR_NONE;
	int ret = EMAIL_ERROR_NONE;
	char *file_path = NULL;
	char *output_file_path = NULL;
	email_mail_data_t result_mail_data = {0};
	email_attachment_data_t *result_attachment_data = NULL;
	emstorage_mail_tbl_t *input_mail_tbl_data = NULL;
	emstorage_attachment_tbl_t *input_attachment_tbl_data = NULL;
	
	/* email_mail_data_t */;
	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);
	EM_DEBUG_LOG("email_mail_data_t buffer_size[%d]", buffer_size);

	/* mail_data */
	if(buffer_size > 0)	 {
		char *stream = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index++);
		em_convert_byte_stream_to_mail_data(stream, buffer_size, &result_mail_data);
	}

	/* attachment */
	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);
	EM_DEBUG_LOG("email_attachment_data_t buffer_size[%d]", buffer_size);

	if(buffer_size > 0)	 {
		char *stream = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index);
		em_convert_byte_stream_to_attachment_data(stream, buffer_size, &result_attachment_data, &result_attachment_data_count);

		EM_DEBUG_LOG("result_attachment_data_count[%d]", result_attachment_data_count);

		if(result_attachment_data_count && !result_attachment_data) {
			EM_DEBUG_EXCEPTION("em_convert_byte_stream_to_attachment_data failed");
			err = EMAIL_ERROR_ON_PARSING;
			goto FINISH_OFF;
		}
	}

	param_index++;
	
	buffer_size = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);
	if (buffer_size > 0) {
		output_file_path = (char *)emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index);
		EM_DEBUG_LOG("output_file_path : [%s]", output_file_path);
	}

	/* Convert mail_data to mail_tbl */
	if (!em_convert_mail_data_to_mail_tbl(&result_mail_data, 1, &input_mail_tbl_data, &err)) {
		EM_DEBUG_EXCEPTION("em_convert_mail_data_to_mail_tbl failed");
		goto FINISH_OFF;
	}

	/* Convert attachment_data to attachment_tbl */
	if (result_attachment_data_count > 0) {
		input_attachment_tbl_data = (emstorage_attachment_tbl_t *)em_malloc(sizeof(emstorage_attachment_tbl_t) * result_attachment_data_count);
		if (input_attachment_tbl_data == NULL) {
			EM_DEBUG_EXCEPTION("em_malloc failed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}
	}

	for (i = 0; i < result_attachment_data_count ; i++) {
		input_attachment_tbl_data[i].attachment_id = result_attachment_data[i].attachment_id;
		input_attachment_tbl_data[i].attachment_name = EM_SAFE_STRDUP(result_attachment_data[i].attachment_name);
		input_attachment_tbl_data[i].attachment_path = EM_SAFE_STRDUP(result_attachment_data[i].attachment_path);
		input_attachment_tbl_data[i].attachment_size = result_attachment_data[i].attachment_size;
		input_attachment_tbl_data[i].mail_id = result_attachment_data[i].mail_id;
		input_attachment_tbl_data[i].account_id = result_attachment_data[i].account_id;
		input_attachment_tbl_data[i].mailbox_id = result_attachment_data[i].mailbox_id;
		input_attachment_tbl_data[i].attachment_save_status = result_attachment_data[i].save_status;
		input_attachment_tbl_data[i].attachment_drm_type = result_attachment_data[i].drm_status;
		input_attachment_tbl_data[i].attachment_inline_content_status = result_attachment_data[i].inline_content_status;
		input_attachment_tbl_data[i].attachment_mime_type = EM_SAFE_STRDUP(result_attachment_data[i].attachment_mime_type);
	}

	if ((err = em_verify_email_address_of_mail_tbl(input_mail_tbl_data, false)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_verify_email_address_of_mail_tbl failed : [%d]", err);
		goto FINISH_OFF;
	}

	if (!emcore_make_rfc822_file_from_mail(input_mail_tbl_data, input_attachment_tbl_data, result_attachment_data_count, NULL, &file_path, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emcore_make_rfc822_file_from_mail failed");
		goto FINISH_OFF;
	}

	if (output_file_path) {
		if (!emstorage_move_file(file_path, output_file_path, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_move_file failed : [%d]", err);
			goto FINISH_OFF;
		}
	} else {
		exist_file_path = 1;
		output_file_path = EM_SAFE_STRDUP(file_path);
	}


	if (!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, (char *)output_file_path, EM_SAFE_STRLEN(output_file_path) + 1)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		err = EMAIL_ERROR_NULL_VALUE;
		goto FINISH_OFF;
	}

	local_result = 1;

FINISH_OFF:
	if ( local_result == 0 ) {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	}

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	emstorage_free_attachment(&input_attachment_tbl_data, result_attachment_data_count, &ret);

	emstorage_free_mail(&input_mail_tbl_data, 1, &ret);

	emcore_free_mail_data(&result_mail_data);

	if(result_attachment_data)
		emcore_free_attachment_data(&result_attachment_data, result_attachment_data_count, NULL);

	EM_SAFE_FREE(file_path);
	if (exist_file_path)
		EM_SAFE_FREE(output_file_path);

	em_flush_memory();
	EM_DEBUG_FUNC_END();
}

void stb_handle_task(int task_type, HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int param_index = 0;
	int is_async_task = (task_type > EMAIL_ASYNC_TASK_BOUNDARY)?1:0;
	int err = EMAIL_ERROR_NONE;
	int task_id = 0;
	int task_parameter_length = 0;
	char *task_parameter = NULL;

	/* task_parameter_length */;
	task_parameter_length = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);
	EM_DEBUG_LOG("task_parameter_length [%d]", task_parameter_length);

	/* task_parameter */
	if(task_parameter_length > 0)	 {
		task_parameter = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index++);
	}

	if(is_async_task) {
		/* add async task */
		if((err = emcore_add_task_to_task_table(task_type, EMAIL_TASK_PRIORITY_MID, task_parameter, task_parameter_length, &task_id)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_add_task_to_task_pool failed [%d]", err);
			goto FINISH_OFF;
		}
	}
	else {
		/* do sync task */
	}

FINISH_OFF:
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if(is_async_task) {
		if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &task_id, sizeof(int)))
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
	}

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	em_flush_memory();
	EM_DEBUG_FUNC_END("err [%d]", err);
}

void* thread_func_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT(void *input_param)
{
	EM_DEBUG_FUNC_BEGIN("input_param [%p]", input_param);
	int err = EMAIL_ERROR_NONE;
	int err_for_signal = EMAIL_ERROR_NONE;
	int i = 0;
	int task_id = THREAD_SELF();
	task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT *task_param = input_param;

	/* Send start signal */
	if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT, task_id, EMAIL_TASK_STATUS_STARTED, EMAIL_ERROR_NONE, 0)) != EMAIL_ERROR_NONE)
		EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);

	for(i = 0; i < task_param->mail_id_count; i++) {
		if((err = emcore_move_mail_to_another_account(task_param->mail_id_array[i], task_param->source_mailbox_id, task_param->target_mailbox_id, task_id)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_move_mail_to_another_account failed [%d]", err);
			goto FINISH_OFF;
		}

		/* Send progress signal */
		if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT, task_id, EMAIL_TASK_STATUS_IN_PROGRESS, i, task_param->mail_id_count)) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);
	}

FINISH_OFF:
	/* Send finish signal */
	if(err == EMAIL_ERROR_NONE) {
		if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT, task_id, EMAIL_TASK_STATUS_FINISHED, EMAIL_ERROR_NONE, 0)) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);
	}
	else {
		if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT, task_id, EMAIL_TASK_STATUS_FAILED, err, 0)) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);
	}

	/* Free task parameter */
	EM_SAFE_FREE(task_param->mail_id_array);
	EM_SAFE_FREE(task_param);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return SUCCESS;
}

void stb_move_mails_to_mailbox_of_another_account(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int param_index = 0;
	int err = EMAIL_ERROR_NONE;
	int task_parameter_length = 0;
	int thread_error;
	thread_t task_id;
	char *task_parameter = NULL;
	task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT *decoded_parameter = NULL;

	/* task_parameter_length */;
	task_parameter_length = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);

	if(task_parameter_length <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM : task_parameter_length [%d]", task_parameter_length);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* task_parameter */
	task_parameter = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index++);

	if((err = email_decode_task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT(task_parameter, task_parameter_length, (void**)&decoded_parameter)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_decode_task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT failed[%d]", err);
		goto FINISH_OFF;
	}

	THREAD_CREATE(task_id, thread_func_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT, (void*)decoded_parameter, thread_error);

	if(thread_error != 0) {
		EM_DEBUG_EXCEPTION("THREAD_CREATE failed [%d]", thread_error);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	THREAD_DETACH(task_id);

FINISH_OFF:
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &task_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	EM_DEBUG_FUNC_END("err [%d]", err);
}

void* thread_func_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX(void *input_param)
{
	EM_DEBUG_FUNC_BEGIN("input_param [%p]", input_param);
	int err = EMAIL_ERROR_NONE;
	int err_for_signal = EMAIL_ERROR_NONE;
	int task_id = THREAD_SELF();
	task_parameter_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX *task_param = input_param;

	/* Send start signal */
	if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX, task_id, EMAIL_TASK_STATUS_STARTED, EMAIL_ERROR_NONE, 0)) != EMAIL_ERROR_NONE)
		EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);

	if((err = emcore_delete_mailbox_ex(task_param->account_id, task_param->mailbox_id_array, task_param->mailbox_id_count, task_param->on_server)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_delete_mailbox_ex failed[%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	/* Send finish signal */
	if(err == EMAIL_ERROR_NONE) {
		if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX, task_id, EMAIL_TASK_STATUS_FINISHED, EMAIL_ERROR_NONE, 0)) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);
	}
	else {
		if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX, task_id, EMAIL_TASK_STATUS_FAILED, err, 0)) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);
	}

	/* Free task parameter */
	EM_SAFE_FREE(task_param->mailbox_id_array);
	EM_SAFE_FREE(task_param);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return SUCCESS;
}

void stb_delete_mailbox_ex(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int param_index = 0;
	int err = EMAIL_ERROR_NONE;
	int task_parameter_length = 0;
	int thread_error = 0;
	thread_t task_id;
	char *task_parameter = NULL;
	task_parameter_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX *decoded_parameter = NULL;

	/* task_parameter_length */;
	task_parameter_length = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);

	if(task_parameter_length <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM : task_parameter_length [%d]", task_parameter_length);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* task_parameter */
	task_parameter = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index++);

	if((err = email_decode_task_parameter_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX(task_parameter, task_parameter_length, (void**)&decoded_parameter)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_decode_task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT failed[%d]", err);
		goto FINISH_OFF;
	}

	THREAD_CREATE(task_id, thread_func_EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX, (void*)decoded_parameter, thread_error);

	if(thread_error != 0) {
		EM_DEBUG_EXCEPTION("THREAD_CREATE failed [%d]", thread_error);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	THREAD_DETACH(task_id);

FINISH_OFF:
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &task_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	EM_DEBUG_FUNC_END("err [%d]", err);
}

void* thread_func_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL(void *input_param)
{
	EM_DEBUG_FUNC_BEGIN("input_param [%p]", input_param);
	int err = EMAIL_ERROR_NONE;
	int err_for_signal = EMAIL_ERROR_NONE;
	int task_id = THREAD_SELF();
	task_parameter_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL *task_param = input_param;

	/* Send start signal */
	if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL, task_id, EMAIL_TASK_STATUS_STARTED, EMAIL_ERROR_NONE, 0)) != EMAIL_ERROR_NONE)
		EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);

	if((err = emcore_send_mail_with_downloading_attachment_of_original_mail(task_param->mail_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_send_mail_with_downloading_attachment_of_original_mail failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:
	/* Send finish signal */
	if(err == EMAIL_ERROR_NONE) {
		if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL, task_id, EMAIL_TASK_STATUS_FINISHED, EMAIL_ERROR_NONE, 0)) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);
	}
	else {
		if((err_for_signal = emcore_send_task_status_signal(EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL, task_id, EMAIL_TASK_STATUS_FAILED, err, 0)) != EMAIL_ERROR_NONE)
			EM_DEBUG_LOG("emcore_send_task_status_signal failed [%d]", err_for_signal);
	}

	/* Free task parameter */
	EM_SAFE_FREE(task_param);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return SUCCESS;
}

void stb_send_mail_with_downloading_attachment_of_original_mail(HIPC_API a_hAPI)
{
	EM_DEBUG_FUNC_BEGIN();
	int param_index = 0;
	int err = EMAIL_ERROR_NONE;
	int task_parameter_length = 0;
	int thread_error = 0;
	thread_t task_id;
	char *task_parameter = NULL;
	task_parameter_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL *decoded_parameter = NULL;

	/* task_parameter_length */;
	task_parameter_length = emipc_get_nth_parameter_length(a_hAPI, ePARAMETER_IN, param_index);

	if(task_parameter_length <= 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM : task_parameter_length [%d]", task_parameter_length);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* task_parameter */
	task_parameter = (char*) emipc_get_nth_parameter_data(a_hAPI, ePARAMETER_IN, param_index++);

	if((err = email_decode_task_parameter_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL(task_parameter, task_parameter_length, (void**)&decoded_parameter)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("email_decode_task_parameter_EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT failed[%d]", err);
		goto FINISH_OFF;
	}

	THREAD_CREATE(task_id, thread_func_EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL, (void*)decoded_parameter, thread_error);

	if(thread_error != 0) {
		EM_DEBUG_EXCEPTION("THREAD_CREATE failed [%d]", thread_error);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	THREAD_DETACH(task_id);

FINISH_OFF:
	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &err, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if(!emipc_add_parameter(a_hAPI, ePARAMETER_OUT, &task_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");

	if (!emipc_execute_stub_api(a_hAPI))
		EM_DEBUG_EXCEPTION("emipc_execute_stub_api failed");

	EM_DEBUG_FUNC_END("err [%d]", err);
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

		case _EMAIL_API_SET_MAILBOX_TYPE:
			stb_set_mailbox_type(a_hAPI);
			break;

		case _EMAIL_API_SET_LOCAL_MAILBOX:
			stb_set_local_mailbox(a_hAPI);
			break;

		case _EMAIL_API_SET_MAIL_SLOT_SIZE:
			stb_set_mail_slot_size_of_mailbox(a_hAPI);
			break;

		case _EMAIL_API_RENAME_MAILBOX:
			stb_rename_mailbox(a_hAPI);
			break;

		case _EMAIL_API_SEND_MAIL:
			stb_send_mail(a_hAPI);
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
			stb_update_rule(a_hAPI);
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

		case _EMAIL_API_EXPUNGE_MAILS_DELETED_FLAGGED:
			stb_expunge_mails_deleted_flagged(a_hAPI);
			break;

		case _EMAIL_API_UPDATE_MAIL_ATTRIBUTE:
			stb_update_mail_attribute(a_hAPI);
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

		case _EMAIL_API_CLEAR_RESULT_OF_SEARCH_MAIL_ON_SERVER :
			stb_clear_result_of_search_mail_on_server(a_hAPI);
			break;

		case _EMAIL_API_ADD_ACCOUNT_WITH_VALIDATION :
			stb_add_account_with_validation(a_hAPI);
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

		case _EMAIL_API_ADD_CERTIFICATE:
			stb_add_certificate(a_hAPI);
			break;

		case _EMAIL_API_DELETE_CERTIFICATE:
			stb_delete_certificate(a_hAPI);
			break;

		case _EMAIL_API_VERIFY_SIGNATURE:
			stb_verify_signature(a_hAPI);
			break;

		case _EMAIL_API_VERIFY_CERTIFICATE:
			stb_verify_certificate(a_hAPI);
			break;

		case _EMAIL_API_PING_SERVICE :
			stb_ping_service(a_hAPI);
			break;

		case _EMAIL_API_UPDATE_NOTIFICATION_BAR_FOR_UNREAD_MAIL :
			stb_update_notification_bar_for_unread_mail(a_hAPI);
			break;

		case _EMAIL_API_SHOW_USER_MESSAGE :
			stb_show_user_message(a_hAPI);
			break;

		case _EMAIL_API_WRITE_MIME_FILE :
			stb_write_mime_file(a_hAPI);
			break;

		case EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT :
			stb_move_mails_to_mailbox_of_another_account(a_hAPI);
			break;

		case EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX :
			stb_delete_mailbox_ex(a_hAPI);
			break;

		case EMAIL_ASYNC_TASK_SEND_MAIL_WITH_DOWNLOADING_ATTACHMENT_OF_ORIGINAL_MAIL :
			stb_send_mail_with_downloading_attachment_of_original_mail(a_hAPI);
			break;
	}
	EM_DEBUG_FUNC_END();
}

GMainLoop *g_mainloop = NULL;

static void callback_for_sigterm(int signum)
{
	EM_DEBUG_FUNC_BEGIN("signum [%d]", signum);

	if (g_mainloop)
		g_main_loop_quit(g_mainloop);

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int main(int argc, char *argv[])
{
	/* Do the email-service Initialization 
       1. Create all DB tables and load the email engine */
	EM_DEBUG_LOG("Email service begin");
	int err = 0, ret;
	GMainLoop *mainloop;

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

	signal(SIGPIPE, SIG_IGN);              /* to ignore signal 13(SIGPIPE) */
	signal(SIGTERM, callback_for_sigterm); /* to handle signal 15(SIGTERM) - power off case*/

	emdaemon_initialize(&err);

	/* Start auto polling */
#ifdef __FEATURE_AUTO_POLLING__
	emdaemon_start_auto_polling(&err);
#endif

	mainloop = g_main_loop_new(NULL, 0);
	g_mainloop = mainloop;

	g_type_init();

	g_main_loop_run(mainloop);
	
	/* Clean up resources */
	g_main_loop_unref(mainloop);

	g_mainloop = NULL;

	emipc_finalize_stub();

	emdaemon_finalize(NULL);

	EM_DEBUG_FUNC_END();
	return 0;
}

