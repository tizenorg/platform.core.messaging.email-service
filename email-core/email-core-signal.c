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

/*
 * email-core-signal.c
 *
 *  Created on: 2012. 11. 22.
 *      Author: kyuho.jo@samsung.com
 */
#include <dbus/dbus.h>

#include "email-core-signal.h"
#include "email-core-utils.h"
#include "email-internal-types.h"
#include "email-debug-log.h"

#define EMAIL_STORAGE_CHANGE_NOTI       "User.Email.StorageChange"
#define EMAIL_NETOWRK_CHANGE_NOTI       "User.Email.NetworkStatus"
#define EMAIL_RESPONSE_TO_API_NOTI      "User.Email.ResponseToAPI"

#define DBUS_SIGNAL_PATH_FOR_TASK_STATUS       "/User/Email/TaskStatus"
#define DBUS_SIGNAL_INTERFACE_FOR_TASK_STATUS  "User.Email.TaskStatus"
#define DBUS_SIGNAL_NAME_FOR_TASK_STATUS       "email"

static pthread_mutex_t _dbus_noti_lock = PTHREAD_MUTEX_INITIALIZER;

typedef enum
{
	_NOTI_TYPE_STORAGE         = 0,
	_NOTI_TYPE_NETWORK         = 1,
	_NOTI_TYPE_RESPONSE_TO_API = 2,
} enotitype_t;

