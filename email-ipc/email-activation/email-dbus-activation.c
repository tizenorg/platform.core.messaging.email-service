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


G_DEFINE_TYPE(EmailService, email_service, G_TYPE_OBJECT)

static void email_service_init(EmailService *email_service)
{
	EM_DEBUG_LOG("email_service_init entered");
}

static void email_service_class_init(EmailServiceClass *email_service_class)
{
	EM_DEBUG_LOG("email_service_class_init entered");

	dbus_g_object_type_install_info(EMAIL_SERVICE_TYPE, &dbus_glib_email_service_object_info);
}

EXPORT_API int emipc_launch_email_service()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	DBusGConnection *connection = NULL;
	DBusGProxy *proxy = NULL;
	GError *error = NULL;
	guint dbus_result = 0;

	g_type_init();

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (!connection) {
		if (error) {
			EM_DEBUG_EXCEPTION("Unable to connect to dbus: %s", error->message);
			g_error_free(error);
			err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
			return err;
		}
	}

	proxy = dbus_g_proxy_new_for_name(connection, EMAIL_SERVICE_NAME, EMAIL_SERVICE_PATH, EMAIL_SERVICE_NAME);
	if (!proxy) {
		EM_DEBUG_EXCEPTION("dbus_g_proxy_new_for_name failed");
		err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
		return err;
	}

	//will trigger activation if the service does not exist
	if (org_tizen_email_service_launch(proxy, &dbus_result, &error) == false) {
		if (error) {
			EM_DEBUG_EXCEPTION("email_service_launch failed : [%s]", error->message);
			g_error_free(error);
			err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
			return err;
		}
	}

	EM_DEBUG_LOG("org_tizen_email_service_launch : [%d]", dbus_result);

	g_object_unref(proxy);
	EM_DEBUG_FUNC_END("ret [%d]", err);
	return err;
}

EXPORT_API int emipc_init_dbus_connection()
{
	EM_DEBUG_FUNC_BEGIN();

	EmailService *object;
	DBusGProxy *proxy = NULL;
	DBusGConnection *connection = NULL;
	GError *error = NULL;

	guint request_ret = 0;
	int err = EMAIL_ERROR_NONE;

	g_type_init();

	//init dbus connection
	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (!connection) {
		if (error) {
			EM_DEBUG_EXCEPTION("Unable to connect to dbus: %s", error->message);
			g_error_free(error);
			err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
			return err;
		}
	}

	//create email_service object
	object = g_object_new(EMAIL_SERVICE_TYPE, NULL);

	//register dbus path
	dbus_g_connection_register_g_object(connection, EMAIL_SERVICE_PATH, G_OBJECT(object));

	//register the service name
	proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);
	if (!proxy) {
		EM_DEBUG_EXCEPTION("dbus_g_proxy_new_for_name failed");
		err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
		return err;
	}

	if (!org_freedesktop_DBus_request_name(proxy, EMAIL_SERVICE_NAME, 0, &request_ret, &error)) {
		if (error) {
			EM_DEBUG_EXCEPTION("Unable to register service: %s", error->message);
			g_error_free(error);
			err = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
		}
	}

	g_object_unref(proxy);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

gboolean email_service_launch(EmailService *email_service, guint *result_val, GError **error)
{
	EM_DEBUG_LOG("email_service_launch entered");
	EM_DEBUG_LOG("email_service_launch PID=[%ld]" , getpid());
	EM_DEBUG_LOG("email_service_launch TID=[%ld]" , pthread_self());

	if (result_val)
		*result_val = TRUE;

	return TRUE;
}
