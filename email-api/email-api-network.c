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



/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-network.c
 * @brief 		This file contains the data structures and interfaces of Network related Functionality provided by 
 *			email-service . 
 */
 
#include "email-api.h"
#include "string.h"
#include "email-convert.h"
#include "email-api-mailbox.h"
#include "email-types.h"
#include "email-utilities.h"
#include "email-ipc.h"
#include "email-storage.h"

#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
#include <vconf.h>
#include <dbus/dbus.h>
#endif	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__ */

#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__


#define ACTIVE_SYNC_HANDLE_INIT_VALUE		(-1)
#define ACTIVE_SYNC_HANDLE_BOUNDARY			(-100000000)


static int email_get_account_server_type_by_account_id(int account_id, emf_account_server_t* account_server_type, int flag, int *error)
{
	EM_DEBUG_FUNC_BEGIN();
	emf_account_t *account=NULL;
	int ret = false;
	int err= EMF_ERROR_NONE;

	if (account_server_type == NULL ) {
		EM_DEBUG_EXCEPTION("account_server_type is NULL");
		err = EMF_ERROR_INVALID_PARAM;
		ret = false;
		goto FINISH_OFF;
	}
		
	if( (err = email_get_account(account_id,WITHOUT_OPTION,&account)) < 0) {
		EM_DEBUG_EXCEPTION ("email_get_account failed [%d] ", err);
		ret = false;
		goto FINISH_OFF;
	}

	if ( flag == false )  {	/*  sending server */
		*account_server_type = account->sending_server_type;
	} else if ( flag == true ) {	/*  receiving server */
		*account_server_type = account->receiving_server_type;
	}

	ret = true;

FINISH_OFF:
	if ( account != NULL ) {
		email_free_account(&account, 1);
	}
	if ( error != NULL ) {
		*error = err;
	}

	return ret;
}

