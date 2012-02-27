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
 * to interact with Email Engine.
 * @file		Emf_Mapi_Network.c
 * @brief 		This file contains the data structures and interfaces of Network related Functionality provided by 
 *			Email Engine . 
 */
 
#include <Emf_Mapi.h>
#include "string.h"
#include "Msg_Convert.h"
#include "Emf_Mapi_Mailbox.h"
#include "emf-types.h"
#include "ipc-library.h"
#include "em-storage.h"

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
	int err= EMF_ERROR_NONE;
			
	if ( handle == NULL ) {
		EM_DEBUG_EXCEPTION("email_get_handle_for_activesync failed : handle is NULL");
		ret = false;
		goto FINISH_OFF;
	}
	
	if ( vconf_get_int(VCONFKEY_EMAIL_SERVICE_ACTIVE_SYNC_HANDLE, &next_handle)  != 0 ) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");

		if ( next_handle != 0 ) {
			ret = false;
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
		
		ret = false;
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
	EM_DEBUG_FUNC_BEGIN();

	DBusConnection *connection;
	DBusMessage    *signal = NULL;
	DBusError      error; 
	const char     *nullString = "";

	EM_DEBUG_LOG("Active Sync noti subType : [%d]", subType);

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
			/*  	download_limit, block_address, block_subject might not be needed */
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
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(nullString), DBUS_TYPE_INVALID); else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->send_mail.mailbox_name), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.mail_id), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.priority), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.keep_local_copy), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.req_delivery_receipt), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.req_read_receipt), DBUS_TYPE_INVALID);
			if ( data->send_mail.options.display_name_from == NULL )
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(nullString), DBUS_TYPE_INVALID); else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->send_mail.options.display_name_from), DBUS_TYPE_INVALID);

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.reply_with_body), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.forward_with_files), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.add_myname_card), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->send_mail.options.add_signature), DBUS_TYPE_INVALID);
			if ( data->send_mail.options.signature == NULL )
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(nullString), DBUS_TYPE_INVALID); else
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
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(nullString), DBUS_TYPE_INVALID); else
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
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(nullString), DBUS_TYPE_INVALID); else
				dbus_message_append_args(signal, DBUS_TYPE_STRING, &(data->download_attachment.attachment_order), DBUS_TYPE_INVALID);
			break;
		case ACTIVE_SYNC_NOTI_VALIDATE_ACCOUNT:
			EM_DEBUG_EXCEPTION("Not support yet : subType[ACTIVE_SYNC_NOTI_VALIDATE_ACCOUNT]", subType);
			break;
		case ACTIVE_SYNC_NOTI_CANCEL_JOB:
			EM_DEBUG_LOG("account_id:[%d]", data->cancel_job.account_id );
			EM_DEBUG_LOG("handle to cancel:[%d]", data->cancel_job.handle);

			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->cancel_job.account_id  ), DBUS_TYPE_INVALID);
			dbus_message_append_args(signal, DBUS_TYPE_INT32, &(data->cancel_job.handle  ), DBUS_TYPE_INVALID);
			break;
		case ACTIVE_SYNC_NOTI_SEARCH_ON_SERVER:
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
		hAPI = ipcEmailAPI_Create(_EMAIL_API_SEND_MAIL);	

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		/* Mailbox */
		mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

		EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, mailbox_stream, size)){
			EM_DEBUG_EXCEPTION("email_send_mail--Add Param mailbox Fail");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}
		
		/* mail_id */
		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))){
			EM_DEBUG_EXCEPTION("email_send_mail--Add Param mail_id Fail");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		/* sending options */
		pSendingOption = em_convert_option_to_byte_stream(sending_option, &size);

		if ( NULL == pSendingOption)	 {
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, pSendingOption, size)){
			EM_DEBUG_EXCEPTION("email_send_mail--Add Param Sending_Option Fail  ");
			EM_SAFE_FREE(mailbox_stream);
			EM_SAFE_FREE(pSendingOption);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
		}

		if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
			EM_DEBUG_EXCEPTION("email_send_mail--ipcEmailProxy_ExecuteAPI Fail  ");
			EM_SAFE_FREE(pSendingOption); 
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_CRASH);
		}

	 	err = *((int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0));
		if (err == EMF_ERROR_NONE)
	 	{
			if(handle)
				*handle = *(unsigned int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1); 
	 	}
	}

