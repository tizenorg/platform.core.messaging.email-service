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
	dbus_uint32_t   error = 0;
	const char	 *nullString = "";

	ENTER_CRITICAL_SECTION(_dbus_noti_lock);

	dbus_error_init (&dbus_error);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);

	if (dbus_error_is_set (&dbus_error)) {
		EM_DEBUG_EXCEPTION ("dbus_bus_get failed: [%s]", dbus_error.message);
		dbus_error_free (&dbus_error);
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
		EM_DEBUG_EXCEPTION ("dbus_connection_send failed [%d]", error);
	}
	else {
		EM_DEBUG_LOG_DEV ("dbus_connection_send done");
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

INTERNAL_FUNC int em_send_notification_to_active_sync_engine(int subType, ASNotiData *data)
{
	EM_DEBUG_FUNC_BEGIN("subType [%d], data [%p]", subType, data);

	DBusConnection     *connection;
	DBusMessage        *signal = NULL;
	DBusError           dbus_error;
	int                 i = 0;
	dbus_uint32_t   error = 0;
	int ret = true;
	const char *null_string = "";

	ENTER_CRITICAL_SECTION(_dbus_noti_lock);

	dbus_error_init (&dbus_error);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);
	if (dbus_error_is_set (&dbus_error)) {
		EM_DEBUG_EXCEPTION("dbus_bus_get failed: [%s]", dbus_error.message);
		dbus_error_free (&dbus_error);
		ret = false;
		goto FINISH_OFF;
	}

	signal = dbus_message_new_signal("/User/Email/ActiveSync", EMAIL_ACTIVE_SYNC_NOTI, "email");

	dbus_message_append_args(signal, DBUS_TYPE_INT32, &subType, DBUS_TYPE_INVALID);
	switch ( subType ) {
		case ACTIVE_SYNC_NOTI_SEND_MAIL:
			EM_DEBUG_LOG("handle:[%d]", data->send_mail.handle);
			EM_DEBUG_LOG("account_id:[%d]", data->send_mail.account_id);
			EM_DEBUG_LOG("mail_id:[%d]", data->send_mail.mail_id);

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.handle), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.mail_id), DBUS_TYPE_INVALID);
			if (data->send_mail.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->send_mail.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;
		case ACTIVE_SYNC_NOTI_SEND_SAVED:				/*  publish a send notification to ASE (active sync engine) */
			EM_DEBUG_EXCEPTION("Not support yet : subType[ACTIVE_SYNC_NOTI_SEND_SAVED]", subType);
			break;
		case ACTIVE_SYNC_NOTI_SEND_REPORT:
			EM_DEBUG_EXCEPTION("Not support yet : subType[ACTIVE_SYNC_NOTI_SEND_REPORT]", subType);
			break;
		case ACTIVE_SYNC_NOTI_SYNC_HEADER:
			EM_DEBUG_LOG("handle:[%d]", data->sync_header.handle);
			EM_DEBUG_LOG("account_id:[%d]", data->sync_header.account_id);
			EM_DEBUG_LOG("mailbox_id:[%d]", data->sync_header.mailbox_id);

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->sync_header.handle ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->sync_header.account_id ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->sync_header.mailbox_id ), DBUS_TYPE_INVALID);
			if (data->sync_header.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->sync_header.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;
		case ACTIVE_SYNC_NOTI_DOWNLOAD_BODY:			/*  publish a download body notification to ASE */
			EM_DEBUG_LOG("handle:[%d]", data->download_body.handle);
			EM_DEBUG_LOG("account_id:[%d]", data->download_body.account_id);
			EM_DEBUG_LOG("mail_id:[%d]", data->download_body.mail_id);
			EM_DEBUG_LOG("with_attachment:[%d]", data->download_body.with_attachment);

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_body.handle  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_body.account_id  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_body.mail_id ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_body.with_attachment  ), DBUS_TYPE_INVALID);
			if (data->download_body.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->download_body.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

			break;
		case ACTIVE_SYNC_NOTI_DOWNLOAD_ATTACHMENT:
			EM_DEBUG_LOG("handle:[%d]", data->download_attachment.handle);
			EM_DEBUG_LOG("account_id:[%d]", data->download_attachment.account_id );
			EM_DEBUG_LOG("mail_id:[%d]", data->download_attachment.mail_id);
			EM_DEBUG_LOG("with_attachment:[%d]", data->download_attachment.attachment_order );

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_attachment.handle  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_attachment.account_id  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_attachment.mail_id ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_attachment.attachment_order), DBUS_TYPE_INVALID);
			if (data->download_attachment.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->download_attachment.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_VALIDATE_ACCOUNT:
			EM_DEBUG_EXCEPTION("Not support yet : subType[ACTIVE_SYNC_NOTI_VALIDATE_ACCOUNT]", subType);
			break;

		case ACTIVE_SYNC_NOTI_CANCEL_JOB:
			EM_DEBUG_LOG("account_id:[%d]",       data->cancel_job.account_id );
			EM_DEBUG_LOG("handle to cancel:[%d]", data->cancel_job.handle);
			EM_DEBUG_LOG("cancel_type:[%d]",      data->cancel_job.cancel_type);

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->cancel_job.account_id  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->cancel_job.handle  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->cancel_job.cancel_type  ), DBUS_TYPE_INVALID);
			if (data->cancel_job.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->cancel_job.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_SEARCH_ON_SERVER:
			EM_DEBUG_LOG("account_id:[%d]",          data->search_mail_on_server.account_id );
			EM_DEBUG_LOG("mailbox_id:[%d]",        data->search_mail_on_server.mailbox_id );
			EM_DEBUG_LOG("search_filter_count:[%d]", data->search_mail_on_server.search_filter_count );
			EM_DEBUG_LOG("handle to cancel:[%d]",    data->search_mail_on_server.handle);

			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->search_mail_on_server.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->search_mail_on_server.mailbox_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->search_mail_on_server.search_filter_count), DBUS_TYPE_INVALID);
			for(i = 0; i < data->search_mail_on_server.search_filter_count; i++) {
				dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->search_mail_on_server.search_filter_list[i].search_filter_type), DBUS_TYPE_INVALID);
				switch(data->search_mail_on_server.search_filter_list[i].search_filter_type) {
					case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       :
					case EMAIL_SEARCH_FILTER_TYPE_UID              :
					case EMAIL_SEARCH_FILTER_TYPE_ALL              :
					case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      :
					case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_NEW        :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_OLD        :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       :
					case EMAIL_SEARCH_FILTER_TYPE_HEADER_PRIORITY  :
						dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->search_mail_on_server.search_filter_list[i].search_filter_key_value.integer_type_key_value), DBUS_TYPE_INVALID);
						break;

					case EMAIL_SEARCH_FILTER_TYPE_BCC              :
					case EMAIL_SEARCH_FILTER_TYPE_BODY             :
					case EMAIL_SEARCH_FILTER_TYPE_CC               :
					case EMAIL_SEARCH_FILTER_TYPE_FROM             :
					case EMAIL_SEARCH_FILTER_TYPE_KEYWORD          :
					case EMAIL_SEARCH_FILTER_TYPE_TEXT             :
					case EMAIL_SEARCH_FILTER_TYPE_SUBJECT          :
					case EMAIL_SEARCH_FILTER_TYPE_TO               :
					case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       :
					case EMAIL_SEARCH_FILTER_TYPE_ATTACHMENT_NAME  :
						dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->search_mail_on_server.search_filter_list[i].search_filter_key_value.string_type_key_value), DBUS_TYPE_INVALID);
						break;

					case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE :
					case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     :
					case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  :
						dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->search_mail_on_server.search_filter_list[i].search_filter_key_value.time_type_key_value), DBUS_TYPE_INVALID);
						break;
					default :
						EM_DEBUG_EXCEPTION("Invalid filter type [%d]", data->search_mail_on_server.search_filter_list[i].search_filter_type);
						break;
				}
			}
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->search_mail_on_server.handle), DBUS_TYPE_INVALID);
			if (data->search_mail_on_server.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->search_mail_on_server.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_CLEAR_RESULT_OF_SEARCH_ON_SERVER :
			EM_DEBUG_LOG("account_id:[%d]",          data->clear_result_of_search_mail_on_server.account_id );
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->search_mail_on_server.account_id), DBUS_TYPE_INVALID);
			if (data->search_mail_on_server.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->search_mail_on_server.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_EXPUNGE_MAILS_DELETED_FLAGGED :
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->expunge_mails_deleted_flagged.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->expunge_mails_deleted_flagged.mailbox_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->expunge_mails_deleted_flagged.on_server), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->expunge_mails_deleted_flagged.handle), DBUS_TYPE_INVALID);
			if (data->expunge_mails_deleted_flagged.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->expunge_mails_deleted_flagged.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_RESOLVE_RECIPIENT :
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->get_resolve_recipients.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->get_resolve_recipients.email_address), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->get_resolve_recipients.handle), DBUS_TYPE_INVALID);
			if (data->get_resolve_recipients.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->get_resolve_recipients.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_VALIDATE_CERTIFICATE :
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->validate_certificate.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->validate_certificate.email_address), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->validate_certificate.handle), DBUS_TYPE_INVALID);
			if (data->validate_certificate.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->validate_certificate.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_ADD_MAILBOX :
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->add_mailbox.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->add_mailbox.mailbox_path), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->add_mailbox.mailbox_alias), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->add_mailbox.handle), DBUS_TYPE_INVALID);
			/* eas is not ready to use void array and length */
			dbus_message_append_args (signal, DBUS_TYPE_STRING, &(data->add_mailbox.eas_data), DBUS_TYPE_INVALID);
