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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#ifdef USE_POWERMGMT
#include <App-manager.h>
#endif

#include "em-storage.h"
#include "emf-dbglog.h"

#define EMAIL_SERVICE_GCONF_DIR  "/Services/email-service"
#define EMAIL_SERVICE_GCONF_MAIL_FULL "mail-full"


#ifdef BACKUP_LCD_DIMMING_STATUS
static GConfValue *g_gconf_backup_value = NULL;
#endif



#ifdef USE_POWERMGMT
EXPORT_API int em_storage_power_management(int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("err_code[%p]", error_code);
	int err		= EM_STORAGE_ERROR_NONE;
	int inst_id	= 0;

	app_get_current_inst_id(&inst_id, &err);

	if (AppMgrAppCommand(inst_id, APP_COMMAND_POWER_MGR, APPMGR_SET_PWRMGR_SYS_RUN_STATUS) < 0) {
		EM_DEBUG_EXCEPTION("APPMGR_SET_PWRMGR_SYS_RUN_STATUS Failed");
		return false;
	}
	EM_DEBUG_FUNC_END("APPMGR_SET_PWRMGR_SYS_RUN_STATUS Success");
	

	return true;
}
#endif

EXPORT_API int em_storage_sleep_on_off(int on, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN();
	return true;
}

EXPORT_API int em_storage_dimming_on_off(int on, int *error_code)
{
	EM_DEBUG_FUNC_BEGIN("on[%d], err_code[%p]", on, error_code);

	em_storage_sleep_on_off(on, error_code);
	
	return true;
}