FINISH_OFF:
	ipcEmailAPI_Destroy(hAPI);
	hAPI = (HIPC_API)NULL;
	EM_SAFE_FREE(pSendingOption); 
	EM_SAFE_FREE(mailbox_stream);
	EM_SAFE_FREE(as_noti_data.send_mail.mailbox_name);
#else	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__ */
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_SEND_MAIL);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* Mailbox */
	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, mailbox_stream, size)){
		EM_DEBUG_EXCEPTION("Add Param mailbox Fail  ");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}
	
	/* mail_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int))){
		EM_DEBUG_EXCEPTION("Add Param mail_id Fail  ");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	/* sending options */
	pSendingOption = em_convert_option_to_byte_stream(sending_option, &size);

	if ( NULL == pSendingOption)
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, pSendingOption, size)){
		EM_DEBUG_EXCEPTION("Add Param Sending_Option Fail  ");
		EM_SAFE_FREE(mailbox_stream);
		EM_SAFE_FREE(pSendingOption);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!ipcEmailProxy_ExecuteAPI(hAPI, NULL)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI Fail  ");
		EM_SAFE_FREE(pSendingOption);
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

 	err  = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
	if (err == EMF_ERROR_NONE)
 	{
		if(handle)
			handle = *(unsigned int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);  /* Warning removal changes  */
 	}
	 	
	EM_DEBUG_LOG(" >>>>>> ERROR CODE : %d ", err);

	ipcEmailAPI_Destroy(hAPI);
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
	
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_MAIL_SEND_SAVED);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);
	
	/* Account ID */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &(account_id), sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter account_id Fail");

	/* Sending Option */
	pOptionStream = em_convert_option_to_byte_stream(sending_option, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(pOptionStream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, pOptionStream, size))
		EM_DEBUG_EXCEPTION("Add Param sending option Fail");
	
	/* Execute API */
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail");
		EM_SAFE_FREE(pOptionStream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);	
	ipcEmailAPI_Destroy(hAPI);
	
	hAPI = NULL;
	EM_SAFE_FREE(pOptionStream);

	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;
}

