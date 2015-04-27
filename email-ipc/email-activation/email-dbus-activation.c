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
#include "email-types.h"
#include "email-debug-log.h"
#include "email-dbus-activation.h"
#include "email-utilities.h"
#include "email-core-account.h"
#include "email-core-utils.h"

EXPORT_API int launch_pid = 0;
EXPORT_API const gchar introspection_xml[] =
"<node>"
"	<interface name='org.tizen.email_service'>"
"		<method name='Launch'>"
"			<arg type='i' name='caller_pid' direction='in' />"
"			<arg type='i' name='ret' direction='out' />"
"		</method>"
"		<method name='SetContactsLog'>"
"			<arg type='i' name='account_id' direction='in'/>"
"			<arg type='s' name='email_address' direction='in'/>"
"			<arg type='s' name='subject' direction='in'/>"
"			<arg type='i' name='date_time' direction='in'/>"
"			<arg type='i' name='action' direction='in'/>"
"			<arg type='i' name='err' direction='out'/>"
"		</method>"
"		<method name='DeleteContactsLog'>"
"			<arg type='i' name='account_id' direction='in' />"
"			<arg type='i' name='ret' direction='out' />"
"		</method>"
"		<method name='GetDisplayName'>"
"			<arg type='s' name='email_address' direction='in' />"
"			<arg type='s' name='multi_user_name' direction='in' />"
"			<arg type='s' name='contact_display_name' direction='out' />"
"			<arg type='i' name='ret' direction='out' />"
"		</method>"
"		<method name='CheckBlockingMode'>"
"			<arg type='s' name='sender_address' direction='in' />"
"			<arg type='s' name='multi_user_name' direction='in' />"
"			<arg type='i' name='blocking_mode' direction='out' />"
"			<arg type='i' name='ret' direction='out' />"
"		</method>"
"		<method name='QueryServerInfo'>"
"			<arg type='i' name='pid' direction='in'/>"
"			<arg type='s' name='domain_name' direction='in'/>"
                        /* err, email_server_info_t */
"			<arg type='(i((sii)a(iisiii)))' name='query_ret' direction='out'/>"
"		</method>"
"	</interface>"
"</node>";

static gboolean on_timer_proxy_new(gpointer userdata)
{
	EM_DEBUG_LOG("on_timer_proxy_new");
	GCancellable *proxy_cancel = (GCancellable *)userdata;

	if (proxy_cancel) {
		if (!g_cancellable_is_cancelled (proxy_cancel))
			g_cancellable_cancel(proxy_cancel);
	}

	return false;
}

/* called from clients */
EXPORT_API int emipc_launch_email_service()
{
	EM_DEBUG_LOG("emipc_launch_email_service");
	GError *gerror = NULL;
	int ret = EMAIL_ERROR_NONE;
	guint timer_tag = 0;

#if !GLIB_CHECK_VERSION(2, 36, 0) 
	g_type_init(); 
#endif

	GCancellable *proxy_cancel = g_cancellable_new();
	timer_tag = g_timeout_add(5000, on_timer_proxy_new, proxy_cancel);
	GDBusProxy* bproxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM, 
	                            G_DBUS_PROXY_FLAGS_NONE, 
	                            NULL, 
	                            EMAIL_SERVICE_NAME,
	                            EMAIL_SERVICE_PATH,
	                            EMAIL_SERVICE_NAME, 
	                            proxy_cancel,
	                            &gerror);

	g_source_remove(timer_tag);

	if (!bproxy) {
		EM_DEBUG_EXCEPTION ("g_dbus_proxy_new_for_bus_sync error [%s]", 
                                 gerror->message);
		ret = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
		goto FINISH_OFF;
	}

	GVariant *result = g_dbus_proxy_call_sync (bproxy, 
                        "Launch", 
                        g_variant_new ("(i)", getpid()), 
                        G_DBUS_CALL_FLAGS_NONE, 
                        5000,  /* msec, 5s*/
                        NULL, 
                        &gerror);


	if (!result) {
		EM_DEBUG_EXCEPTION ("g_dbus_proxy_call_sync 'Launch' error [%s]", 
                                 gerror->message);
		ret = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
		goto FINISH_OFF;
	}

	g_variant_get (result, "(&d)", &ret);

FINISH_OFF:
	EM_DEBUG_LOG ("ret [%d]\n", ret);
	if (bproxy)
		g_object_unref (bproxy);

	if (proxy_cancel)
		g_object_unref(proxy_cancel);

	if (gerror)
		g_error_free (gerror);

	return ret;
}

void cancellable_connect_cb ()
{
	EM_DEBUG_LOG ("Cancellable is now canceled");
}

EXPORT_API GCancellable *cancel = NULL;

GVariant* em_gdbus_get_display_name (GVariant *parameters)
{
	char *email_address        = NULL;
	char *multi_user_name      = NULL;
	char *contact_display_name = NULL;

	g_variant_get (parameters, "(ss)", &email_address, &multi_user_name);

	/* replace "" to NULL */
	if (!g_strcmp0(email_address,""))
		EM_SAFE_FREE (email_address);

	int err = emcore_get_mail_display_name_internal (multi_user_name, email_address, &contact_display_name);

	/* make return_val */
	if (!contact_display_name) {
		contact_display_name = strdup("");
	}
	GVariant* ret = g_variant_new ("(si)", contact_display_name, err);

	/* clean-up */
	EM_SAFE_FREE (email_address);
	EM_SAFE_FREE (contact_display_name);

	return ret;
}

GVariant* em_gdbus_check_blocking_mode (GVariant *parameters)
{
	char *sender_address = NULL;
	char *multi_user_name = NULL;
	int blocking_mode = 0;
	int err = EMAIL_ERROR_NONE;

	g_variant_get (parameters, "(ss)", &sender_address, &multi_user_name);
#ifdef __FEATURE_BLOCKING_MODE__
	err = emcore_check_blocking_mode_internal (multi_user_name, sender_address, &blocking_mode);
#endif /* __FEATURE_BLOCKING_MODE__ */
	/* make return_val */
	GVariant* ret = g_variant_new ("(ii)", blocking_mode, err);

	/* clean-up string */
	EM_SAFE_FREE (sender_address);
	EM_SAFE_FREE (multi_user_name);

	return ret;
}

