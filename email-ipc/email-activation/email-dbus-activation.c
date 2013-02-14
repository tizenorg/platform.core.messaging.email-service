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

#include <glib.h>
#include <pthread.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <sys/utsname.h>
#include "email-types.h"
#include "email-debug-log.h"
#include "email-dbus-activation.h"
#include "email-service-binding.h"
#include "email-service-glue.h"
#include <unistd.h>

GMainLoop *loop = NULL;
GObject *object = NULL;
DBusGConnection *connection = NULL;

static pid_t launch_by_client = 0;

void __net_nfc_discovery_polling_cb(int signo);
bool Check_Redwood();

G_DEFINE_TYPE(EmailService, email_service, G_TYPE_OBJECT)

#define __G_ASSERT(test, return_val, error, domain, error_code)\
G_STMT_START\
{\
        if G_LIKELY (!(test)) { \
                g_set_error (error, domain, error_code, #test); \
                return (return_val); \
        }\
}\
G_STMT_END


static void email_service_init(EmailService *email_service)
{
	EM_DEBUG_LOG("email_service_init entered");
}

GQuark email_service_error_quark(void)
{
	EM_DEBUG_LOG("email_service_error_quark entered");
	return g_quark_from_static_string("email_service_error");
}


static void email_service_class_init(EmailServiceClass *email_service_class)
{
	EM_DEBUG_LOG("email_service_class_init entered");

	/*
	dbus_g_object_type_install_info(EMAIL_SERVICE_TYPE, &dbus_glib_nfc_service_object_info);
	*/
}

EXPORT_API int emipc_launch_email_service()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	/*DBUS*/
	DBusGConnection *connection = NULL;
	DBusGProxy *proxy = NULL;
	GError *error = NULL;
	guint dbus_result = 0;

	//if(!g_thread_supported()) {
	//	g_thread_init(NULL);
	//}

	//dbus_g_thread_init();

	g_type_init();

	//pthread_mutex_lock(&g_client_context.g_client_lock);

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error == NULL) {
		proxy = dbus_g_proxy_new_for_name(connection, EMAIL_SERVICE_NAME, EMAIL_SERVICE_PATH, EMAIL_SERVICE_NAME);

		if (proxy != NULL) {
			if (org_tizen_email_service_launch(proxy, &dbus_result, &error) == false) {
				EM_DEBUG_EXCEPTION("email_service_launch failed");
				if (error != NULL) {
					EM_DEBUG_EXCEPTION("message : [%s]", error->message);
					g_error_free(error);
					return EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
				}
			}
			EM_DEBUG_LOG("org_tizen_email_service_launch");

			g_object_unref (proxy);
		}
		else {
			EM_DEBUG_EXCEPTION("dbus_g_proxy_new_for_name failed");
		}
	}
	else {
		EM_DEBUG_EXCEPTION("ERROR: Can't get on system bus [%s]", error->message);
		g_error_free(error);
	}

	EM_DEBUG_FUNC_END("ret [%d]", err);
	return err;
}


EXPORT_API int emipc_init_dbus_connection()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	GError *error = NULL;
	DBusGProxy *proxy = NULL;
	guint ret = 0;

	g_type_init();

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error == NULL) {
		object = (GObject *)g_object_new(EMAIL_SERVICE_TYPE, NULL);
		dbus_g_connection_register_g_object(connection, EMAIL_SERVICE_PATH, object);

		// register service
		proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);
		if (proxy != NULL) {
			if (!org_freedesktop_DBus_request_name(proxy, EMAIL_SERVICE_NAME, 0, &ret, &error)) {
				EM_DEBUG_EXCEPTION("Unable to register service: %s", error->message);
				g_error_free(error);
			}

			g_object_unref (proxy);
		}
		else {
			EM_DEBUG_EXCEPTION("dbus_g_proxy_new_for_name failed");
		}
	}
	else {
		EM_DEBUG_EXCEPTION("ERROR: Can't get on system bus [%s]", error->message);
		g_error_free(error);
	}
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

gboolean email_service_launch(EmailService *email_service, guint *result_val, GError **error)
{
	EM_DEBUG_LOG("email_service_launch entered");

	EM_DEBUG_LOG("email_service_launch PID=[%ld]\n" , getpid());
	EM_DEBUG_LOG("email_service_launch TID=[%ld]" , pthread_self());

	launch_by_client = getpid();

	return TRUE;
}