static int email_get_handle_for_activesync(int *handle, int *error)
{
	EM_DEBUG_FUNC_BEGIN();

	static int next_handle = 0;
	int ret = false;
	int err = EMF_ERROR_NONE;
			
	if ( handle == NULL ) {
		EM_DEBUG_EXCEPTION("email_get_handle_for_activesync failed : handle is NULL");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	if ( vconf_get_int(VCONFKEY_EMAIL_SERVICE_ACTIVE_SYNC_HANDLE, &next_handle)  != 0 ) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
		if ( next_handle != 0 ) {
			err = EMF_ERROR_GCONF_FAILURE;
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG(">>>>>> VCONFKEY_EMAIL_SERVICE_ACTIVE_SYNC_HANDLE : get lastest handle[%d]", next_handle);

	/*  set the value of the handle for active sync */
	next_handle--;
	if ( next_handle < ACTIVE_SYNC_HANDLE_BOUNDARY ) {
		next_handle = ACTIVE_SYNC_HANDLE_INIT_VALUE;
	}
	if ( vconf_set_int(VCONFKEY_EMAIL_SERVICE_ACTIVE_SYNC_HANDLE, next_handle) != 0) {
		EM_DEBUG_EXCEPTION("vconf_set_int failed");
		err = EMF_ERROR_GCONF_FAILURE;
		goto FINISH_OFF;
	}
	ret = true;
	*handle = next_handle;
	EM_DEBUG_LOG(">>>>>> return next handle[%d]", *handle);
	
FINISH_OFF:
	if ( error != NULL ) {
		*error = err;
	}
	
	return ret;
}

static int email_send_notification_to_active_sync_engine(int subType, ASNotiData *data)
{
	EM_DEBUG_FUNC_BEGIN("subType [%d], data [%p]", subType, data);

	DBusConnection     *connection;
	DBusMessage        *signal = NULL;
	DBusError           error;
	const char         *nullString = "";
	int                 i = 0;
	dbus_int32_t        array_for_time_type[9] = { 0 , };

	dbus_error_init (&error);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

	if(connection == NULL)
		goto FINISH_OFF;

	signal = dbus_message_new_signal("/User/Email/ActiveSync", EMF_ACTIVE_SYNC_NOTI, "email");

	dbus_message_append_args(signal, DBUS_TYPE_INT32, &subType, DBUS_TYPE_INVALID);
	switch ( subType ) {
		case ACTIVE_SYNC_NOTI_SEND_MAIL:
			EM_DEBUG_LOG("handle:[%d]", data->send_mail.handle);
			EM_DEBUG_LOG("account_id:[%d]", data->send_mail.account_id);
			EM_DEBUG_LOG("mailbox_name:[%s]", data->send_mail.mailbox_name);
			EM_DEBUG_LOG("mail_id:[%d]", data->send_mail.mail_id);
			EM_DEBUG_LOG("options.priority:[%d]", data->send_mail.options.priority);
			EM_DEBUG_LOG("options.keep_local_copy:[%d]", data->send_mail.options.keep_local_copy);
			EM_DEBUG_LOG("options.req_delivery_receipt:[%d]", data->send_mail.options.req_delivery_receipt);
			EM_DEBUG_LOG("options.req_read_receipt:[%d]", data->send_mail.options.req_read_receipt);
			/* download_limit, block_address, block_subject might not be needed */
			EM_DEBUG_LOG("options.download_limit:[%d]", data->send_mail.options.download_limit);
			EM_DEBUG_LOG("options.block_address:[%d]", data->send_mail.options.block_address);
			EM_DEBUG_LOG("options.block_subject:[%d]", data->send_mail.options.block_subject);
			EM_DEBUG_LOG("options.display_name_from:[%s]", data->send_mail.options.display_name_from);
			EM_DEBUG_LOG("options.reply_with_body:[%d]", data->send_mail.options.reply_with_body);
			EM_DEBUG_LOG("options.forward_with_files:[%d]", data->send_mail.options.forward_with_files);
			EM_DEBUG_LOG("options.add_myname_card:[%d]", data->send_mail.options.add_myname_card);
			EM_DEBUG_LOG("options.add_signature:[%d]", data->send_mail.options.add_signature);
			EM_DEBUG_LOG("options.signature:[%s]", data->send_mail.options.signature);

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.handle), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.account_id), DBUS_TYPE_INVALID);
			if ( data->send_mail.mailbox_name == NULL )
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(nullString), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->send_mail.mailbox_name), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.mail_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.priority), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.keep_local_copy), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.req_delivery_receipt), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.req_read_receipt), DBUS_TYPE_INVALID);
			if ( data->send_mail.options.display_name_from == NULL )
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(nullString), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->send_mail.options.display_name_from), DBUS_TYPE_INVALID);

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.reply_with_body), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.forward_with_files), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.add_myname_card), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.add_signature), DBUS_TYPE_INVALID);
			if ( data->send_mail.options.signature == NULL )
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(nullString), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->send_mail.options.signature), DBUS_TYPE_INVALID);

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
			EM_DEBUG_LOG("mailbox_name:[%s]", data->sync_header.mailbox_name);
			
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->sync_header.handle ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->sync_header.account_id ), DBUS_TYPE_INVALID);
			if ( data->sync_header.mailbox_name == NULL )
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(nullString), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->sync_header.mailbox_name), DBUS_TYPE_INVALID);
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
			break;
		case ACTIVE_SYNC_NOTI_DOWNLOAD_ATTACHMENT:
			EM_DEBUG_LOG("handle:[%d]", data->download_attachment.handle);
			EM_DEBUG_LOG("account_id:[%d]", data->download_attachment.account_id );
			EM_DEBUG_LOG("mail_id:[%d]", data->download_attachment.mail_id);
			EM_DEBUG_LOG("with_attachment:[%s]", data->download_attachment.attachment_order );

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_attachment.handle  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_attachment.account_id  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->download_attachment.mail_id ), DBUS_TYPE_INVALID);
			if ( data->download_attachment.attachment_order == NULL )
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(nullString), DBUS_TYPE_INVALID);
			else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->download_attachment.attachment_order), DBUS_TYPE_INVALID);
			break;
		case ACTIVE_SYNC_NOTI_VALIDATE_ACCOUNT:
			EM_DEBUG_EXCEPTION("Not support yet : subType[ACTIVE_SYNC_NOTI_VALIDATE_ACCOUNT]", subType);
			break;
		case ACTIVE_SYNC_NOTI_CANCEL_JOB:
			EM_DEBUG_LOG("account_id:[%d]",       data->cancel_job.account_id );
			EM_DEBUG_LOG("handle to cancel:[%d]", data->cancel_job.handle);

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->cancel_job.account_id  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->cancel_job.handle  ), DBUS_TYPE_INVALID);
			break;
		case ACTIVE_SYNC_NOTI_SEARCH_ON_SERVER:
			EM_DEBUG_LOG("account_id:[%d]",          data->search_mail_on_server.account_id );
			EM_DEBUG_LOG("mailbox_name:[%s]",        data->search_mail_on_server.mailbox_name );
			EM_DEBUG_LOG("search_filter_count:[%d]", data->search_mail_on_server.search_filter_count );
			EM_DEBUG_LOG("handle to cancel:[%d]",    data->search_mail_on_server.handle);

			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->search_mail_on_server.account_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->search_mail_on_server.mailbox_name), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->search_mail_on_server.search_filter_count), DBUS_TYPE_INVALID);
			for(i = 0; i < data->search_mail_on_server.search_filter_count; i++) {
				dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->search_mail_on_server.search_filter_list[i].search_filter_type), DBUS_TYPE_INVALID);
				switch(data->search_mail_on_server.search_filter_list[i].search_filter_type) {
					case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       :
					case EMAIL_SEARCH_FILTER_TYPE_UID              :
					case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      :
					case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     :
					case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       :
						dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->search_mail_on_server.search_filter_list[i].search_filter_key_value.integer_type_key_value), DBUS_TYPE_INVALID);
						break;

					case EMAIL_SEARCH_FILTER_TYPE_BCC              :
					case EMAIL_SEARCH_FILTER_TYPE_CC               :
					case EMAIL_SEARCH_FILTER_TYPE_FROM             :
					case EMAIL_SEARCH_FILTER_TYPE_KEYWORD          :
					case EMAIL_SEARCH_FILTER_TYPE_SUBJECT          :
					case EMAIL_SEARCH_FILTER_TYPE_TO               :
					case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       :
						dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->search_mail_on_server.search_filter_list[i].search_filter_key_value.string_type_key_value), DBUS_TYPE_INVALID);
						break;

					case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE :
					case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     :
					case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  : {
							array_for_time_type[0] = data->search_mail_on_server.search_filter_list[i].search_filter_key_value.time_type_key_value.tm_sec;
							array_for_time_type[1] = data->search_mail_on_server.search_filter_list[i].search_filter_key_value.time_type_key_value.tm_min;
							array_for_time_type[2] = data->search_mail_on_server.search_filter_list[i].search_filter_key_value.time_type_key_value.tm_hour;
							array_for_time_type[3] = data->search_mail_on_server.search_filter_list[i].search_filter_key_value.time_type_key_value.tm_mday;
							array_for_time_type[4] = data->search_mail_on_server.search_filter_list[i].search_filter_key_value.time_type_key_value.tm_mon;
							array_for_time_type[5] = data->search_mail_on_server.search_filter_list[i].search_filter_key_value.time_type_key_value.tm_year;
							array_for_time_type[6] = data->search_mail_on_server.search_filter_list[i].search_filter_key_value.time_type_key_value.tm_wday;
							array_for_time_type[7] = data->search_mail_on_server.search_filter_list[i].search_filter_key_value.time_type_key_value.tm_yday;
							array_for_time_type[8] = data->search_mail_on_server.search_filter_list[i].search_filter_key_value.time_type_key_value.tm_isdst;
							dbus_message_append_args(signal, DBUS_TYPE_ARRAY, DBUS_TYPE_INT32, array_for_time_type, 9, DBUS_TYPE_INVALID);
						}
						break;
					default :
						EM_DEBUG_EXCEPTION("Invalid filter type [%d]", data->search_mail_on_server.search_filter_list[i].search_filter_type);
						break;
				}
			}
			dbus_message_append_args(signal, DBUS_TYPE_INT32,  &(data->search_mail_on_server.handle), DBUS_TYPE_INVALID);
			break;
		default:
			EM_DEBUG_EXCEPTION("Invalid Notification type of Active Sync : subType[%d]", subType);
			return FAILURE;
	}

	if(!dbus_connection_send (connection, signal, NULL)) {
		EM_DEBUG_EXCEPTION("dbus_connection_send is failed");
		return FAILURE;
	} else
		EM_DEBUG_LOG("dbus_connection_send is successful");

	dbus_connection_flush(connection);