EXPORT_API int email_send_report(emf_mail_t* mail,  unsigned* handle)
{
	EM_DEBUG_FUNC_BEGIN("mail[%p], handle[%p]", mail, handle);
	
	char* pMailHeadStream =  NULL;
	char* pMailBodyStream =  NULL;
	char* pMailInfoStream =  NULL;
	int size = 0;
	int err = EMF_ERROR_NONE;

	if (!mail || !mail->info || mail->info->account_id <= 0 || !mail->head || 
		!mail->head->from || !mail->head->mid)  {
		if (mail != NULL)  {
			if (mail->info != NULL)  {
				if (mail->head != NULL)
					EM_DEBUG_LOG("mail->info->account_id[%d], mail->head->from[%p], mail->head->mid[%p]", mail->info->account_id, mail->head->from, mail->head->mid); else
					EM_DEBUG_LOG("mail->info->account_id[%d], mail->head[%p]", mail->info->account_id, mail->head);		
			} else  {
				if (mail->head != NULL)
					EM_DEBUG_LOG("mail->info[%p],  mail->head->from[%p], mail->head->mid[%p]", mail->info, mail->head->from, mail->head->mid); else
					EM_DEBUG_LOG("mail->info[%p],  mail->head[%p]", mail->info, mail->head);
			}
		} else
			EM_DEBUG_LOG("mail[%p]", mail);
		
		err = EMF_ERROR_INVALID_MAIL;		
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_MAIL");
		EM_DEBUG_FUNC_END("err [%d]", err); 
		return err;
	}
	
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_MAIL_SEND_REPORT);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* Head */
	pMailHeadStream = em_convert_mail_head_to_byte_stream(mail->head, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(pMailHeadStream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, pMailHeadStream, size))
		EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

	/* Body */
	pMailBodyStream = em_convert_mail_body_to_byte_stream(mail->body, &size);

	if(NULL == pMailBodyStream) {
		EM_SAFE_FREE(pMailHeadStream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, pMailBodyStream, size))
		EM_DEBUG_EXCEPTION("Add Param mail body Fail  ");

	/* Info */
	pMailInfoStream = em_convert_mail_info_to_byte_stream(mail->info, &size);

	/* EM_PROXY_IF_NULL_RETURN_VALUE(pMailInfoStream, hAPI, EMF_ERROR_NULL_VALUE); */
	if(NULL == pMailInfoStream) {
		EM_SAFE_FREE(pMailHeadStream);
		EM_SAFE_FREE(pMailBodyStream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, pMailInfoStream, size))
		EM_DEBUG_EXCEPTION("Add Param mail body Fail");

	/* Execute API */
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail");
		EM_SAFE_FREE(pMailInfoStream);
		EM_SAFE_FREE(pMailBodyStream);
		EM_SAFE_FREE(pMailHeadStream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);	

	EM_SAFE_FREE(pMailHeadStream);
	EM_SAFE_FREE(pMailBodyStream);
	EM_SAFE_FREE(pMailInfoStream);
		
	ipcEmailAPI_Destroy(hAPI);

	hAPI = NULL;

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
	
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_SYNC_LOCAL_ACTIVITY);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* account_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("Add Param account_id Fail");
		
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);

	ipcEmailAPI_Destroy(hAPI);

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
		hAPI = ipcEmailAPI_Create(_EMAIL_API_SYNC_HEADER);	

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

		EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, mailbox_stream, size))
			EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

		if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail  ");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		}
			
		err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);	

		if (err != EMF_ERROR_NONE)
			goto FINISH_OFF;

		if(handle)
			*handle = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);	
	}

FINISH_OFF:
	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);
	EM_SAFE_FREE(as_noti_data.sync_header.mailbox_name);
#else	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_SYNC_HEADER);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, mailbox_stream, size))
		EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail  ");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}
		
	 err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);	

	 EM_DEBUG_LOG(" >>>>>> RETURN VALUE : %d ", err);
	 
	  if (err != EMF_ERROR_NONE)
		 goto FINISH_OFF;
	 
	 if(handle)
	 	*handle = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);	
FINISH_OFF:
	ipcEmailAPI_Destroy(hAPI);
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
	emf_mail_account_tbl_t *account_tbl_array = NULL;
	int as_err;
#endif

	hAPI = ipcEmailAPI_Create(_EMAIL_API_SYNC_HEADER);	

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	mailbox.account_id = ALL_ACCOUNT;
	mailbox.name = NULL;
	mailbox.alias = NULL;
	mailbox.account_name = NULL;

	mailbox_stream = em_convert_mailbox_to_byte_stream(&mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, mailbox_stream, size))
		EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail  ");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}
		
	 err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);	

	 if (err != EMF_ERROR_NONE)
		 goto FINISH_OFF;

	  return_handle = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);

