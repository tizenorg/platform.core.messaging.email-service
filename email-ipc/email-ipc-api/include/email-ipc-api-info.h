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


#ifndef _IPC_APIINFO_H_
#define _IPC_APIINFO_H_

#include <stdio.h>

#include "email-ipc.h"
#include "email-ipc-api-info.h"
#include "email-ipc-param-list.h"

typedef struct {
	long api_id;
	long response_id;
	long app_id;
	emipc_param_list *params[2];
} emipc_email_api_info;

/*EXPORT_API bool emipc_set_app_id_of_api_info(emipc_email_api_info *api_info, long app_id); */

EXPORT_API bool emipc_deserialize_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction, void *stream);

EXPORT_API unsigned char *emipc_serialize_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction, int *stream_len);

/*EXPORT_API int emipc_get_stream_length_of_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction); */

EXPORT_API void *emipc_get_parameters_of_api_info(emipc_email_api_info *api_info, EPARAMETER_DIRECTION direction);

EXPORT_API bool emipc_free_api_info(emipc_email_api_info *api_info);

/* don't insert empty line */
#define EM_APIID_TO_STR(nAPIID) \
	({\
		int id=nAPIID;\
		const char* s=NULL;\
		switch(id) {\
			case _EMAIL_API_ADD_ACCOUNT:\
					s = "_EMAIL_API_ADD_ACCOUNT";\
					break;\
			case _EMAIL_API_DELETE_ACCOUNT:\
					s = "_EMAIL_API_DELETE_ACCOUNT";\
					break;\
			case _EMAIL_API_UPDATE_ACCOUNT:\
					s = "_EMAIL_API_UPDATE_ACCOUNT";\
					break;\
			case _EMAIL_API_ADD_MAILBOX:\
					s = "_EMAIL_API_ADD_MAILBOX";\
					break;\
			case _EMAIL_API_DELETE_MAILBOX:\
					s = "_EMAIL_API_DELETE_MAILBOX";\
					break;\
			case _EMAIL_API_UPDATE_MAILBOX:\
					s = "_EMAIL_API_UPDATE_MAILBOX";\
					break;\
			case _EMAIL_API_RENAME_MAILBOX:\
					s = "_EMAIL_API_RENAME_MAILBOX";\
					break;\
			case _EMAIL_API_SET_MAILBOX_TYPE:\
					s = "_EMAIL_API_SET_MAILBOX_TYPE";\
					break;\
			case _EMAIL_API_SET_MAIL_SLOT_SIZE:\
					s = "_EMAIL_API_SET_MAIL_SLOT_SIZE";\
					break;\
			case _EMAIL_API_SEND_MAIL:\
					s = "_EMAIL_API_SEND_MAIL";\
					break;\
			case _EMAIL_API_GET_MAILBOX_COUNT:\
					s = "_EMAIL_API_GET_MAILBOX_COUNT";\
					break;\
			case _EMAIL_API_GET_MAILBOX_LIST:\
					s = "_EMAIL_API_GET_MAILBOX_LIST";\
					break;\
			case _EMAIL_API_GET_SUBMAILBOX_LIST:\
					s = "_EMAIL_API_GET_SUBMAILBOX_LIST";\
					break;\
			case _EMAIL_API_SYNC_HEADER:\
					s = "_EMAIL_API_SYNC_HEADER";\
					break;\
			case _EMAIL_API_DOWNLOAD_BODY:\
					s = "_EMAIL_API_DOWNLOAD_BODY";\
					break;\
			case _EMAIL_API_CLEAR_DATA:\
					s = "_EMAIL_API_CLEAR_DATA";\
					break;\
			case _EMAIL_API_DELETE_ALL_MAIL:\
					s = "_EMAIL_API_DELETE_ALL_MAIL";\
					break;\
			case _EMAIL_API_DELETE_MAIL:\
					s = "_EMAIL_API_DELETE_MAIL";\
					break;\
			case _EMAIL_API_MODIFY_MAIL_FLAG:\
					s = "_EMAIL_API_MODIFY_MAIL_FLAG";\
					break;\
			case _EMAIL_API_MODIFY_MAIL_EXTRA_FLAG:\
					s = "_EMAIL_API_MODIFY_MAIL_EXTRA_FLAG";\
					break;\
	 		case _EMAIL_API_ADD_RULE:\
					s = "_EMAIL_API_ADD_RULE";\
					break;\
			case _EMAIL_API_GET_RULE:\
					s = "_EMAIL_API_GET_RULE";\
					break;\
			case _EMAIL_API_GET_RULE_LIST:\
					s = "_EMAIL_API_GET_RULE";\
					break;\
			case _EMAIL_API_FIND_RULE:\
					s = "_EMAIL_API_FIND_RULE";\
					break;\
			case _EMAIL_API_UPDATE_RULE:\
					s = "_EMAIL_API_UPDATE_RULE";\
					break;\
			case _EMAIL_API_DELETE_RULE:\
					s = "_EMAIL_API_DELETE_RULE";\
					break;\
			case _EMAIL_API_MOVE_MAIL:\
					s = "_EMAIL_API_MOVE_MAIL";\
					break;\
			case _EMAIL_API_MOVE_ALL_MAIL:\
					s = "_EMAIL_API_MOVE_ALL_MAIL";\
					break;\
			case _EMAIL_API_SET_FLAGS_FIELD:\
					s = "_EMAIL_API_SET_FLAGS_FIELD";\
					break;\
			case _EMAIL_API_ADD_MAIL:\
					s = "_EMAIL_API_ADD_MAIL";\
					break;\
			case _EMAIL_API_UPDATE_MAIL:\
					s = "_EMAIL_API_UPDATE_MAIL";\
					break;\
			case _EMAIL_API_MOVE_THREAD_TO_MAILBOX:\
					s = "_EMAIL_API_MOVE_THREAD_TO_MAILBOX";\
					break;\
			case _EMAIL_API_DELETE_THREAD:\
					s = "_EMAIL_API_DELETE_THREAD";\
					break;\
			case _EMAIL_API_MODIFY_SEEN_FLAG_OF_THREAD:\
					s = "_EMAIL_API_MODIFY_SEEN_FLAG_OF_THREAD";\
					break;\
			case _EMAIL_API_ADD_ATTACHMENT:\
					s = "_EMAIL_API_ADD_ATTACHMENT";\
					break;\
			case _EMAIL_API_GET_IMAP_MAILBOX_LIST:\
					s = "_EMAIL_API_GET_IMAP_MAILBOX_LIST";\
					break;\
			case _EMAIL_API_GET_ATTACHMENT:\
					s = "_EMAIL_API_GET_ATTACHMENT";\
					break;\
			case _EMAIL_API_DELETE_ATTACHMENT:\
					s = "_EMAIL_API_DELETE_ATTACHMENT";\
					break;\
			case _EMAIL_API_DOWNLOAD_ATTACHMENT:\
					s = "_EMAIL_API_DOWNLOAD_ATTACHMENT";\
					break;\
			case _EMAIL_API_GET_ACCOUNT_LIST:\
					s = "_EMAIL_API_GET_ACCOUNT_LIST";\
					break;\
			case _EMAIL_API_SEND_SAVED:\
					s = "_EMAIL_API_SEND_SAVED";\
					break;\
			case _EMAIL_API_CANCEL_JOB:\
					s = "_EMAIL_API_CANCEL_JOB";\
					break;\
			case _EMAIL_API_GET_PENDING_JOB:\
					s = "_EMAIL_API_GET_PENDING_JOB";\
					break;\
			case _EMAIL_API_NETWORK_GET_STATUS:\
					s = "_EMAIL_API_NETWORK_GET_STATUS";\
					break;\
			case _EMAIL_API_SEND_RETRY:\
					s = "_EMAIL_API_SEND_RETRY";\
					break;\
			case _EMAIL_API_VALIDATE_ACCOUNT :\
					s = "_EMAIL_API_VALIDATE_ACCOUNT";\
					break;\
			case _EMAIL_API_SEND_MAIL_CANCEL_JOB :\
					s = "_EMAIL_API_SEND_MAIL_CANCEL_JOB";\
					break;\
			case _EMAIL_API_SEARCH_MAIL_ON_SERVER :\
					s = "_EMAIL_API_SEARCH_MAIL_ON_SERVER";\
					break;\
			case _EMAIL_API_ADD_ACCOUNT_WITH_VALIDATION :\
					s = "_EMAIL_API_ADD_ACCOUNT_WITH_VALIDATION";\
					break;\
			case _EMAIL_API_BACKUP_ACCOUNTS :\
					s = "_EMAIL_API_BACKUP_ACCOUNTS";\
					break;\
			case _EMAIL_API_RESTORE_ACCOUNTS :\
					s = "_EMAIL_API_RESTORE_ACCOUNTS";\
					break;\
			case _EMAIL_API_PRINT_RECEIVING_EVENT_QUEUE :\
					s = "_EMAIL_API_PRINT_RECEIVING_EVENT_QUEUE";\
					break;\
			case _EMAIL_API_PING_SERVICE :\
					s = "_EMAIL_API_PING_SERVICE";\
					break;\
			case _EMAIL_API_UPDATE_NOTIFICATION_BAR_FOR_UNREAD_MAIL :\
					s = "_EMAIL_API_UPDATE_NOTIFICATION_BAR_FOR_UNREAD_MAIL";\
					break;\
			case _EMAIL_API_GET_PASSWORD_LENGTH_OF_ACCOUNT:\
					s = "_EMAIL_API_GET_PASSWORD_LENGTH_OF_ACCOUNT";\
					break;\
			case _EMAIL_API_SHOW_USER_MESSAGE:\
					s = "_EMAIL_API_SHOW_USER_MESSAGE";\
					break;\
			default : \
					s = "UNKNOWN_APIID";\
		}\
		s;\
	})


#endif	/* _IPC_APIINFO_H_ */