FINISH_OFF:
	
	if(signal)
		dbus_message_unref(signal);

	EM_DEBUG_FUNC_END();
	return true;
}
#endif	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__ */


EXPORT_API int email_send_mail( emf_mailbox_t* mailbox, int mail_id, emf_option_t* sending_option, unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], sending_option[%p], handle[%p]", mailbox, mail_id, sending_option, handle);
	
	char* mailbox_stream = NULL;
	char* pSendingOption = NULL;
	int size = 0;
	int err = EMF_ERROR_NONE;
		
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(mailbox->name, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(mailbox->account_id, EMF_ERROR_INVALID_PARAM);

	EM_DEBUG_LOG("Account ID [ %d],mailbox->name[%s], mailbox->account_id[%d] ", mailbox->account_id, mailbox->name, mailbox->account_id);

#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
	emf_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	/*  check account bind type and branch off  */
	if ( email_get_account_server_type_by_account_id(mailbox->account_id, &account_server_type, false, &err) == false ) {
		EM_DEBUG_EXCEPTION("email_get_account_server_type_by_account_id failed[%d]", err);
		err = EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
		goto FINISH_OFF;
	}

	if ( account_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle;
		if ( email_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("email_get_handle_for_activesync failed[%d].", err);
			err = EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}
		
		/*  noti to active sync */
		as_noti_data.send_mail.handle = as_handle;
		as_noti_data.send_mail.account_id = mailbox->account_id;
		as_noti_data.send_mail.mailbox_name = strdup(mailbox->name);
		as_noti_data.send_mail.mail_id = mail_id;

		memcpy(&as_noti_data.send_mail.options, sending_option, sizeof(emf_option_t));

		if ( email_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SEND_MAIL, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("email_send_notification_to_active_sync_engine failed.");
			err = EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(handle)
			*handle = as_handle;
	} else {		
		hAPI = emipc_create_email_api(_EMAIL_API_SEND_MAIL);	

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		/* Mailbox */
		mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

		EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)){
			EM_DEBUG_EXCEPTION("email_send_mail--Add Param mailbox failed");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}
		
		/* mail_id */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))){
			EM_DEBUG_EXCEPTION("email_send_mail--Add Param mail_id failed");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		/* sending options */
		pSendingOption = em_convert_option_to_byte_stream(sending_option, &size);

		if ( NULL == pSendingOption)	 {
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, pSendingOption, size)){
			EM_DEBUG_EXCEPTION("email_send_mail--Add Param Sending_Option failed  ");
			EM_SAFE_FREE(mailbox_stream);
			EM_SAFE_FREE(pSendingOption);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("email_send_mail--emipc_execute_proxy_api failed  ");
			EM_SAFE_FREE(pSendingOption); 
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_CRASH);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		if (err == EMF_ERROR_NONE) {
			if(handle)
				emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
	 	}
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = (HIPC_API)NULL;
	EM_SAFE_FREE(pSendingOption); 
	EM_SAFE_FREE(mailbox_stream);
	EM_SAFE_FREE(as_noti_data.send_mail.mailbox_name);
