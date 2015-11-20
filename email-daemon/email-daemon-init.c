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
#include <dlfcn.h>				/* added for Disabling the Pthread flag log */
#include <net_connection.h>

#include "email-daemon.h"
#include "email-storage.h"
#include "email-debug-log.h"
#include "email-daemon-init.h"
#include "email-daemon-account.h"
#include "email-daemon-auto-poll.h"
#include "email-daemon-event.h"
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-event.h"
#include "email-core-account.h"
#include "email-core-mailbox.h"
#include "email-core-smtp.h"
#include "email-core-global.h"
#include "email-core-imap-idle.h"
#include "email-storage.h"
#include "email-core-task-manager.h"
#include "email-core-alarm.h"
#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
#include "email-core-auto-download.h"
#endif
#include "email-daemon-emn.h"
#include "email-network.h"
#include "email-device.h"
#include "email-core-cynara.h"
#include "c-client.h"
#include "email-core-smime.h"
#include "email-core-container.h"

connection_h conn = NULL;

extern void *
pop3_parameters(long function, void *value);
extern void *
imap_parameters(long function, void *value);

INTERNAL_FUNC int g_client_count = 0;
extern int blocking_mode_of_setting;
extern GList *alarm_data_list;

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
extern pthread_cond_t  _auto_downalod_available_signal;
#endif

static int default_alarm_callback(int input_timer_id, void *user_parameter);

/*  static functions */
static int _emdaemon_load_email_core()
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;

	/* initialize mail core */
	if (!emdaemon_core_init(&err))
		goto FINISH_OFF;

#ifdef __FEATURE_OPEN_SSL_MULTIHREAD_HANDLE__
	emdaemon_setup_handler_for_open_ssl_multithread();
#endif /* __FEATURE_OPEN_SSL_MULTIHREAD_HANDLE__ */

	if (emdaemon_start_event_loop(&err) < 0)
		goto FINISH_OFF;

	if (emdaemon_start_event_loop_for_sending_mails(&err) < 0)
		goto FINISH_OFF;

	emcore_init_task_handler_array();

	/* Disabled task manager
	if ((err = emcore_start_task_manager_loop()) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_start_task_manager_loop failed [%d]",err);
		goto FINISH_OFF;
	}
	*/

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
	if (emdaemon_start_thread_for_downloading_partial_body(&err) < 0) {
		EM_DEBUG_EXCEPTION("emcore_start_thread_for_downloading_partial_body failed [%d]",err);
		goto FINISH_OFF;
	}
#endif

#ifdef __FEATURE_BLOCKING_MODE__
	emdaemon_init_blocking_mode_status();
#endif /* __FEATURE_BLOCKING_MODE__ */

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
	if (!emcore_start_auto_download_loop(&err))
		goto FINISH_OFF;
#endif

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int _emdaemon_unload_email_core()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	/* finish event loop */
	emcore_stop_event_loop(&err);
	emcore_stop_task_manager_loop();

    /* Destroy container for daemon */
    emcore_destroy_container();

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
	emcore_stop_auto_download_loop(&err);
#endif

#ifdef __FEATURE_OPEN_SSL_MULTIHREAD_HANDLE__
	emdaemon_cleanup_handler_for_open_ssl_multithread();
#endif /* __FEATURE_OPEN_SSL_MULTIHREAD_HANDLE__ */

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