/*			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->add_mailbox.eas_data_length), DBUS_TYPE_INVALID);
			if (data->add_mailbox.eas_data_length > 0) {
				dbus_message_append_args (signal, DBUS_TYPE_ARRAY,  DBUS_TYPE_INT32, &(data->add_mailbox.eas_data),\
												data->add_mailbox.eas_data_length,DBUS_TYPE_INVALID);
			} */
			if (data->add_mailbox.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->add_mailbox.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_RENAME_MAILBOX :
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->rename_mailbox.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->rename_mailbox.mailbox_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->rename_mailbox.mailbox_name), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->rename_mailbox.mailbox_alias), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->rename_mailbox.handle), DBUS_TYPE_INVALID);
			/* eas is not ready to use void array and length */
			dbus_message_append_args (signal, DBUS_TYPE_STRING, &(data->rename_mailbox.eas_data), DBUS_TYPE_INVALID);
/*			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->rename_mailbox.eas_data_length), DBUS_TYPE_INVALID);
			if (data->rename_mailbox.eas_data_length > 0) {
				dbus_message_append_args (signal, DBUS_TYPE_ARRAY,  DBUS_TYPE_INT32, &(data->rename_mailbox.eas_data),\
											   data->rename_mailbox.eas_data_length, DBUS_TYPE_INVALID);
			}*/
			if (data->rename_mailbox.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->rename_mailbox.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_DELETE_MAILBOX :
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->delete_mailbox.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->delete_mailbox.mailbox_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->delete_mailbox.handle), DBUS_TYPE_INVALID);
			if (data->delete_mailbox.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->delete_mailbox.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_DELETE_MAILBOX_EX :
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->delete_mailbox_ex.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->delete_mailbox_ex.mailbox_id_count), DBUS_TYPE_INVALID);
			for(i = 0; i <data->delete_mailbox_ex.mailbox_id_count; i++)
				dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->delete_mailbox_ex.mailbox_id_array[i]), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->delete_mailbox_ex.handle), DBUS_TYPE_INVALID);
			if (data->delete_mailbox_ex.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->delete_mailbox_ex.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_SEND_MAIL_WITH_DOWNLOADING_OF_ORIGINAL_MAIL:
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail_with_downloading_attachment_of_original_mail.handle), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail_with_downloading_attachment_of_original_mail.mail_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail_with_downloading_attachment_of_original_mail.account_id), DBUS_TYPE_INVALID);
			if (data->send_mail_with_downloading_attachment_of_original_mail.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->send_mail_with_downloading_attachment_of_original_mail.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_SCHEDULE_SENDING_MAIL:
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->schedule_sending_mail.handle), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->schedule_sending_mail.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->schedule_sending_mail.mail_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->schedule_sending_mail.scheduled_time), DBUS_TYPE_INVALID);
			if (data->schedule_sending_mail.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->schedule_sending_mail.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		case ACTIVE_SYNC_NOTI_CANCEL_SENDING_MAIL:
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->cancel_sending_mail.handle), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->cancel_sending_mail.account_id  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->cancel_sending_mail.mail_id), DBUS_TYPE_INVALID);
			if (data->cancel_sending_mail.multi_user_name)
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->cancel_sending_mail.multi_user_name), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(null_string), DBUS_TYPE_INVALID);

			break;

		default:
			EM_DEBUG_EXCEPTION("Invalid Notification type of Active Sync : subType[%d]", subType);
			ret = false;
			goto FINISH_OFF;
	}

	if (!dbus_connection_send (connection, signal, &error)) {
		EM_DEBUG_EXCEPTION ("dbus_connection_send failed [%d]", error);
		ret = false;
	}

	/*dbus_connection_flush(connection);*/