#else	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__ */
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SEND_MAIL);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* Mailbox */
	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)){
		EM_DEBUG_EXCEPTION("Add Param mailbox failed  ");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}
	
	/* mail_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))){
		EM_DEBUG_EXCEPTION("Add Param mail_id failed  ");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* sending options */
	pSendingOption = em_convert_option_to_byte_stream(sending_option, &size);

	if ( NULL == pSendingOption)
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, pSendingOption, size)){
		EM_DEBUG_EXCEPTION("Add Param Sending_Option failed  ");
		EM_SAFE_FREE(mailbox_stream);
		EM_SAFE_FREE(pSendingOption);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!emipc_execute_proxy_api(hAPI, NULL)) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed  ");
		EM_SAFE_FREE(pSendingOption);
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

 	err  emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), );
	if (err == EMF_ERROR_NONE)
 	{
		if(handle)
			handle = *(unsigned int*)emipc_get_parameter(hAPI, ePARAMETER_OUT, 1);  /* Warning removal changes  */
 	}
	 	
	EM_DEBUG_LOG(" >>>>>> ERROR CODE : %d ", err);

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(pSendingOption);
	EM_SAFE_FREE(mailbox_stream);
#endif	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__ */
	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;	
}

EXPORT_API int email_send_saved(int account_id, emf_option_t* sending_option, unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d], sending_option[%p], handle[%p]", account_id, sending_option, handle);
	
	char* pOptionStream = NULL;
	int err = EMF_ERROR_NONE;
	int size = 0;
	
	EM_IF_NULL_RETURN_VALUE(account_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(sending_option, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(account_id, EMF_ERROR_INVALID_PARAM);
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SEND_SAVED);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);
	
	/* Account ID */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(account_id), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter account_id failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* Sending Option */
	pOptionStream = em_convert_option_to_byte_stream(sending_option, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(pOptionStream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, pOptionStream, size)) {
		EM_DEBUG_EXCEPTION("Add Param sending option failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_SAFE_FREE(pOptionStream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	
	emipc_destroy_email_api(hAPI);
	
	hAPI = NULL;
	EM_SAFE_FREE(pOptionStream);

	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;
}

EXPORT_API int email_sync_local_activity(int account_id)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d]", account_id);
	
	int err = EMF_ERROR_NONE;

	if (account_id < 0 || account_id == 0)  {		
		EM_DEBUG_EXCEPTION("account_id[%d]", account_id);
		
		return EMF_ERROR_INVALID_PARAM;		
	}
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SYNC_LOCAL_ACTIVITY);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}
	
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	emipc_destroy_email_api(hAPI);

	hAPI = NULL;
	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;
}

int EmfMailboxSyncHeader(emf_mailbox_t* mailbox, unsigned* handle, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], handle[%p], err_code[%p]", mailbox, handle, err_code);
	int err = EMF_ERROR_NONE;

	err = email_sync_header(mailbox,handle);

	if (err_code != NULL)
		*err_code = err;
	
    	return (err >= 0);
}

EXPORT_API int email_sync_header(emf_mailbox_t* mailbox, unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], handle[%p]", mailbox, handle);
	char* mailbox_stream = NULL;
	int err = EMF_ERROR_NONE;	
	int size = 0;
	/* int total_count = 0; */
	
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(mailbox->account_id, EMF_ERROR_INVALID_PARAM);

