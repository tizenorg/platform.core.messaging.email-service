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
 * File: email-daemon-init.c
 * Desc: email-daemon Initialization
 *
 * Auth:
 *
 * History:
 *    2006.08.16 : created
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <vconf.h>
#include <vconf-internal-account-keys.h>
#include <dbus/dbus.h>
#include <dlfcn.h>           /* added for Disabling the Pthread flag log */

#include "email-daemon.h"
#include "email-storage.h"
#include "email-debug-log.h"
#include "email-daemon-account.h"
#include "email-daemon-auto-poll.h"   
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-event.h" 
#include "email-core-account.h"    
#include "email-core-mailbox.h"    
#include "email-core-api.h"
#include "email-core-smtp.h"    
#include "email-core-global.h"
#include "email-storage.h"   
#include "email-core-sound.h" 
#include "email-core-task-manager.h"

extern int g_client_count ;

/*  static functions */
static int _emdaemon_load_email_core()
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;

	/* initialize mail core */
	if (!emcore_init(&err))
		goto FINISH_OFF;

	if (emcore_start_event_loop(&err) < 0)
		goto FINISH_OFF;

	if (emcore_start_event_loop_for_sending_mails(&err) < 0)
		goto FINISH_OFF;

	if ((err = emcore_start_task_manager_loop()) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_start_task_manager_loop failed [%d]",err);
		goto FINISH_OFF;
	}

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
	if (emcore_start_thread_for_downloading_partial_body(&err) < 0) {
		EM_DEBUG_EXCEPTION("emcore_start_thread_for_downloading_partial_body failed [%d]",err);
		goto FINISH_OFF;
	}
#endif
	if (emcore_start_thread_for_alerting_new_mails(&err) < 0)
		goto FINISH_OFF;

FINISH_OFF:

	return err;
}