#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
	memset(&as_noti_data, 0x00, sizeof(ASNotiData));

	/*  Get all accounts for sending notification to active sync engine. */
	if (!em_storage_get_account_list(&account_count, &account_tbl_array , true, false, &as_err)) {
		EM_DEBUG_EXCEPTION("email_get_account_list-- Failed [ %d ]  ", as_err);
		err = em_storage_get_emf_error_from_em_storage_error(as_err);
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

	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);
	if ( account_tbl_array )
		em_storage_free_account(&account_tbl_array, account_count, NULL);
	
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
		hAPI = ipcEmailAPI_Create(_EMAIL_API_DOWNLOAD_BODY);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

		EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

		/* MailBox Information */
		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, mailbox_stream, size))
			EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

		/* Mail Id */
		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&mail_id, sizeof(int)))
			EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

		/* with_attachment */
		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&with_attachment, sizeof(int)))
			EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");
		
		if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail  ");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		}
			
		 err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);	
		 if (err != EMF_ERROR_NONE)		
			 goto FINISH_OFF;
		 
		 if(handle)	
		 {
		 	*handle = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
			EM_DEBUG_LOG(" >>>>>>>>>>>>>>>>>>> RETURN VALUE : %d  handle %d", err, *handle);

		 }
	}

FINISH_OFF:
	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);
#else	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_DOWNLOAD_BODY);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);

	/* MailBox Information */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, mailbox_stream, size))
		EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

	/* Mail Id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&mail_id, size))
		EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

	/* with_attachment */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&with_attachment, size))
		EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");
	
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail  ");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}
		
	 err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);	
	 if (err != EMF_ERROR_NONE)	 
		 goto FINISH_OFF;
	 
	 /* Download handle - 17-Apr-07 */
	 if(handle) {
	 	*handle = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
		EM_DEBUG_LOG(" >>>>>>>>>>>>>>>>>>> RETURN VALUE : %d  handle %d", err, *handle);

	 }
FINISH_OFF:
		 	
	ipcEmailAPI_Destroy(hAPI);
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
		hAPI = ipcEmailAPI_Create(_EMAIL_API_DOWNLOAD_ATTACHMENT);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		/* Mailbox */
		mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

		EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);
		
		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, mailbox_stream, size))
			EM_DEBUG_EXCEPTION("Add Param mailbox Fail  ");

		/* Mail ID */
		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter mail_id Fail ");

		/* nth */
		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*) nth, sizeof(nth)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter mail_id Fail ");
		
		/* Execute API */
		if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail  ");
			EM_SAFE_FREE(mailbox_stream);
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		}

		err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
		if (err != EMF_ERROR_NONE)		
			goto FINISH_OFF;
		 
		if(handle)
			*handle = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
		
	}
	
FINISH_OFF:
	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;
	EM_SAFE_FREE(mailbox_stream);
	EM_SAFE_FREE(as_noti_data.download_attachment.attachment_order);
#else	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_DOWNLOAD_ATTACHMENT);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* Mailbox */
	mailbox_stream = em_convert_mailbox_to_byte_stream(mailbox, &size);

	EM_PROXY_IF_NULL_RETURN_VALUE(mailbox_stream, hAPI, EMF_ERROR_NULL_VALUE);
	
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, mailbox_stream, size))
		EM_DEBUG_EXCEPTION("EmfDownloadAttachment--Add Param mailbox Fail  ");

	/* Mail ID */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &(mail_id), sizeof(int)))
			EM_DEBUG_EXCEPTION(" EmfDownloadAttachment ipcEmailAPI_AddParameter mail_idFail ");

	/* nth */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, nth, sizeof(nth)))
			EM_DEBUG_EXCEPTION(" EmfDownloadAttachment ipcEmailAPI_AddParameter mail_idFail ");

	/* Execute API */
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("EmfDownloadAttachment--ipcEmailProxy_ExecuteAPIFail  ");
		EM_SAFE_FREE(mailbox_stream);
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);	

	EM_DEBUG_LOG(" >>>>> EmfDownloadAttachment RETURN VALUE : %d ", err);

 	 if (err != EMF_ERROR_NONE)		
		 goto FINISH_OFF;
	 
	 if(handle)
	 	*handle = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
	
	/*  Prevent defect 36700 */
	/* EM_DEBUG_LOG(" >>>>> Handle_proxy : %d ", *handle); */