#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
	emf_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	/*  2010/02/12 ch715.lee : check account bind type and branch off  */
	if ( email_get_account_server_type_by_account_id(mailbox->account_id, &account_server_type, true, &err) == false ) {
		EM_DEBUG_EXCEPTION("email_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( account_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle;
		if ( email_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("email_get_handle_for_activesync failed[%d].", err);
			goto FINISH_OFF;
		}
		
		/*  noti to active sync */
		as_noti_data.sync_header.handle = as_handle;
		as_noti_data.sync_header.account_id = mailbox->account_id;
		/* In case that Mailbox is NULL,   SYNC ALL MAILBOX */
		as_noti_data.sync_header.mailbox_name = (mailbox && mailbox->name) ? strdup(mailbox->name) : NULL;

		if ( email_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SYNC_HEADER, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("email_send_notification_to_active_sync_engine failed.");
			err = EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(handle)
			*handle = as_handle;

	}
	else {
		hAPI = emipc_create_email_api(_EMAIL_API_SYNC_HEADER);	

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

		EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		}
			
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	

		if (err != EMF_ERROR_NONE)
			goto FINISH_OFF;

		if(handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);	
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);
	EM_SAFE_FREE(as_noti_data.sync_header.mailbox_name);
#else	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_SYNC_HEADER);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}
		
	 emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	

	 EM_DEBUG_LOG("RETURN VALUE : %d ", err);
	 
	  if (err != EMF_ERROR_NONE)
		 goto FINISH_OFF;
	 
	 if(handle)
	 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);	
FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);
#endif	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__ */

	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;
}


EXPORT_API int email_sync_header_for_all_account(unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN("handle[%p]", handle);
	char* mailbox_stream = NULL;
	emf_mailbox_t mailbox;
	int err = EMF_ERROR_NONE;	
	int size = 0;
	HIPC_API hAPI = NULL;
	int return_handle;
#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
	ASNotiData as_noti_data;
	int i, account_count = 0;
	emstorage_account_tbl_t *account_tbl_array = NULL;
	int as_err;
#endif

	hAPI = emipc_create_email_api(_EMAIL_API_SYNC_HEADER);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	mailbox.account_id = ALL_ACCOUNT;
	mailbox.name = NULL;
	mailbox.alias = NULL;
	mailbox.account_name = NULL;

	mailbox_stream = em_convert_mailbox_to_byte_stream(&mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}
		
	 emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	

	 if (err != EMF_ERROR_NONE)
		 goto FINISH_OFF;

	   emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), &return_handle);

#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	/*  Get all accounts for sending notification to active sync engine. */
	if (!emstorage_get_account_list(&account_count, &account_tbl_array , true, false, &as_err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_list failed [ %d ]  ", as_err);

		goto FINISH_OFF;
	}

	for(i = 0; i < account_count; i++) {
		if ( account_tbl_array[i].receiving_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC ) {
			/*  use returned handle value for a active sync handle */
			/* int as_handle; */
			/*
			if ( email_get_handle_for_activesync(&as_handle, &err) == false ) {
				EM_DEBUG_EXCEPTION("email_get_handle_for_activesync failed[%d].", err);
				goto FINISH_OFF;
			}
			*/
			
			/*  noti to active sync */
			as_noti_data.sync_header.handle = return_handle;
			as_noti_data.sync_header.account_id = account_tbl_array[i].account_id;
			/* In case that Mailbox is NULL,   SYNC ALL MAILBOX */
			as_noti_data.sync_header.mailbox_name = NULL;

			if ( email_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SYNC_HEADER, &as_noti_data) == false) {
				EM_DEBUG_EXCEPTION("email_send_notification_to_active_sync_engine failed.");
				err = EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
				goto FINISH_OFF;
			}
		}
	}
#endif	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__ */

	 
	 if(handle)
		*handle = return_handle;

FINISH_OFF:

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);
	if ( account_tbl_array )
		emstorage_free_account(&account_tbl_array, account_count, NULL);
	
	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;
}