INTERNAL_FUNC int emcore_initialize_signal()
{
	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC int emcore_finalize_signal()
{
	DELETE_CRITICAL_SECTION(_dbus_noti_lock);
	return EMAIL_ERROR_NONE;
}

static int emcore_send_signal(enotitype_t notiType, int subType, int data1, int data2, char *data3, int data4)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_PROFILE_BEGIN(profile_emcore_send_signal);

	int ret = 0;
	DBusConnection *connection;
	DBusMessage	*signal = NULL;
	DBusError	   dbus_error;
	dbus_uint32_t   error;
	const char	 *nullString = "";

	ENTER_CRITICAL_SECTION(_dbus_noti_lock);

	dbus_error_init (&dbus_error);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);

	if (connection == NULL) {
		EM_DEBUG_LOG("dbus_bus_get is failed");
		goto FINISH_OFF;
	}

	if (notiType == _NOTI_TYPE_STORAGE) {
		signal = dbus_message_new_signal("/User/Email/StorageChange", EMAIL_STORAGE_CHANGE_NOTI, "email");

		if (signal == NULL) {
			EM_DEBUG_EXCEPTION("dbus_message_new_signal is failed");
			goto FINISH_OFF;
		}
		EM_DEBUG_LOG("/User/Email/StorageChange Signal is created by dbus_message_new_signal");

		dbus_message_append_args(signal, DBUS_TYPE_INT32, &subType, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data1, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data2, DBUS_TYPE_INVALID);
		if (data3 == NULL)
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &nullString, DBUS_TYPE_INVALID);
		else
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &data3, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data4, DBUS_TYPE_INVALID);
	}
	else if (notiType == _NOTI_TYPE_NETWORK) {
		signal = dbus_message_new_signal("/User/Email/NetworkStatus", EMAIL_NETOWRK_CHANGE_NOTI, "email");

		if (signal == NULL) {
			EM_DEBUG_EXCEPTION("dbus_message_new_signal is failed");
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("/User/Email/NetworkStatus Signal is created by dbus_message_new_signal");

		dbus_message_append_args(signal, DBUS_TYPE_INT32, &subType, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data1, DBUS_TYPE_INVALID);
		if (data3 == NULL)
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &nullString, DBUS_TYPE_INVALID);
		else
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &data3, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data2, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data4, DBUS_TYPE_INVALID);
	}
	else if (notiType == _NOTI_TYPE_RESPONSE_TO_API) {
		signal = dbus_message_new_signal("/User/Email/ResponseToAPI", EMAIL_RESPONSE_TO_API_NOTI, "email");

		if (signal == NULL) {
			EM_DEBUG_EXCEPTION("dbus_message_new_signal is failed");
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("/User/Email/ResponseToAPI Signal is created by dbus_message_new_signal");

		dbus_message_append_args(signal, DBUS_TYPE_INT32, &subType, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data1, DBUS_TYPE_INVALID);
		dbus_message_append_args(signal, DBUS_TYPE_INT32, &data2, DBUS_TYPE_INVALID);
	}
	else {
		EM_DEBUG_EXCEPTION("Wrong notification type [%d]", notiType);
		error = EMAIL_ERROR_IPC_CRASH;
		goto FINISH_OFF;
	}

	if (!dbus_connection_send(connection, signal, &error)) {
		EM_DEBUG_LOG("dbus_connection_send is failed [%d]", error);
	}
	else {
		EM_DEBUG_LOG("dbus_connection_send is successful");
		ret = 1;
	}

/* 	EM_DEBUG_LOG("Before dbus_connection_flush");	 */
/* 	dbus_connection_flush(connection);               */
/* 	EM_DEBUG_LOG("After dbus_connection_flush");	 */

	ret = true;
FINISH_OFF:
	if (signal)
		dbus_message_unref(signal);

	LEAVE_CRITICAL_SECTION(_dbus_noti_lock);
	EM_PROFILE_END(profile_emcore_send_signal);
	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


INTERNAL_FUNC int emcore_notify_storage_event(email_noti_on_storage_event transaction_type, int data1, int data2 , char *data3, int data4)
{
	EM_DEBUG_FUNC_BEGIN("transaction_type[%d], data1[%d], data2[%d], data3[%p], data4[%d]", transaction_type, data1, data2, data3, data4);
	return emcore_send_signal(_NOTI_TYPE_STORAGE, (int)transaction_type, data1, data2, data3, data4);
}

INTERNAL_FUNC int emcore_notify_network_event(email_noti_on_network_event status_type, int data1, char *data2, int data3, int data4)
{
	EM_DEBUG_FUNC_BEGIN("status_type[%d], data1[%d], data2[%p], data3[%d], data4[%d]", status_type, data1, data2, data3, data4);
	return emcore_send_signal(_NOTI_TYPE_NETWORK, (int)status_type, data1, data3, data2, data4);
}

INTERNAL_FUNC int emcore_notify_response_to_api(email_event_type_t event_type, int data1, int data2)
{
	EM_DEBUG_FUNC_BEGIN("event_type[%d], data1[%d], data2[%p], data3[%d], data4[%d]", event_type, data1, data2);
	return emcore_send_signal(_NOTI_TYPE_RESPONSE_TO_API, (int)event_type, data1, data2, NULL, 0);
}

INTERNAL_FUNC int emcore_send_task_status_signal(email_task_type_t input_task_type, int input_task_id, email_task_status_type_t input_task_status, int input_param_1, int input_param_2)
{
	EM_DEBUG_FUNC_BEGIN("input_task_type [%d] input_task_id [%d] input_task_status [%d] input_param_1 [%d] input_param_2 [%d]", input_task_type, input_task_id, input_task_status, input_param_1, input_param_2);

	int             err = EMAIL_ERROR_NONE;
	DBusConnection *connection;
	DBusMessage	   *signal = NULL;
	DBusError	    dbus_error;
	dbus_uint32_t   error;

	ENTER_CRITICAL_SECTION(_dbus_noti_lock);

	dbus_error_init (&dbus_error);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);

	if (connection == NULL) {
		EM_DEBUG_LOG("dbus_bus_get is failed");
		goto FINISH_OFF;
	}

	signal = dbus_message_new_signal(DBUS_SIGNAL_PATH_FOR_TASK_STATUS, DBUS_SIGNAL_INTERFACE_FOR_TASK_STATUS, DBUS_SIGNAL_NAME_FOR_TASK_STATUS);

	if (signal == NULL) {
		EM_DEBUG_EXCEPTION("dbus_message_new_signal is failed");
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Signal for task status has been created by dbus_message_new_signal");

	dbus_message_append_args(signal, DBUS_TYPE_INT32, &input_task_type, DBUS_TYPE_INVALID);
	dbus_message_append_args(signal, DBUS_TYPE_INT32, &input_task_id, DBUS_TYPE_INVALID);
	dbus_message_append_args(signal, DBUS_TYPE_INT32, &input_task_status, DBUS_TYPE_INVALID);
	dbus_message_append_args(signal, DBUS_TYPE_INT32, &input_param_1, DBUS_TYPE_INVALID);
	dbus_message_append_args(signal, DBUS_TYPE_INT32, &input_param_2, DBUS_TYPE_INVALID);

	if (!dbus_connection_send(connection, signal, &error)) {
		EM_DEBUG_LOG("dbus_connection_send is failed [%d]", error);
	}
	else {
		EM_DEBUG_LOG("dbus_connection_send is successful");
	}

/* 	EM_DEBUG_LOG("Before dbus_connection_flush");	 */
/* 	dbus_connection_flush(connection);               */
/* 	EM_DEBUG_LOG("After dbus_connection_flush");	 */

FINISH_OFF:
	if (signal)
		dbus_message_unref(signal);

	LEAVE_CRITICAL_SECTION(_dbus_noti_lock);
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