FINISH_OFF:
	ipcEmailAPI_Destroy(hAPI);
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
	emf_mail_account_tbl_t *account_list = NULL;
	int i, account_count = 0;
	void *return_from_ipc = NULL;

	if ( account_id == ALL_ACCOUNT ) {	/*  this means that job is executed with all account */
		/*  Get all accounts for sending notification to active sync engine. */
		if (!em_storage_get_account_list(&account_count, &account_list , true, false, &err)) {
			EM_DEBUG_EXCEPTION("email_get_account_list-- Failed [ %d ]  ", err);
			err = em_storage_get_emf_error_from_em_storage_error(err);
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
		hAPI = ipcEmailAPI_Create(_EMAIL_API_CANCEL_JOB);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int)))		/*  account_id == 0 */
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter account_id Fail ");

		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &handle, sizeof(int)))
			EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter handle Fail ");

		/* Execute API */
		if(ipcEmailProxy_ExecuteAPI(hAPI) < 0) {
			EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail  ");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		}

		return_from_ipc = ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
		if (return_from_ipc) {
			err = *(int *)return_from_ipc;
		}
		
		ipcEmailAPI_Destroy(hAPI);
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
			hAPI = ipcEmailAPI_Create(_EMAIL_API_CANCEL_JOB);

			EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

			if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int)))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter account_id Fail ");

			if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &handle, sizeof(int)))
				EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter handle Fail ");

			/* Execute API */
			if(ipcEmailProxy_ExecuteAPI(hAPI) < 0) {
				EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPIFail  ");
				EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
			}

			return_from_ipc = ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
			if (return_from_ipc) {
				err = *(int *)return_from_ipc;
			}	
			
			ipcEmailAPI_Destroy(hAPI);
			hAPI = NULL;

		}
	}

FINISH_OFF:
	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;
	if ( account_list )
		em_storage_free_account(&account_list, account_count, NULL);
		
#else	/*  __FEATURE_SUPPORT_ACTIVE_SYNC__		 */
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_CANCEL_JOB);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int)))
		EM_DEBUG_EXCEPTION(" EmfCancelJob ipcEmailAPI_AddParameter account_id Fail ");

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &handle, sizeof(int)))
		EM_DEBUG_EXCEPTION(" EmfCancelJob ipcEmailAPI_AddParameter handle Fail ");

	/* Execute API */
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI Fail");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);	

	EM_DEBUG_LOG(" >>>>> EmfCancelJob RETURN VALUE : %d ", err);

	ipcEmailAPI_Destroy(hAPI);
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
	
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_GET_PENDING_JOB);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &action, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter action Fail ");

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &account_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter account_id Fail ");

	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, &mail_id, sizeof(int)))
		EM_DEBUG_EXCEPTION("ipcEmailAPI_AddParameter account_id Fail ");

	/* Execute API */
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI Fail  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);

	if(status) {
		*status = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
		EM_DEBUG_LOG("status : %d ", *status);
	}
	
	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;

	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;

}



EXPORT_API int email_get_network_status(int* on_sending, int* on_receiving)
{
	EM_DEBUG_FUNC_BEGIN("on_sending[%p], on_receiving[%p]", on_sending, on_receiving);
	int err = EMF_ERROR_NONE;
	
	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_NETWORK_GET_STATUS);

	EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

	/* Execute API */
	if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
		EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI Fail");
		ipcEmailAPI_Destroy(hAPI);
		hAPI = NULL;
		err = EMF_ERROR_IPC_SOCKET_FAILURE ;
		EM_DEBUG_FUNC_END("err [%d]", err); return err;
	}

	if(on_sending)
		*on_sending = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
	if(on_receiving)	
		*on_receiving = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);

	ipcEmailAPI_Destroy(hAPI);
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

	HIPC_API hAPI = ipcEmailAPI_Create(_EMAIL_API_GET_IMAP_MAILBOX_LIST);	

	/* account_id */
	if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (char*)&account_id, sizeof(int))) {
		EM_DEBUG_LOG("Add Param account_id Fail  ");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_NULL_VALUE);
	}

	if(!ipcEmailProxy_ExecuteAPI(hAPI))  {
		EM_DEBUG_LOG("ipcProxy_ExecuteAsyncAPI Fail");
		EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
	}

 	err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);
	if(handle)
	*handle = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);

	ipcEmailAPI_Destroy(hAPI);
	hAPI = NULL;

	EM_DEBUG_FUNC_END("err [%d]", err);  
	return err;
}