EXPORT_API int email_download_body(emf_mailbox_t* mailbox, int mail_id, int with_attachment, unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d],with_attachment[%d]", mailbox, mail_id, with_attachment);
	char* mailbox_stream = NULL;
	int err = EMF_ERROR_NONE;
	int size = 0;
		
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(mail_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(mailbox->account_id, EMF_ERROR_INVALID_PARAM);
		
#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
	emf_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	/*  2010/02/12 ch715.lee : check account bind type and branch off  */
	if ( email_get_account_server_type_by_account_id(mailbox->account_id, &account_server_type, true, &err) == false ) {
		EM_DEBUG_EXCEPTION("email_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( account_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle;
		if ( email_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("email_get_handle_for_activesync failed[%d].", err);
			goto FINISH_OFF;
		}
		
		/*  noti to active sync */
		as_noti_data.download_body.handle = as_handle;
		as_noti_data.download_body.account_id = mailbox->account_id;
		as_noti_data.download_body.mail_id = mail_id;
		as_noti_data.download_body.with_attachment = with_attachment;

		if ( email_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_DOWNLOAD_BODY, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("email_send_notification_to_active_sync_engine failed.");
			err = EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(handle)
			*handle = as_handle;
	} else {
		hAPI = emipc_create_email_api(_EMAIL_API_DOWNLOAD_BODY);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

		EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

		/* MailBox Information */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		/* Mail Id */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		/* with_attachment */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&with_attachment, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}
		
		if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		}
			
		 emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	
		 if (err != EMF_ERROR_NONE)		
			 goto FINISH_OFF;
		 
		 if(handle)	
		 {
		 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
			EM_DEBUG_LOG("RETURN VALUE : %d  handle %d", err, *handle);

		 }
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);
#else	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_DOWNLOAD_BODY);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	/* MailBox Information */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* Mail Id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&mail_id, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* with_attachment */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&with_attachment, size)) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}
	
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}
		
	 emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	
	 if (err != EMF_ERROR_NONE)	 
		 goto FINISH_OFF;
	 
	 /* Download handle */
	 if(handle) {
	 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
		EM_DEBUG_LOG("RETURN VALUE : %d handle %d", err, *handle);

	 }
FINISH_OFF:
		 	
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);

#endif	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */

	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;

}





/* API - Downloads the Email Attachment Information [ INTERNAL ] */

EXPORT_API int email_download_attachment(emf_mailbox_t* mailbox,int mail_id, const char* nth,unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN("mailbox[%p], mail_id[%d], nth[%p], handle[%p]", mailbox, mail_id, nth, handle);
	char* mailbox_stream = NULL;
	int err = EMF_ERROR_NONE;
	int size = 0;
		
	EM_IF_NULL_RETURN_VALUE(mailbox, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(mail_id, EMF_ERROR_INVALID_PARAM);
	EM_IF_ACCOUNT_ID_NULL(mailbox->account_id, EMF_ERROR_INVALID_PARAM);
	/* EM_IF_NULL_RETURN_VALUE(nth, EMF_ERROR_INVALID_PARAM); */
	
#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
	emf_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	if ( email_get_account_server_type_by_account_id(mailbox->account_id, &account_server_type, true, &err) == false ) {
		EM_DEBUG_EXCEPTION("email_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( account_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle;
		if ( email_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("email_get_handle_for_activesync failed[%d].", err);
			goto FINISH_OFF;
		}
		
		/*  noti to active sync */
		as_noti_data.download_attachment.handle = as_handle;
		as_noti_data.download_attachment.account_id = mailbox->account_id;
		as_noti_data.download_attachment.mail_id = mail_id;
		as_noti_data.download_attachment.attachment_order = strdup(nth);
		if ( email_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_DOWNLOAD_ATTACHMENT, &as_noti_data) == false) {
			EM_DEBUG_EXCEPTION("email_send_notification_to_active_sync_engine failed.");
			err = EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(handle)
			*handle = as_handle;
	} else {
		hAPI = emipc_create_email_api(_EMAIL_API_DOWNLOAD_ATTACHMENT);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		/* Mailbox */
		mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

		EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);
		
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		/* Mail ID */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter mail_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		/* nth */
		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*) nth, sizeof(nth))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter mail_id failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		/* Execute API */
		if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		if (err != EMF_ERROR_NONE)		
			goto FINISH_OFF;
		 
		if(handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
		
	}
	
FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);
	EM_SAFE_FREE(as_noti_data.download_attachment.attachment_order);
#else	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_DOWNLOAD_ATTACHMENT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* Mailbox */
	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);
	
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, mailbox_stream, size)) {
		EM_DEBUG_EXCEPTION("Add Param mailbox failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* Mail ID */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter mail_id failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* nth */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, nth, sizeof(nth))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter mail_id failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("EmfDownloadAttachment--emipc_execute_proxy_api failed");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	

	EM_DEBUG_LOG(" >>>>> EmfDownloadAttachment RETURN VALUE : %d ", err);

 	 if (err != EMF_ERROR_NONE)		
		 goto FINISH_OFF;
	 
	 if(handle)
	 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);
	
	/*  Prevent defect 36700 */
	/* EM_DEBUG_LOG(" >>>>> Handle_proxy : %d ", *handle); */
FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);
#endif	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */

	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;
	
}


EXPORT_API int email_cancel_job(int account_id, int handle)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d], handle [%d]", account_id, handle);
	int err = EMF_ERROR_NONE;

	if(account_id < 0)
		return EMF_ERROR_INVALID_PARAM;