FINISH_OFF:

	if(signal)
		dbus_message_unref(signal);

	LEAVE_CRITICAL_SECTION(_dbus_noti_lock);
	EM_DEBUG_FUNC_END();
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
	dbus_uint32_t   error = 0;

	ENTER_CRITICAL_SECTION(_dbus_noti_lock);

	dbus_error_init (&dbus_error);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);
	if (dbus_error_is_set (&dbus_error)) {
		EM_DEBUG_EXCEPTION("dbus_bus_get failed: [%s]", dbus_error.message);
		dbus_error_free (&dbus_error);
		err = EMAIL_ERROR_IPC_CONNECTION_FAILURE;
		goto FINISH_OFF;
	}

	signal = dbus_message_new_signal(DBUS_SIGNAL_PATH_FOR_TASK_STATUS, DBUS_SIGNAL_INTERFACE_FOR_TASK_STATUS, DBUS_SIGNAL_NAME_FOR_TASK_STATUS);

	if (signal == NULL) {
		EM_DEBUG_EXCEPTION("dbus_message_new_signal is failed");
		err = EMAIL_ERROR_IPC_CONNECTION_FAILURE;
		goto FINISH_OFF;
	}
	EM_DEBUG_LOG("Signal for task status has been created by dbus_message_new_signal");

	dbus_message_append_args(signal, DBUS_TYPE_INT32, &input_task_type, DBUS_TYPE_INVALID);
	dbus_message_append_args(signal, DBUS_TYPE_INT32, &input_task_id, DBUS_TYPE_INVALID);
	dbus_message_append_args(signal, DBUS_TYPE_INT32, &input_task_status, DBUS_TYPE_INVALID);
	dbus_message_append_args(signal, DBUS_TYPE_INT32, &input_param_1, DBUS_TYPE_INVALID);
	dbus_message_append_args(signal, DBUS_TYPE_INT32, &input_param_2, DBUS_TYPE_INVALID);

	if (!dbus_connection_send(connection, signal, &error)) {
		EM_DEBUG_EXCEPTION("dbus_connection_send failed: [%d]", error);
		err = EMAIL_ERROR_IPC_CONNECTION_FAILURE;
	}
	else {
		EM_DEBUG_LOG("dbus_connection_send done");
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