EXPORT_API int email_find_mail_on_server(int input_account_id, const char *input_mailbox_name, int input_search_type, char *input_search_value, unsigned *output_handle)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id [%d], input_mailbox_name [%p], input_search_type [%d], input_search_value [%p], output_handle [%p]", input_account_id, input_mailbox_name, input_search_type, input_search_value, output_handle);

	int       err = EMF_ERROR_NONE;
	HIPC_API  hAPI = NULL;

	EM_IF_NULL_RETURN_VALUE(input_account_id,   EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_mailbox_name, EMF_ERROR_INVALID_PARAM);
	EM_IF_NULL_RETURN_VALUE(input_search_value, EMF_ERROR_INVALID_PARAM);

	switch ( input_search_type ) {
		case EMF_SEARCH_FILTER_SUBJECT:
		case EMF_SEARCH_FILTER_SENDER:
		case EMF_SEARCH_FILTER_RECIPIENT:
		case EMF_SEARCH_FILTER_ALL:
			break;
		default:
			EM_DEBUG_EXCEPTION("Invalid search filter type[%d]", input_search_type);
			err = EMF_ERROR_INVALID_PARAM;
			return err;
	}

#ifdef __FEATURE_SUPPORT_ACTIVE_SYNC__
	emf_account_server_t account_server_type = EMF_SERVER_TYPE_NONE;
	ASNotiData as_noti_data;

	memset(&as_noti_data, 0, sizeof(ASNotiData)); /* initialization of union members */

	if ( email_get_account_server_type_by_account_id(input_account_id, &account_server_type, true, &err) == false ) {
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
		as_noti_data.find_mail_on_server.handle       = as_handle;
		as_noti_data.find_mail_on_server.account_id   = input_account_id;
		as_noti_data.find_mail_on_server.mailbox_name = EM_SAFE_STRDUP(input_mailbox_name);
		as_noti_data.find_mail_on_server.search_type  = input_search_type;
		as_noti_data.find_mail_on_server.search_value = EM_SAFE_STRDUP(input_search_value);

		if ( email_send_notification_to_active_sync_engine(ACTIVE_SYNC_NOTI_SEARCH_ON_SERVER, &as_noti_data) == false) {
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
		hAPI = ipcEmailAPI_Create(_EMAIL_API_FIND_MAIL_ON_SERVER);

		EM_IF_NULL_RETURN_VALUE(hAPI, EMF_ERROR_NULL_VALUE);

		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (void*)&input_account_id, sizeof(int)))
			EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (void*)input_mailbox_name, strlen(input_mailbox_name)))
			EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (void*)&input_search_type, sizeof(int)))
			EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

		if(!ipcEmailAPI_AddParameter(hAPI, ePARAMETER_IN, (void*)input_search_value, strlen(input_search_value)))
			EM_DEBUG_EXCEPTION("Add Param mail head Fail  ");

		if(!ipcEmailProxy_ExecuteAPI(hAPI)) {
			EM_DEBUG_EXCEPTION("ipcEmailProxy_ExecuteAPI failed");
			EM_PROXY_IF_NULL_RETURN_VALUE(0, hAPI, EMF_ERROR_IPC_SOCKET_FAILURE);
		}

		err = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 0);

		if (err != EMF_ERROR_NONE)
			goto FINISH_OFF;

		if(output_handle)
			*output_handle = *(int*)ipcEmailAPI_GetParameter(hAPI, ePARAMETER_OUT, 1);
	}

FINISH_OFF:
	if(hAPI) {
		ipcEmailAPI_Destroy(hAPI);
		hAPI = NULL;
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
