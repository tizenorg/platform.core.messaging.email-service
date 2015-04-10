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

#ifndef __EMAIL_DBUS_ACTIVATION_H__
#define __EMAIL_DBUS_ACTIVATION_H__

#include <gio/gio.h>

#define EMAIL_SERVICE_NAME "org.tizen.email_service"
#define EMAIL_SERVICE_PATH "/org/tizen/email_service"

/* 
note: it is not safe freeing the return-val
*/
#define EM_GDBUS_STR_NEW(str)    (str? str:"")
/* input string is released in case of dummy val */
#define EM_GDBUS_STR_GET(str)    \
	({\
		char* ret;\
		if (g_strcmp0(str, "")) {\
			ret = str;\
		}\
		else {\
			g_free(str);\
			str = NULL;\
			ret = NULL;\
		}\
		ret;\
	})


EXPORT_API int emipc_launch_email_service();
EXPORT_API GVariant* em_gdbus_set_contact_log (GVariant *parameters);
EXPORT_API GVariant* em_gdbus_delete_contact_log (GVariant *parameters);
EXPORT_API GVariant* em_gdbus_get_display_name (GVariant *parameters);
EXPORT_API GVariant* em_gdbus_check_blocking_mode (GVariant *parameters);
#endif /* __EMAIL_DBUS_ACTIVATION_H__ */