#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
	emf_account_server_t account_server_type;
	HIPC_API hAPI = NULL;
	ASNotiData as_noti_data;
	emstorage_account_tbl_t *account_list = NULL;
	int i, account_count = 0;

	if ( account_id == ALL_ACCOUNT ) {	/*  this means that job is executed with all account */
		/*  Get all accounts for sending notification to active sync engine. */
		if (!emstorage_get_account_list(&account_count, &account_list , true, false, &err)) {
			EM_DEBUG_EXCEPTION("email_get_account_list-- Failed [ %d ]  ", err);
	
			goto FINISH_OFF;
		}

		for(i = 0; i < account_count; i++) {
			if ( email_get_account_server_type_by_account_id(account_list[i].account_id, &account_server_type, true, &err) == false ) {
				EM_DEBUG_EXCEPTION("email_get_account_server_type_by_account_id failed[%d]", err);
				goto FINISH_OFF;
			}

			if ( account_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC ) {
				memset(&as_noti_data, 0x00, sizeof(ASNotiData));
				as_noti_data.cancel_job.account_id = account_list[i].account_id;
				as_noti_data.cancel_job.handle = handle;

				if ( email_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_CANCEL_JOB, &as_noti_data) == false) {
					EM_DEBUG_EXCEPTION("email_send_notification_to_active_sync_engine failed.");
					err = EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
					goto FINISH_OFF;
				}
			}
		}

		/*  request canceling to stub */
		hAPI = emipc_create_email_api(_EMAIL_API_CANCEL_JOB);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int))) {		/*  account_id == 0 */
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &handle, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		/* Execute API */
		if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
		emipc_destroy_email_api(hAPI);
		hAPI = NULL;
	}
	else {
		if ( email_get_account_server_type_by_account_id(account_id, &account_server_type, true, &err) == false ) {
			EM_DEBUG_EXCEPTION("email_get_account_server_type_by_account_id failed[%d]", err);
			goto FINISH_OFF;
		}

		if ( account_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC ) {
			memset(&as_noti_data, 0x00, sizeof(ASNotiData));
			as_noti_data.cancel_job.account_id = account_id;
			as_noti_data.cancel_job.handle = handle;

			if ( email_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_CANCEL_JOB, &as_noti_data) == false) {
				EM_DEBUG_EXCEPTION("email_send_notification_to_active_sync_engine failed.");
				err = EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
				goto FINISH_OFF;
			}
		} else {
			hAPI = emipc_create_email_api(_EMAIL_API_CANCEL_JOB);

			EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

			if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int))) {
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
				EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
			}

			if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &handle, sizeof(int))) {
				EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
				EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
			}

			/* Execute API */
			if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
				EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
			}
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
			emipc_destroy_email_api(hAPI);
			hAPI = NULL;
		}
	}

FINISH_OFF:
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	if ( account_list )
		emstorage_free_account(&account_list, account_count, NULL);
		
#else	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_CANCEL_JOB);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &handle, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);	

	EM_DEBUG_LOG(" >>>>> EmfCancelJob RETURN VALUE : %d ", err);

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
#endif	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */

	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;
}



EXPORT_API int email_get_pending_job(emf_action_t action, int account_id, int mail_id, emf_event_status_type_t * status)
{
	EM_DEBUG_FUNC_BEGIN("action[%d], account_id[%d], mail_id[%d], status[%p]", action, account_id, mail_id, status);
	
	int err = EMF_ERROR_NONE;

	EM_IF_ACCOUNT_ID_NULL(account_id, EMF_ERROR_NULL_VALUE);
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_GET_PENDING_JOB);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &action, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter action failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter account_id failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, &mail_id, sizeof(int))) {
		EM_DEBUG_EXCEPTION("emipc_add_parameter account_id failed ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

	if(status) {
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), status);
		EM_DEBUG_LOG("status : %d ", *status);
	}
	
	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;

}



EXPORT_API int email_get_network_status(int* on_sending, int* on_receiving)
{
	EM_DEBUG_FUNC_BEGIN("on_sending[%p], on_receiving[%p]", on_sending, on_receiving);
	int err = EMF_ERROR_NONE;
	
	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_NETWORK_GET_STATUS);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* Execute API */
	if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
		emipc_destroy_email_api(hAPI);
		hAPI = NULL;
		err = EMF_ERROR_IPC_SOCKET_FAILURE ;
		EM_DEBUG_FUNC_END("err [%d]", err); return err;
	}

	if(on_sending)
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), on_sending );
	if(on_receiving)	
		emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), on_receiving);

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;
	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;
}

