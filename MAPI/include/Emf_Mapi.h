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


#ifndef __EMF_MAPI_H__
#define __EMF_MAPI_H__

#include <stdio.h>
#include <stdlib.h>
#include "Emf_Mapi_Account.h"
#include "Emf_Mapi_Message.h"
#include "Emf_Mapi_Rule.h"
#include "Emf_Mapi_Mailbox.h"
#include "Emf_Mapi_Network.h"
#include "Emf_Mapi_Init.h"

/**
* @defgroup EMAIL_FRAMEWORK Email Service
* @{
*/

/**
* @{
*/

/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with Email Engine.
 * @file		Emf_Mapi.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 		This file contains the data structures and interfaces provided by 
 *			Email Engine. 
 */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



enum {
	//Account 
	_EMAIL_API_ADD_ACCOUNT                             = 0x01000000,
	_EMAIL_API_DELETE_ACCOUNT                          = 0x01000001,
	_EMAIL_API_UPDATE_ACCOUNT                          = 0x01000002,
	_EMAIL_API_GET_ACCOUNT                             = 0x01000003,
	_EMAIL_API_GET_ACCOUNT_LIST                        = 0x01000005,
	_EMAIL_API_GET_MAILBOX_COUNT                       = 0x01000007,
	_EMAIL_API_VALIDATE_ACCOUNT                        = 0x01000008,
	_EMAIL_API_ADD_ACCOUNT_WITH_VALIDATION             = 0x01000009,
	_EMAIL_API_BACKUP_ACCOUNTS                         = 0x0100000A,
	_EMAIL_API_RESTORE_ACCOUNTS                        = 0x0100000B,
	_EMAIL_API_GET_PASSWORD_LENGTH_OF_ACCOUNT          = 0x0100000C,

	//Message
	_EMAIL_API_ADD_MAIL_OLD                            = 0x01100000, /* Deprecated */
	_EMAIL_API_GET_MAIL                                = 0x01100001,
	_EMAIL_API_DELETE_MAIL                             = 0x01100002,
	_EMAIL_API_UPDATE_MAIL_OLD                         = 0x01100003, /* Deprecated */
	_EMAIL_API_DELETE_ALL_MAIL                         = 0x01100004,
	_EMAIL_API_GET_MAILBOX_LIST                        = 0x01100006,
	_EMAIL_API_GET_SUBMAILBOX_LIST                     = 0x01100007, 
	_EMAIL_API_CLEAR_DATA                              = 0x01100009,
	_EMAIL_API_MOVE_MAIL                               = 0x0110000A,
	_EMAIL_API_MOVE_ALL_MAIL                           = 0x0110000B,
	_EMAIL_API_ADD_ATTACHMENT                          = 0x0110000C,
	_EMAIL_API_GET_ATTACHMENT                          = 0x0110000D,
	_EMAIL_API_DELETE_ATTACHMENT                       = 0x0110000E,
	_EMAIL_API_MODIFY_MAIL_FLAG                        = 0x0110000F,
	_EMAIL_API_MODIFY_SEEN_MAIL_FLAG                   = 0x01100010, /* Deprecated */
	_EMAIL_API_MODIFY_MAIL_EXTRA_FLAG                  = 0x01100011,
	_EMAIL_API_GET_INFO                                = 0x01100012,
	_EMAIL_API_GET_HEADER_INFO                         = 0x01100013,
	_EMAIL_API_GET_BODY_INFO                           = 0x01100014,
	_EMAIL_API_SET_FLAGS_FIELD                         = 0x01100016,
	_EMAIL_API_ADD_MAIL                                = 0x01100017, 
	_EMAIL_API_UPDATE_MAIL                             = 0x01100018,

	// Thread
	_EMAIL_API_MOVE_THREAD_TO_MAILBOX                  = 0x01110000,
	_EMAIL_API_DELETE_THREAD                           = 0x01110001,
	_EMAIL_API_MODIFY_SEEN_FLAG_OF_THREAD              = 0x01110002,

	//mailbox
	_EMAIL_API_ADD_MAILBOX                             = 0x01200000,
	_EMAIL_API_DELETE_MAILBOX                          = 0x01200001,
	_EMAIL_API_UPDATE_MAILBOX                          = 0x01200002,
	_EMAIL_API_SET_MAIL_SLOT_SIZE                      = 0x01200007,

	//Network
	_EMAIL_API_SEND_MAIL                               = 0x01300000,
	_EMAIL_API_SYNC_HEADER                             = 0x01300001,
	_EMAIL_API_DOWNLOAD_BODY                           = 0x01300002,
	_EMAIL_API_DOWNLOAD_ATTACHMENT                     = 0x01300003,
	_EMAIL_API_NETWORK_GET_STATUS                      = 0x01300004,
	_EMAIL_API_MAIL_SEND_SAVED                         = 0x01300005,
	_EMAIL_API_MAIL_SEND_REPORT                        = 0x01300006,
	_EMAIL_API_DELETE_EMAIL                            = 0x01300007,
	_EMAIL_API_DELETE_EMAIL_ALL                        = 0x01300008,
	_EMAIL_API_GET_IMAP_MAILBOX_LIST                   = 0x01300015,
	_EMAIL_API_SEND_MAIL_CANCEL_JOB                    = 0x01300017, 
	_EMAIL_API_FIND_MAIL_ON_SERVER                     = 0x01300019,

	//Rule
	_EMAIL_API_ADD_RULE                                = 0x01400000,
	_EMAIL_API_GET_RULE                                = 0x01400001,
	_EMAIL_API_GET_RULE_LIST                           = 0x01400002,
	_EMAIL_API_FIND_RULE                               = 0x01400003,
	_EMAIL_API_DELETE_RULE                             = 0x01400004,
	_EMAIL_API_UPDATE_RULE                             = 0x01400005,
	_EMAIL_API_CANCEL_JOB                              = 0x01400006,
	_EMAIL_API_GET_PENDING_JOB                         = 0x01400007,
	_EMAIL_API_SEND_RETRY                              = 0x01400008,
	_EMAIL_API_UPDATE_ACTIVITY                         = 0x01400009,
	_EMAIL_API_SYNC_LOCAL_ACTIVITY                     = 0x0140000A,
	_EMAIL_API_PRINT_RECEIVING_EVENT_QUEUE             = 0x0140000B,

	//Etc
	_EMAIL_API_PING_SERVICE                            = 0x01500000,
	_EMAIL_API_UPDATE_NOTIFICATION_BAR_FOR_UNREAD_MAIL = 0x01500001, 
};

#ifdef __cplusplus
}
#endif

/**
* @} @}
*/

#endif /* __EMAIL_MAPI_H__ */