#ifdef __FEATURE_SYNC_STATUS__
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

	if (!emdaemon_get_account_list(NULL, &account_list, &account_count, &err))  {
		EM_DEBUG_EXCEPTION("emdaemon_get_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	if(input_node)
		sync_start_toggle = vconf_keynode_get_int(input_node);

	for(i = 0; i < account_count; i++) {
		if(sync_start_toggle == 1) {
			if(!emstorage_get_mailbox_by_mailbox_type(NULL, account_list[i].account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox_tbl_data, true, &err)) {
				EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type for [%d] failed [%d]", account_list[i].account_id, err);
				continue;
			}

			if(!emdaemon_sync_header(NULL, account_list[i].account_id, mailbox_tbl_data->mailbox_id, &handle, &err)) {
				EM_DEBUG_EXCEPTION("emdaemon_sync_header for [%d] failed [%d]", account_list[i].account_id, err);
			}

			emstorage_free_mailbox(&mailbox_tbl_data, 1, NULL); /* prevent 27459: remove unnecessary if clause */
			mailbox_tbl_data = NULL;
		}
		else {
			emcore_cancel_all_threads_of_an_account(NULL, account_list[i].account_id);
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

	if (!emdaemon_get_account_list(NULL, &account_list, &account_count, &err))  {
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

		if(!emdaemon_update_account(NULL, account_info->account_id, account_info, &err)) {
			EM_DEBUG_EXCEPTION("emdaemon_update_account failed [%d]", err);
			goto FINISH_OFF;
		}
	}

FINISH_OFF:
	if(account_list)
		emdaemon_free_account(&account_list, account_count, NULL);

	EM_DEBUG_FUNC_END();
}
#endif /* __FEATURE_SYNC_STATUS__ */

static void callback_for_NETWORK_STATUS(connection_type_e new_conn_type, void *input_user_data)
{
	EM_DEBUG_FUNC_BEGIN("new_conn_type [%p], input_user_data [%p]", new_conn_type, input_user_data);
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int total = 0;
	int unseen = 0;
	int account_count = 0;
	int conn_type = 0;
	int handle = 0;
	int send_saved_handle = 0;
	char *multi_user_name = NULL;
	email_event_t *event_data = NULL;
	email_account_t *account_list = NULL;
	email_account_t *account_info = NULL;
	emstorage_mailbox_tbl_t *local_mailbox = NULL;

	conn_type = emnetwork_get_network_status();

	switch (new_conn_type) {
		case CONNECTION_TYPE_WIFI:
		case CONNECTION_TYPE_CELLULAR:
			if(conn_type != new_conn_type) {
				EM_DEBUG_LOG("Network type changed from [%d] to [%d]", conn_type, new_conn_type);
				emnetwork_set_network_status(new_conn_type);
			}
			break;

		case CONNECTION_TYPE_DISCONNECTED:
			EM_DEBUG_LOG("Network is disconnected. [%d]", new_conn_type);
			emnetwork_set_network_status(new_conn_type);
			goto FINISH_OFF;

		default:
			EM_DEBUG_LOG("Unexpected value came. Set disconnectted network. [%d]", new_conn_type);
			emnetwork_set_network_status(CONNECTION_TYPE_DISCONNECTED);
			goto FINISH_OFF;
	}

	emcore_get_sync_fail_event_data(&event_data);

	if (event_data != NULL) {
		if (!emcore_insert_event(event_data, &handle, &err)) {
			EM_DEBUG_LOG("emcore_insert_event failed : %d", err);
		}
	}

	if (false == emcore_is_partial_body_thd_que_empty())
		emcore_pb_thd_set_local_activity_continue(true);

	if (!emdaemon_get_account_list(NULL, &account_list, &account_count, &err))  {
		EM_DEBUG_EXCEPTION("emdaemon_get_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	for (i = 0; i < account_count ; i++) {
		account_info = account_list + i;
		/* check if inbox folder sync is finished */
		if (!emstorage_get_mailbox_by_mailbox_type (multi_user_name, account_info->account_id, EMAIL_MAILBOX_TYPE_INBOX, &local_mailbox, false, &err)) {
			if (err == EMAIL_ERROR_MAILBOX_NOT_FOUND) {
				int handle = 0;
				emdaemon_get_imap_mailbox_list(multi_user_name, account_info->account_id, "", &handle, &err);
				if (err != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emdaemon_get_imap_mailbox_list error [%d]", err);
				}
			}
		}

		if (!emstorage_free_mailbox(&local_mailbox, 1, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_free_mailbox error [%d]", err);
		}

		/* send mails in outbox */
		if (!emcore_get_mail_count_by_query(NULL, account_info->account_id, EMAIL_MAILBOX_TYPE_OUTBOX, 0, &total, &unseen, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_mail_count_by_query failed [%d]", err);
		}

		if (total <= 0)
			continue;

		if (!emdaemon_send_mail_saved(multi_user_name, account_info->account_id, &send_saved_handle, &err)) {
			EM_DEBUG_EXCEPTION("emdaemon_send_mail_saved failed : [%d]", err);
		}
	}

FINISH_OFF:

	if (local_mailbox)
		emstorage_free_mailbox(&local_mailbox, 1, NULL);

	if (account_list)
		emdaemon_free_account(&account_list, account_count, NULL);

	EM_DEBUG_FUNC_END("Error : [%d]", err);
}

#ifdef __FEATURE_BLOCKING_MODE__
INTERNAL_FUNC void callback_for_BLOCKING_MODE_STATUS(keynode_t *input_node, void *input_user_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int blocking_mode_of_setting = 0;

	if (!input_node) {
		EM_DEBUG_EXCEPTION("Invalid param");
		return;
	}

	blocking_mode_of_setting = vconf_keynode_get_bool(input_node);

	emcore_set_blocking_mode_of_setting(blocking_mode_of_setting);

	EM_DEBUG_FUNC_END();
}
#endif /* __FEATURE_BLOCKING_MODE__ */

#ifdef __FEATURE_OMA_EMN__
static void callback_for_VCONFKEY_MSG_SERVER_READY(keynode_t *input_node, void *input_user_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int msg_server_ready = 0;

	if (!input_node) {
		EM_DEBUG_EXCEPTION("Invalid param");
		return;
	}

	msg_server_ready = vconf_keynode_get_bool(input_node);

	if(msg_server_ready) {
		if(emdaemon_initialize_emn() != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emdaemon_initialize_emn failed");
		}
	}

	return;
}
#endif /* __FEATURE_OMA_EMN__ */

static void callback_for_VCONFKEY_GLOBAL_BADGE_STATUS(keynode_t *input_node, void *input_user_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int noti_status = 0;
	int err = EMAIL_ERROR_NONE;
	int badge_ticker = 0;

	if (!input_node) {
		EM_DEBUG_EXCEPTION("Invalid param");
		return;
	}

	noti_status = vconf_keynode_get_bool(input_node);

	if (noti_status) {
		emcore_display_unread_in_badge((char *)input_user_data);
	} else {
		if (vconf_get_bool(VCONF_VIP_NOTI_BADGE_TICKER, &badge_ticker) != 0) {
			EM_DEBUG_EXCEPTION("vconf_get_bool failed");
			err = EMAIL_ERROR_GCONF_FAILURE;
			goto FINISH_OFF;
		}
		/* if priority sender is on, show the priority sender unread */
		if (badge_ticker) {
			emcore_display_unread_in_badge((char *)input_user_data);
			goto FINISH_OFF;
		}

		/* reset badge */
		if ((err = emcore_display_badge_count((char *)input_user_data, 0)) != EMAIL_ERROR_NONE)
			EM_DEBUG_EXCEPTION("emcore_display_badge_count failed : [%d]", err);
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END();
}


static void callback_for_VCONFKEY_PRIORITY_BADGE_STATUS(keynode_t *input_node, void *input_user_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int noti_status = 0;
	int err = EMAIL_ERROR_NONE;
	int badge_ticker = 0;

	if (!input_node) {
		EM_DEBUG_EXCEPTION("Invalid param");
		return;
	}

	noti_status = vconf_keynode_get_bool(input_node);

	if (noti_status) {
		emcore_display_unread_in_badge((char *)input_user_data);
	} else {

		if (vconf_get_bool(VCONFKEY_TICKER_NOTI_BADGE_EMAIL, &badge_ticker) != 0) {
			EM_DEBUG_EXCEPTION("vconf_get_bool failed");
			err = EMAIL_ERROR_GCONF_FAILURE;
			goto FINISH_OFF;
		}

		/*if global badge is on, show the global unread*/
		if (badge_ticker) {
			emcore_display_unread_in_badge((char *)input_user_data);
			goto FINISH_OFF;
		}
		/* if all badges are off, reset badge count */
		if ((err = emcore_display_badge_count((char *)input_user_data, 0)) != EMAIL_ERROR_NONE)
			EM_DEBUG_EXCEPTION("emcore_display_badge_count failed : [%d]", err);
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END();
}

#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
static void callback_for_VCONFKEY_WIFI_STATE(keynode_t *input_node, void *input_user_data)
{
	EM_DEBUG_FUNC_BEGIN();
	int wifi_status = 0;

	if (!input_node) {
		EM_DEBUG_EXCEPTION("Invalid param");
		return;
	}

	wifi_status = vconf_keynode_get_int(input_node);

	EM_DEBUG_LOG("wifi_status [%d]", wifi_status);

	if (wifi_status > 1) {
		if (!emcore_get_pbd_thd_state() &&
				emcore_is_event_queue_empty() &&
				emcore_is_send_event_queue_empty()) {
			WAKE_CONDITION_VARIABLE(_auto_downalod_available_signal);
		}
	}

	EM_DEBUG_FUNC_END();
}
#endif

INTERNAL_FUNC int emdaemon_initialize(char *multi_user_name, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	connection_type_e net_state = CONNECTION_TYPE_DISCONNECTED;

	if (!g_client_count) {
		EM_DEBUG_LOG("Initialization was already done. increased counter=[%d]", g_client_count);

		EM_DEBUG_LOG("************* start email service build time [%s %s] ************* ", __DATE__, __TIME__);
		err = emstorage_init_db(multi_user_name);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("DB create failed");
			goto FINISH_OFF;
		}

        dbus_threads_init_default();

#if !GLIB_CHECK_VERSION(2, 36, 0)
		g_type_init();
#endif
        if ((err = _emdaemon_load_email_core()) != EMAIL_ERROR_NONE)  {
            EM_DEBUG_EXCEPTION("_emdaemon_load_email_core failed [%d]", err);
            goto FINISH_OFF;
        }

		err = emcore_init_cynara();
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_init_cynara failed : [%d]", err);
			goto FINISH_OFF;
		}

#ifdef __FEATURE_OMA_EMN__
        if(emdaemon_initialize_emn() != EMAIL_ERROR_NONE) {
            vconf_notify_key_changed(VCONFKEY_MSG_SERVER_READY, callback_for_VCONFKEY_MSG_SERVER_READY, NULL);
        }
#endif

    }

    /* open database */
	if (!emstorage_open(multi_user_name, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_open failed [%d]", err);
		goto FINISH_OFF;
	}

	if ((err = emstorage_update_db_table_schema(multi_user_name)) != EMAIL_ERROR_NONE)
		EM_DEBUG_EXCEPTION("emstorage_update_db_table_schema failed [%d]", err);

	if (!emstorage_clean_save_status(multi_user_name, EMAIL_MAIL_STATUS_SAVED, &err))
		EM_DEBUG_EXCEPTION("emstorage_check_mail_status Failed [%d]", err );

#ifdef __FEATURE_AUTO_RETRY_SEND__
	if ((err = emcore_create_alarm_for_auto_resend (multi_user_name, AUTO_RESEND_INTERVAL)) != EMAIL_ERROR_NONE) {
		if (err == EMAIL_ERROR_MAIL_NOT_FOUND)
			EM_DEBUG_LOG ("no mail found");
		else
			EM_DEBUG_EXCEPTION("emcore_create_alarm_for_auto_resend failed [%d]", err);
	}
#endif /* __FEATURE_AUTO_RETRY_SEND__ */


	/* Start auto polling */
#ifdef __FEATURE_AUTO_POLLING__
	emdaemon_start_auto_polling(multi_user_name, &err);
#endif

#ifdef __FEATURE_IMAP_IDLE__
	if (!g_client_count)
		emcore_create_imap_idle_thread();
#endif /* __FEATURE_IMAP_IDLE__ */

	/* Subscribe Events */
#ifdef __FEATURE_SYNC_STATUS__
	vconf_notify_key_changed(VCONFKEY_ACCOUNT_SYNC_ALL_STATUS_INT,  callback_for_SYNC_ALL_STATUS_from_account_svc,  NULL);
	vconf_notify_key_changed(VCONFKEY_ACCOUNT_AUTO_SYNC_STATUS_INT, callback_for_AUTO_SYNC_STATUS_from_account_svc, NULL);
#endif /* __FEATURE_SYNC_STATUS__  */

	int error_from_connection = 0;

	if ((error_from_connection = connection_create(&conn)) == CONNECTION_ERROR_NONE) {
		if (connection_get_type(conn, &net_state) != CONNECTION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("connection_get_type failed");
		} else {
			emnetwork_set_network_status(net_state);
		}

		if (connection_set_type_changed_cb(conn, callback_for_NETWORK_STATUS, NULL) != CONNECTION_ERROR_NONE)
			EM_DEBUG_EXCEPTION("connection_set_type_changed_cb failed");
	}
	else {
		EM_DEBUG_EXCEPTION("connection_create failed[%d]", error_from_connection);
	}

#ifdef __FEATURE_BLOCKING_MODE__
	vconf_notify_key_changed(VCONFKEY_SETAPPL_BLOCKINGMODE_NOTIFICATIONS, callback_for_BLOCKING_MODE_STATUS, NULL);
#endif
	vconf_notify_key_changed(VCONFKEY_TICKER_NOTI_BADGE_EMAIL, callback_for_VCONFKEY_GLOBAL_BADGE_STATUS, multi_user_name);
	vconf_notify_key_changed(VCONF_VIP_NOTI_BADGE_TICKER, callback_for_VCONFKEY_PRIORITY_BADGE_STATUS, multi_user_name);
	vconf_notify_key_changed(VCONFKEY_SETAPPL_STATE_TICKER_NOTI_EMAIL_BOOL, callback_for_VCONFKEY_GLOBAL_BADGE_STATUS, multi_user_name);
	vconf_notify_key_changed(VCONF_VIP_NOTI_NOTIFICATION_TICKER, callback_for_VCONFKEY_PRIORITY_BADGE_STATUS, multi_user_name);
#ifdef __FEATURE_WIFI_AUTO_DOWNLOAD__
	vconf_notify_key_changed(VCONFKEY_WIFI_STATE, callback_for_VCONFKEY_WIFI_STATE, NULL);
#endif

	emcore_display_unread_in_badge(multi_user_name);

	ret = true;
	g_client_count++;

FINISH_OFF:

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

	if ( (err = _emdaemon_unload_email_core()) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_emdaemon_unload_email_core failed [%d]", err);
		goto FINISH_OFF;
	}

	connection_destroy(conn);

	/* Finish cynara */
	emcore_finish_cynara();

	/* close database */
	if (!emstorage_close(&err)) {
		EM_DEBUG_EXCEPTION("emstorage_close failed [%d]", err);
		goto FINISH_OFF;
	}

        /* Openssl clean up */
        emcore_clean_openssl_library();

	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;

	return ret;
}

#ifdef __FEATURE_AUTO_POLLING__
INTERNAL_FUNC int emdaemon_start_auto_polling(char *multi_user_name, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	/*  default variable */
	int ret = false, count = 0, i= 0;
	int err = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t* account_list = NULL;

	/* get account list */
	if (!emstorage_get_account_list(multi_user_name, &count, &account_list, false, false, &err))  {
		EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [%d]", err);
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++)  {
		/* start auto polling, if check_interval not zero */
		if(account_list[i].check_interval > 0 || ((account_list[i].peak_days > 0) && account_list[i].peak_interval > 0)) {
			if(!emdaemon_add_polling_alarm(multi_user_name, account_list[i].account_id))
				EM_DEBUG_EXCEPTION("emdaemon_add_polling_alarm failed");
		}
	}

	ret = true;
FINISH_OFF:
	if (account_list)
		emstorage_free_account(&account_list, count, NULL);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("ret[%d]", ret);
	return ret;
}
#endif /* __FEATURE_AUTO_POLLING__ */

/* initialize mail core */
INTERNAL_FUNC int emdaemon_core_init(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if (err_code != NULL) {
		*err_code = EMAIL_ERROR_NONE;
	}

	mail_link(&imapdriver);    /*  link in the imap driver  */
	mail_link(&pop3driver);    /*  link in the pop3 driver  */

	mail_link(&unixdriver);	   /*  link in the unix driver  */
	mail_link(&dummydriver);   /*  link in the dummy driver  */

	ssl_onceonlyinit();

	auth_link(&auth_xoauth2);  /*  link in the xoauth2 authenticator  */
	auth_link(&auth_md5);      /*  link in the md5 authenticator  */
	auth_link(&auth_pla);      /*  link in the pla authenticator  */
	auth_link(&auth_log);      /*  link in the log authenticator  */

	/* Disabled to authenticate with plain text */
	mail_parameters(NIL, SET_DISABLEPLAINTEXT, (void *) 2);

	/* Set max trials for login */
	imap_parameters(SET_MAXLOGINTRIALS, (void *)2); /* 3-> 2*/
	pop3_parameters(SET_MAXLOGINTRIALS, (void *)2); /* 3-> 2*/
	smtp_parameters(SET_MAXLOGINTRIALS, (void *)1); /* 3-> 2*/

	mail_parameters(NIL, SET_SSLCERTIFICATEQUERY, (void *)emnetwork_callback_ssl_cert_query);
	mail_parameters(NIL, SET_SSLCAPATH, (void *)SSL_CERT_DIRECTORY);

	/* Set time out in second */
	mail_parameters(NIL, SET_OPENTIMEOUT  , (void *)50);
	mail_parameters(NIL, SET_READTIMEOUT  , (void *)60); /* 180 -> 15 */
	mail_parameters(NIL, SET_WRITETIMEOUT , (void *)20); /* 180 -> 15 */
	mail_parameters(NIL, SET_CLOSETIMEOUT , (void *)20); /* 30 -> 15 */

	emdaemon_init_alarm_data_list();

	/* Openssl library init */
	emcore_init_openssl_library();

	if (err_code)
		*err_code = EMAIL_ERROR_NONE;

	return true;
}

INTERNAL_FUNC bool emdaemon_init_blocking_mode_status()
{
	EM_DEBUG_FUNC_BEGIN("blocking_mode_of_setting : [%d]", blocking_mode_of_setting);

#ifdef __FEATURE_BLOCKING_MODE__
	if (vconf_get_bool(VCONFKEY_SETAPPL_BLOCKINGMODE_NOTIFICATIONS, &blocking_mode_of_setting) != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_bool failed");
		return false;
	}
#endif /* __FEATURE_BLOCKING_MODE__ */

	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emdaemon_init_alarm_data_list()
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = ALARMMGR_RESULT_SUCCESS;
	int err = EMAIL_ERROR_NONE;
	alarm_data_list = NULL;

	if ((ret = alarmmgr_init(EMAIL_ALARM_DESTINATION)) != ALARMMGR_RESULT_SUCCESS) {
		EM_DEBUG_EXCEPTION("alarmmgr_init failed [%d]",ret);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if ((ret = alarmmgr_set_cb(default_alarm_callback, NULL)) != ALARMMGR_RESULT_SUCCESS) {
		EM_DEBUG_EXCEPTION("alarmmgr_set_cb() failed [%d]", ret);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int default_alarm_callback(int input_timer_id, void *user_parameter)
{
	EM_DEBUG_FUNC_BEGIN("input_timer_id [%d] user_parameter [%p]", input_timer_id, user_parameter);
	int err = EMAIL_ERROR_NONE;
	email_alarm_data_t *alarm_data = NULL;

//	EM_DEBUG_ALARM_LOG("default_alarm_callback input_timer_id[%d]", input_timer_id);

	emdevice_set_sleep_on_off(STAY_AWAKE_FLAG_FOR_ALARM_CALLBACK, false, NULL);

	if ((err = emcore_get_alarm_data_by_alarm_id(input_timer_id, &alarm_data)) != EMAIL_ERROR_NONE || alarm_data == NULL) {
		EM_DEBUG_EXCEPTION("emcore_get_alarm_data_by_alarm_id failed [%d]", err);
		goto FINISH_OFF;
	}

	emcore_delete_alram_data_from_alarm_data_list(alarm_data);

	if ((err = alarm_data->alarm_callback(alarm_data, user_parameter)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("alarm_callback failed [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (alarm_data) {
		EM_SAFE_FREE(alarm_data->multi_user_name);
		EM_SAFE_FREE(alarm_data->user_data);
		EM_SAFE_FREE(alarm_data);
	}

	emdevice_set_sleep_on_off(STAY_AWAKE_FLAG_FOR_ALARM_CALLBACK, true, NULL);


	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