EXPORT_API int email_get_imap_mailbox_list(int account_id, const char* mailbox, unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMF_ERROR_NONE;

	if(account_id <= 0 || !mailbox) {
		EM_DEBUG_LOG("invalid parameters");
		return EMF_ERROR_INVALID_PARAM;
	}

	HIPC_API hAPI = emipc_create_email_api(_EMAIL_API_GET_IMAP_MAILBOX_LIST);	

	/* account_id */
	if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_LOG("emipc_add_parameter failed  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!emipc_execute_proxy_api(hAPI))  {
		EM_DEBUG_LOG("ipcProxy_ExecuteAsyncAPI failed");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

 	emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);
	if(handle)
	emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), handle);

	emipc_destroy_email_api(hAPI);
	hAPI = NULL;

	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;
}

EXPORT_API int email_search_mail_on_server(int input_account_id, const char *input_mailbox_name, email_search_filter_t *input_search_filter_list, int input_search_filter_count, unsigned *output_handle)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d], input_mailbox_name [%p], input_search_filter_list [%p], input_search_filter_count [%d], output_handle [%p]", input_account_id, input_mailbox_name, input_search_filter_list, input_search_filter_count, output_handle);

	int       err = EMF_ERROR_NONE;
	int       return_value = 0;
	int       stream_size_for_search_filter_list = 0;
	char     *stream_for_search_filter_list = NULL;
	HIPC_API  hAPI = NULL;

	EM_IF_NULL_RETURN_VALUE(input_account_id,         EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mailbox_name,       EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_search_filter_list, EMF_ERROR_INVALID_PARAM);

#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
	emf_account_server_t account_server_type = EMF_SERVER_TYPE_NONE;
	ASNotiData as_noti_data;

	memset(&as_noti_data, 0, sizeof(ASNotiData)); /* initialization of union members */

	if ( email_get_account_server_type_by_account_id(input_account_id, &account_server_type, true, &err) == false ) {
		EM_DEBUG_EXCEPTION("email_get_account_server_type_by_account_id failed[%d]", err);
		goto FINISH_OFF;
	}

	if ( account_server_type == EMF_SERVER_TYPE_ACTIVE_SYNC ) {
		int as_handle = 0;

		if ( email_get_handle_for_activesync(&as_handle, &err) == false ) {
			EM_DEBUG_EXCEPTION("email_get_handle_for_activesync failed[%d].", err);
			goto FINISH_OFF;
		}

		/*  noti to active sync */
		as_noti_data.search_mail_on_server.handle              = as_handle;
		as_noti_data.search_mail_on_server.account_id          = input_account_id;
		as_noti_data.search_mail_on_server.mailbox_name        = EM_SAFE_STRDUP(input_mailbox_name);
		as_noti_data.search_mail_on_server.search_filter_list  = input_search_filter_list;
		as_noti_data.search_mail_on_server.search_filter_count = input_search_filter_count;

		return_value = email_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SEARCH_ON_SERVER, &as_noti_data);

		EM_SAFE_FREE(as_noti_data.search_mail_on_server.mailbox_name);

		if ( return_value == false ) {
			EM_DEBUG_EXCEPTION("email_send_notification_to_active_sync_engine failed.");
			err = EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE;
			goto FINISH_OFF;
		}

		if(output_handle)
			*output_handle = as_handle;
	}
	else
#endif  /*  __FEATURE_SUPPORT_ACTIVE_SYNC__	*/
	{
		hAPI = emipc_create_email_api(_EMAIL_API_SEARCH_MAIL_ON_SERVER);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (void*)&input_account_id, sizeof(int))) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			err = EMF_ERROR_IPC_PROTOCOL_FAILURE;
			goto FINISH_OFF;
		}

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, (void*)input_mailbox_name, strlen(input_mailbox_name))){
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			err = EMF_ERROR_IPC_PROTOCOL_FAILURE;
			goto FINISH_OFF;
		}

		stream_for_search_filter_list = em_convert_search_filter_to_byte_stream(input_search_filter_list, input_search_filter_count, &stream_size_for_search_filter_list);

		EM_PROXY_IF_NULL_RETURN_VALUE(stream_for_search_filter_list, hAPI, EMF_ERROR_NULL_VALUE);

		if(!emipc_add_parameter(hAPI, ePARAMETER_IN, stream_for_search_filter_list, stream_size_for_search_filter_list)) {
			EM_DEBUG_EXCEPTION("emipc_add_parameter failed  ");
			err = EMF_ERROR_IPC_PROTOCOL_FAILURE;
			goto FINISH_OFF;
		}

		if(emipc_execute_proxy_api(hAPI) != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emipc_execute_proxy_api failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		}

		emipc_get_parameter(hAPI, ePARAMETER_OUT, 0, sizeof(int), &err);

		if (err != EMF_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("_EMAIL_API_SEARCH_MAIL_ON_SERVER failed [%d]", err);
			goto FINISH_OFF;
		}

		if(output_handle)
			emipc_get_parameter(hAPI, ePARAMETER_OUT, 1, sizeof(int), output_handle);
	}

FINISH_OFF:
	if(hAPI) {
		emipc_destroy_email_api(hAPI);
		hAPI = NULL;
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
