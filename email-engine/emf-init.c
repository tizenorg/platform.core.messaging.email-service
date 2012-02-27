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
 * File: emf-init.c
 * Desc: Mail Framework Initialization
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

#include "emflib.h"
#include "em-storage.h"
#include "emf-global.h"
#include "emf-dbglog.h"
#include "emf-account.h"
#include "emf-auto-poll.h"   
#include "em-core-utils.h"
#include "em-core-mesg.h"
#include "em-core-event.h" 
#include "em-core-account.h"    
#include "em-core-mailbox.h"    
#include "em-core-api.h"    
#include "em-core-global.h"
#include "em-storage.h"   
#include "em-core-sound.h" 


/* added for Disabling the Pthread flag log */

#ifdef API_TEST
#include "emf-api-test.h"
#endif

#ifdef _CONTACT_SUBSCRIBE_CHANGE_
#include <contacts-svc.h>
#endif

#include <dlfcn.h>
#define ENGINE_PATH_EM_CORE	"libemail-core.so"

extern int g_client_count ;
extern int g_client_run;

/*  static functions */
static int _emf_load_engine(emf_engine_type_t type, int* err_code);
static int _emf_unload_engine(emf_engine_type_t type, int* err_code);

#ifdef _CONTACT_SUBSCRIBE_CHANGE_
static void _emf_contact_change_cb(void *data)
{
	EM_DEBUG_FUNC_BEGIN();
	/*  Run new thread and contact sync */
	EM_DEBUG_LOG("Contact Changed.");
	em_core_contact_sync_handler();
}
#endif

EXPORT_API int emf_init(int* err_code)
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

#ifdef __FEATURE_USE_PTHREAD__
	dbus_threads_init_default();
#else	/*  __FEATURE_USE_PTHREAD__ */
	if (!g_thread_supported())
		g_thread_init(NULL);
	dbus_g_thread_init ();
#endif	/*  __FEATURE_USE_PTHREAD__ */
	
	g_type_init();


	em_storage_shm_file_init(SHM_FILE_FOR_DB_LOCK);

#ifdef __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__
	em_storage_shm_file_init(SHM_FILE_FOR_MAIL_ID_LOCK);
#endif /* __FEATURE_USE_SHARED_MUTEX_FOR_GENERATING_MAIL_ID__ */

	/* open database */
	if (!em_storage_open(&err))  {
		EM_DEBUG_EXCEPTION("em_storage_open failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!em_storage_clean_save_status(EMF_MAIL_STATUS_SAVED, &err))
		EM_DEBUG_EXCEPTION("em_storage_check_mail_status Failed [%d]", err );
	
	g_client_count = 0;    
	
	if (!emf_init_account_reference())  {
		EM_DEBUG_EXCEPTION("emf_init_account_reference fail...");
		err = EMF_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}
    EM_DEBUG_LOG("emf_init_account_reference over - g_client_count [%d]", g_client_count);	
	
	if (!_emf_load_engine(EMF_ENGINE_TYPE_EM_CORE, &err))  {
		EM_DEBUG_EXCEPTION("_emf_load_engine failed [%d]", err);
		
		/* err = EMF_ERROR_DB_FAILURE; */
		goto FINISH_OFF;
	}

	em_core_check_unread_mail(); 
	
#ifdef _CONTACT_SUBSCRIBE_CHANGE_
	em_core_init_last_sync_time();
	contacts_svc_subscribe_change(CTS_SUBSCRIBE_CONTACT_CHANGE, _emf_contact_change_cb, NULL);
#endif
	
	ret = true;
	
FINISH_OFF:
	if (ret == true)
		g_client_count = 1;

	EM_DEBUG_LOG(">>>>>>>>> ret value : %d g_client_count [%d]", ret, g_client_count);	
#ifdef API_TEST
	emf_api_test_send_mail();
#endif
	
	if (err_code)
		*err_code = err;
	
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API int emf_close(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	/*  default variable */
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	if (g_client_count > 1) {
		EM_DEBUG_EXCEPTION("engine is still used by application. decreased counter=[%d]", g_client_count);
		
		g_client_count--;
		
		/* err = EMF_ERROR_CLOSE_FAILURE; */
		goto FINISH_OFF;
	}
	
	if (!_emf_unload_engine(EMF_ENGINE_TYPE_EM_CORE, &err)) {
		EM_DEBUG_EXCEPTION("_emf_unload_engine failed [%d]", err);
		goto FINISH_OFF;
	}
	
	/* free account reference list */
	emf_free_account_reference();
	
	/* close database */
	if (!em_storage_close(&err)) {
		EM_DEBUG_EXCEPTION("em_storage_close failed [%d]", err);
		goto FINISH_OFF;
	}
	
	g_client_count = 0;
	
#ifdef _CONTACT_SUBSCRIBE_CHANGE_
	contacts_svc_unsubscribe_change(CTS_SUBSCRIBE_CONTACT_CHANGE, _emf_contact_change_cb);
#endif
	
#ifdef __FEATURE_AUTO_POLLING__
	emf_free_account_alarm_binder_list();
#endif

	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	
	return ret;
}

static int _emf_load_engine(emf_engine_type_t type, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int err = EMF_ERROR_NONE;
	int ret = false;
	
	switch (type)  {
		case 1:
			/* initialize mail core */
			if (!em_core_init(&err)) 
				goto FINISH_OFF;
			
			if (em_core_event_loop_start(&err) < 0) 
				goto FINISH_OFF;

			if (em_core_send_event_loop_start(&err) < 0) 
				goto FINISH_OFF;

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
			if (em_core_partial_body_thread_loop_start(&err) < 0) {
				EM_DEBUG_EXCEPTION("em_core_partial_body_thread_loop_start failed [%d]",err);
				goto FINISH_OFF;
			}
#endif
/*
			if ( em_core_open_contact_db_library() == false )
				goto FINISH_OFF;
*/
			if (em_core_alert_loop_start(&err) < 0)		
				goto FINISH_OFF;
		
			break;
		
		default:
			return false;
	}
	
	ret = true;
	
FINISH_OFF:
	if (err_code)
		*err_code = err;
	
	return ret;
}

static int _emf_unload_engine(emf_engine_type_t type, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMF_ERROR_NONE;
	
	switch (type)  {
		case EMF_ENGINE_TYPE_EM_CORE:
				/* finish event loop */
				em_core_event_loop_stop(&err);
/*				
			em_core_close_contact_db_library();
*/			
			ret = true;
			break;
			
		default:
			err = EMF_ERROR_NOT_SUPPORTED;
			break;
	}
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}

#ifdef __FEATURE_AUTO_POLLING__
EXPORT_API int emf_auto_polling(int* err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	/*  default variable */
	int ret = false, count = 0, i= 0;
	int err = EMF_ERROR_NONE;
	emf_mail_account_tbl_t* account_list = NULL;

	/* get account list */
	if (!em_storage_get_account_list(&count, &account_list, false, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_account_list failed [%d]", err);		
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++)  {
		/* start auto poll */
		/* start auto polling, if check_interval not zero */
		if(account_list[i].check_interval > 0) {
			if(!emf_add_polling_alarm( account_list[i].account_id,account_list[i].check_interval))
				EM_DEBUG_EXCEPTION("emf_add_polling_alarm[ NOTI_ACCOUNT_ADD] : start auto poll Failed >>> ");
		}
	}

	ret = true;
FINISH_OFF:  
	if (account_list)
		em_storage_free_account(&account_list, count, NULL);
	
	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}
#endif

