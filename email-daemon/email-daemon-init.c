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
#include "email-core-global.h"
#include "email-storage.h"   
#include "email-core-sound.h" 

extern int g_client_count ;

/*  static functions */
static int _emdaemon_load_email_core()
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMF_ERROR_NONE;

	/* initialize mail core */
	if (!emcore_init(&err))
		goto FINISH_OFF;

	if (emcore_start_event_loop(&err) < 0)
		goto FINISH_OFF;

	if (emcore_send_event_loop_start(&err) < 0)
		goto FINISH_OFF;

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
	if (emcore_partial_body_thread_loop_start(&err) < 0) {
		EM_DEBUG_EXCEPTION("emcore_partial_body_thread_loop_start failed [%d]",err);
		goto FINISH_OFF;
	}
#endif
	if (emcore_start_alert_thread(&err) < 0)
		goto FINISH_OFF;

FINISH_OFF:

	return err;
}

static int _emdaemon_unload_email_core()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMF_ERROR_NONE;

	/* finish event loop */
	emcore_stop_event_loop(&err);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emdaemon_initialize(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
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

	if (!emstorage_clean_save_status(EMF_MAIL_STATUS_SAVED, &err))
		EM_DEBUG_EXCEPTION("emstorage_check_mail_status Failed [%d]", err );
	
	g_client_count = 0;    
	
	if (!emdaemon_initialize_account_reference())  {
		EM_DEBUG_EXCEPTION("emdaemon_initialize_account_reference fail...");
		err = EMF_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}
    EM_DEBUG_LOG("emdaemon_initialize_account_reference over - g_client_count [%d]", g_client_count);	
	
	if ((err = _emdaemon_load_email_core()) != EMF_ERROR_NONE)  {
		EM_DEBUG_EXCEPTION("_emdaemon_load_email_core failed [%d]", err);
		goto FINISH_OFF;
	}

	emcore_check_unread_mail(); 
	
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
	int err = EMF_ERROR_NONE;
	
	if (g_client_count > 1) {
		EM_DEBUG_EXCEPTION("engine is still used by application. decreased counter=[%d]", g_client_count);
		g_client_count--;
		err = EMF_ERROR_CLOSE_FAILURE;
		goto FINISH_OFF;
	}
	
	if ( (err = _emdaemon_unload_email_core()) != EMF_ERROR_NONE) {
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
	int err = EMF_ERROR_NONE;
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