static int _emdaemon_unload_email_core()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	/* finish event loop */
	emcore_stop_event_loop(&err);
	emcore_stop_task_manager_loop();

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static void callback_for_SYNC_ALL_STATUS_from_account_svc(keynode_t *input_node, void *input_user_data)
{
	EM_DEBUG_FUNC_BEGIN("input_node [%p], input_user_data [%p]", input_node, input_user_data);
	int handle = 0;
	int i = 0;
	int err = EMAIL_ERROR_NONE;
	int account_count = 0;
	int sync_start_toggle = 0;
	email_account_t         *account_list = NULL;
	emstorage_mailbox_tbl_t *mailbox_tbl_data = NULL;

	if (!emdaemon_get_account_list(&account_list, &account_count, &err))  {
		EM_DEBUG_EXCEPTION("emdaemon_get_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if(input_node)
		sync_start_toggle = vconf_keynode_get_int(input_node);

	for(i = 0; i < account_count; i++) {
		if(sync_start_toggle == 1) {
			if(!emstorage_get_mailbox_by_mailbox_type(account_list[i].account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox_tbl_data, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type for [%d] failed [%d]", account_list[i].account_id, err);
				continue;
			}

			if(!emdaemon_sync_header(account_list[i].account_id, mailbox_tbl_data->mailbox_id, &handle, &err)) {
				EM_DEBUG_EXCEPTION("emdaemon_sync_header for [%d] failed [%d]", account_list[i].account_id, err);
			}
			if(mailbox_tbl_data)
				emstorage_free_mailbox(&mailbox_tbl_data, 1, NULL);
			mailbox_tbl_data = NULL;
		}
		else {
			emcore_cancel_all_threads_of_an_account(account_list[i].account_id);
		}
	}

FINISH_OFF:
	if(account_list)
		emdaemon_free_account(&account_list, account_count, NULL);
	if(mailbox_tbl_data)
		emstorage_free_mailbox(&mailbox_tbl_data, 1, NULL);

	EM_DEBUG_FUNC_END();
}

static void callback_for_AUTO_SYNC_STATUS_from_account_svc(keynode_t *input_node, void *input_user_data)
{
	EM_DEBUG_FUNC_BEGIN("input_node [%p], input_user_data [%p]", input_node, input_user_data);
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int auto_sync_toggle = 0;
	int account_count = 0;
	email_account_t *account_list = NULL;
	email_account_t *account_info = NULL;

	if (!emdaemon_get_account_list(&account_list, &account_count, &err))  {
		EM_DEBUG_EXCEPTION("emdaemon_get_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if(input_node)
		auto_sync_toggle = vconf_keynode_get_int(input_node);

	for(i = 0; i < account_count; i++) {
		account_info = account_list + i;

		if(auto_sync_toggle == 1) { /* on */
			/* start sync */
			if(account_info->check_interval < 0)
				account_info->check_interval = ~account_info->check_interval + 1;
		}
		else { /* off */
			/* terminate sync */
			if(account_info->check_interval > 0)
				account_info->check_interval = ~account_info->check_interval + 1;
		}

		if(!emdaemon_update_account(account_info->account_id, account_info, &err)) {
			EM_DEBUG_EXCEPTION("emdaemon_update_account failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	if(account_list)
		emdaemon_free_account(&account_list, account_count, NULL);

	EM_DEBUG_FUNC_END();
}

static void callback_for_NETWORK_STATUS(keynode_t *input_node, void *input_user_data)
{
	EM_DEBUG_FUNC_BEGIN("input_node [%p], input_user_data [%p]", input_node, input_user_data);
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int total = 0;
	int unseen = 0;
	int network_status = 0;
	int account_count = 0;
	email_account_t *account_list = NULL;
	email_account_t *account_info = NULL;
	emstorage_mailbox_tbl_t *local_mailbox = NULL;

	if (input_node)
		network_status = vconf_keynode_get_int(input_node);

	if (network_status == VCONFKEY_NETWORK_OFF) {
		EM_DEBUG_EXCEPTION("Network is OFF");
		goto FINISH_OFF;
	}

	if (!emdaemon_get_account_list(&account_list, &account_count, &err))  {
		EM_DEBUG_EXCEPTION("emdaemon_get_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	for (i = 0; i < account_count ; i++) {
		account_info = account_list + i;

		if (!emstorage_get_mailbox_by_mailbox_type(account_info->account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &local_mailbox, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_get_mail_count(account_info->account_id, local_mailbox->mailbox_name, &total, &unseen, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_mail_count failed [%d]", err);
			goto FINISH_OFF;
		}

		if (total <= 0)
			continue;

		if (!emcore_send_saved_mail(account_info->account_id, local_mailbox->mailbox_name, &err)) {
			EM_DEBUG_EXCEPTION("emcore_send_saved_mail failed [%d]", err);
			goto FINISH_OFF;
		}

		if (!emstorage_free_mailbox(&local_mailbox, 1, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_free_mailbox failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:

	if (local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

	if (account_list)
		emdaemon_free_account(&account_list, account_count, NULL);

	EM_DEBUG_FUNC_END("Error : [%d]", err);
}

INTERNAL_FUNC int emdaemon_initialize(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	
	if (g_client_count > 0)  {
		EM_DEBUG_LOG("Initialization was already done. increased counter=[%d]", g_client_count);
		g_client_count++;
		return true;
	}
	else 
		EM_DEBUG_LOG("************* start email service build time [%s %s] ************* ", __DATE__, __TIME__);

	dbus_threads_init_default();
	
	g_type_init();

	emstorage_shm_file_init(SHM_FILE_FOR_DB_LOCK);

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
	emstorage_shm_file_init(SHM_FILE_FOR_MAIL_ID_LOCK);
#endif /* __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

	/* open database */
	if (!emstorage_open(&err))  {
		EM_DEBUG_EXCEPTION("emstorage_open failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!emstorage_clean_save_status(EMAIL_MAIL_STATUS_SAVED, &err))
		EM_DEBUG_EXCEPTION("emstorage_check_mail_status Failed [%d]", err );
	
	g_client_count = 0;    
	
	if (!emdaemon_initialize_account_reference())  {
		EM_DEBUG_EXCEPTION("emdaemon_initialize_account_reference fail...");
		err = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}
    EM_DEBUG_LOG("emdaemon_initialize_account_reference over - g_client_count [%d]", g_client_count);	
	
	if ((err = _emdaemon_load_email_core()) != EMAIL_ERROR_NONE)  {
		EM_DEBUG_EXCEPTION("_emdaemon_load_email_core failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Subscribe Events */
	vconf_notify_key_changed(VCONFKEY_ACCOUNT_SYNC_ALL_STATUS_INT,  callback_for_SYNC_ALL_STATUS_from_account_svc,  NULL);
	vconf_notify_key_changed(VCONFKEY_ACCOUNT_AUTO_SYNC_STATUS_INT, callback_for_AUTO_SYNC_STATUS_from_account_svc, NULL);
	vconf_notify_key_changed(VCONFKEY_NETWORK_STATUS, callback_for_NETWORK_STATUS, NULL);

	emcore_display_unread_in_badge();
	
	ret = true;
	
FINISH_OFF:
	if (ret == true)
		g_client_count = 1;

	if (err_code)
		*err_code = err;
	
	EM_DEBUG_FUNC_END("ret [%d], g_client_count [%d]", ret, g_client_count);
	return ret;
}

INTERNAL_FUNC int emdaemon_finalize(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	
	if (g_client_count > 1) {
		EM_DEBUG_EXCEPTION("engine is still used by application. decreased counter=[%d]", g_client_count);
		g_client_count--;
		err = EMAIL_ERROR_CLOSE_FAILURE;
		goto FINISH_OFF;
	}
	
	if ( (err = _emdaemon_unload_email_core()) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_emdaemon_unload_email_core failed [%d]", err);
		goto FINISH_OFF;
	}
	
	/* free account reference list */
	emdaemon_free_account_reference();
	
	/* close database */
	if (!emstorage_close(&err)) {
		EM_DEBUG_EXCEPTION("emstorage_close failed [%d]", err);
		goto FINISH_OFF;
	}
	
	g_client_count = 0;
	
#ifdef __FEATURE_AUTO_POLLING__
	emdaemon_free_account_alarm_binder_list();
#endif

	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	
	return ret;
}

#ifdef __FEATURE_AUTO_POLLING__
INTERNAL_FUNC int emdaemon_start_auto_polling(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	/*  default variable */
	int ret = false, count = 0, i= 0;
	int err = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t* account_list = NULL;

	/* get account list */
	if (!emstorage_get_account_list(&count, &account_list, false, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", err);		
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++)  {
		/* start auto polling, if check_interval not zero */
		if(account_list[i].check_interval > 0) {
			if(!emdaemon_add_polling_alarm( account_list[i].account_id,account_list[i].check_interval))
				EM_DEBUG_EXCEPTION("emdaemon_add_polling_alarm failed");
		}
	}

	ret = true;
FINISH_OFF:  
	if (account_list)
		emstorage_free_account(&account_list, count, NULL);
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}
#endif /* __FEATURE_AUTO_POLLING__ */

