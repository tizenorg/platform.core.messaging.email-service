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


#ifndef __EMAIL_TYPES_H__
#define __EMAIL_TYPES_H__

/**
* @defgroup EMAIL_SERVICE Email Service
* @{
*/

/**
* @ingroup EMAIL_SERVICE
* @defgroup EMAIL_TYPES Email Types
* @{
*/
/**
 * This file defines structures and enums of Email Framework.
 * @file	email-types.h
 * @author	Kyu-ho Jo(kyuho.jo@samsung.com)
 * @author	Choongho Lee(ch715.lee@samsung.com)
 * @version	0.1
 * @brief	This file is the header file of Email Framework library.
 */


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <glib.h>
#include <stdbool.h>
#include "email-errors.h"


/*****************************************************************************/
/*  Macros                                                                   */
/*****************************************************************************/

#define MAILBOX_NAME_LENGTH               256
#define MAX_EMAIL_ADDRESS_LENGTH          254                                    /* RFC5322, RFC3696 */
#define MAX_USER_NAME_LENGTH              64
#define MAX_DATETIME_STRING_LENGTH        20
#define MAX_PREVIEW_TEXT_LENGTH           512
#define STRING_LENGTH_FOR_DISPLAY         100
#define MEETING_REQ_OBJECT_ID_LENGTH      256

#define ALL_ACCOUNT                       0
#define NEW_ACCOUNT_ID                    0xFFFFFFFE
#define ALL_MAIL                          -1

#define EMAIL_SEARCH_FILTER_NONE          0x00
#define EMAIL_SEARCH_FILTER_SUBJECT       0x01
#define EMAIL_SEARCH_FILTER_SENDER        0x02
#define EMAIL_SEARCH_FILTER_RECIPIENT     0x04
#define EMAIL_SEARCH_FILTER_ALL           0x07                                   /*  EMAIL_SEARCH_FILTER_SUBJECT + EMAIL_SEARCH_FILTER_SENDER + EMAIL_SEARCH_FILTER_RECIPIEN */

#define EMAIL_SUCCESS                     0                                      /*  we need to modify the success return value */

#define EMAIL_ACC_GET_OPT_DEFAULT         0x01                                   /**< Default values without account name */
#define EMAIL_ACC_GET_OPT_ACCOUNT_NAME    0x02                                   /**< Account name */
#define EMAIL_ACC_GET_OPT_PASSWORD        0x04                                   /**< With password */
#define EMAIL_ACC_GET_OPT_OPTIONS         0x08                                   /**< Account options : email_option_t */
#define EMAIL_ACC_GET_OPT_FULL_DATA       0xFF                                   /**< With all data of account */

#define GET_FULL_DATA                     (EMAIL_ACC_GET_OPT_FULL_DATA)
#define GET_FULL_DATA_WITHOUT_PASSWORD    (EMAIL_ACC_GET_OPT_DEFAULT | EMAIL_ACC_GET_OPT_ACCOUNT_NAME | EMAIL_ACC_GET_OPT_OPTIONS )
#define WITHOUT_OPTION                    (EMAIL_ACC_GET_OPT_DEFAULT | EMAIL_ACC_GET_OPT_ACCOUNT_NAME )
#define ONLY_OPTION                       (EMAIL_ACC_GET_OPT_OPTIONS)

#define THREAD_TYPE_RECEIVING             0                                      /**< for function 'email_activate_pdp' */
#define THREAD_TYPE_SENDING               1                                      /**< for function 'email_activate_pdp' */

#define EMAIL_IMAP_PORT                   143                                    /**< Specifies the default IMAP port.*/
#define EMAIL_POP3_PORT                   110                                    /**< Specifies the default POP3 port.*/
#define EMAIL_SMTP_PORT                   25                                     /**< Specifies the default SMTP port.*/
#define EMAIL_IMAPS_PORT                  993                                    /**< Specifies the default IMAP SSL port.*/
#define EMAIL_POP3S_PORT                  995                                    /**< Specifies the default POP3 SSL port.*/
#define EMAIL_SMTPS_PORT                  465                                    /**< Specifies the default SMTP SSL port.*/
#define EMAIL_ACCOUNT_MAX                 10                                     /**< Specifies the MAX account.*/

#define EMAIL_INBOX_NAME                  "INBOX"                                /**< Specifies the name of inbox.*/
#define EMAIL_DRAFTBOX_NAME               "DRAFTBOX"                             /**< Specifies the name of draftbox.*/
#define EMAIL_OUTBOX_NAME                 "OUTBOX"                               /**< Specifies the name of outbox.*/
#define EMAIL_SENTBOX_NAME                "SENTBOX"                              /**< Specifies the name of sentbox.*/
#define EMAIL_TRASH_NAME                  "TRASH"                                /**< Specifies the name of trash.*/
#define EMAIL_SPAMBOX_NAME                "SPAMBOX"                              /**< Specifies the name of spambox.*/

#define EMAIL_INBOX_DISPLAY_NAME          "Inbox"                                /**< Specifies the display name of inbox.*/
#define EMAIL_DRAFTBOX_DISPLAY_NAME       "Draftbox"                             /**< Specifies the display name of draftbox.*/
#define EMAIL_OUTBOX_DISPLAY_NAME         "Outbox"                               /**< Specifies the display name of outbox.*/
#define EMAIL_SENTBOX_DISPLAY_NAME        "Sentbox"                              /**< Specifies the display name of sentbox.*/
#define EMAIL_TRASH_DISPLAY_NAME          "Trash"                                /**< Specifies the display name of sentbox.*/
#define EMAIL_SPAMBOX_DISPLAY_NAME        "Spambox"                              /**< Specifies the display name of spambox.*/

#define EMAIL_SEARCH_RESULT_MAILBOX_NAME  "_`S1!E2@A3#R4$C5^H6&R7*E8(S9)U0-L=T_" /**< Specifies the name of search result mailbox.*/

#define SYNC_STATUS_FINISHED              0                                      /* BIN 00000000 */
#define SYNC_STATUS_SYNCING               1                                      /* BIN 00000001 */
#define SYNC_STATUS_HAVE_NEW_MAILS        2                                      /* BIN 00000010 */

#define UNKNOWN_CHARSET_PLAIN_TEXT_FILE   "unknown"
#define UNKNOWN_CHARSET_HTML_TEXT_FILE    "unknown.htm"

#define FAILURE                           -1
#define SUCCESS                           0

#define DAEMON_EXECUTABLE_PATH            "/usr/bin/email-service"

#ifndef EXPORT_API
#define EXPORT_API                        __attribute__((visibility("default")))
#endif

#ifndef DEPRECATED
#define DEPRECATED                        __attribute__((deprecated))
#endif

#define VCONF_VIP_NOTI_RINGTONE_PATH          "db/private/email-service/noti_ringtone_path"            
#define VCONF_VIP_NOTI_REP_TYPE               "db/private/email-service/noti_rep_type"                 
#define VCONF_VIP_NOTI_NOTIFICATION_TICKER    "db/private/email-service/noti_notification_ticker"      
#define VCONF_VIP_NOTI_DISPLAY_CONTENT_TICKER "db/private/email-service/noti_display_content_ticker"   
#define VCONF_VIP_NOTI_BADGE_TICKER           "db/private/email-service/noti_badge_ticker"             

/*****************************************************************************/
/*  Enumerations                                                             */
/*****************************************************************************/

enum {
	/* Account */
	_EMAIL_API_ADD_ACCOUNT                               = 0x01000000,
	_EMAIL_API_DELETE_ACCOUNT                            = 0x01000001,
	_EMAIL_API_UPDATE_ACCOUNT                            = 0x01000002,
	_EMAIL_API_GET_ACCOUNT                               = 0x01000003,
	_EMAIL_API_GET_ACCOUNT_LIST                          = 0x01000005,
	_EMAIL_API_GET_MAILBOX_COUNT                         = 0x01000007,
	_EMAIL_API_VALIDATE_ACCOUNT                          = 0x01000008,
	_EMAIL_API_ADD_ACCOUNT_WITH_VALIDATION               = 0x01000009,
	_EMAIL_API_BACKUP_ACCOUNTS                           = 0x0100000A,
	_EMAIL_API_RESTORE_ACCOUNTS                          = 0x0100000B,
	_EMAIL_API_GET_PASSWORD_LENGTH_OF_ACCOUNT            = 0x0100000C,

	/* Mail */
	_EMAIL_API_DELETE_MAIL                               = 0x01100002,
	_EMAIL_API_DELETE_ALL_MAIL                           = 0x01100004,
	_EMAIL_API_GET_MAILBOX_LIST                          = 0x01100006,
	_EMAIL_API_GET_SUBMAILBOX_LIST                       = 0x01100007,
	_EMAIL_API_CLEAR_DATA                                = 0x01100009,
	_EMAIL_API_MOVE_MAIL                                 = 0x0110000A,
	_EMAIL_API_MOVE_ALL_MAIL                             = 0x0110000B,
	_EMAIL_API_ADD_ATTACHMENT                            = 0x0110000C,
	_EMAIL_API_GET_ATTACHMENT                            = 0x0110000D,
	_EMAIL_API_DELETE_ATTACHMENT                         = 0x0110000E,
	_EMAIL_API_MODIFY_MAIL_FLAG                          = 0x0110000F,
	_EMAIL_API_MODIFY_MAIL_EXTRA_FLAG                    = 0x01100011,
	_EMAIL_API_SET_FLAGS_FIELD                           = 0x01100016,
	_EMAIL_API_ADD_MAIL                                  = 0x01100017,
	_EMAIL_API_UPDATE_MAIL                               = 0x01100018,
	_EMAIL_API_ADD_READ_RECEIPT                          = 0x01100019,
	_EMAIL_API_EXPUNGE_MAILS_DELETED_FLAGGED             = 0x0110001A,
	_EMAIL_API_UPDATE_MAIL_ATTRIBUTE                     = 0x0110001B,

	/* Thread */
	_EMAIL_API_MOVE_THREAD_TO_MAILBOX                    = 0x01110000,
	_EMAIL_API_DELETE_THREAD                             = 0x01110001,
	_EMAIL_API_MODIFY_SEEN_FLAG_OF_THREAD                = 0x01110002,

	/* Mailbox */
	_EMAIL_API_ADD_MAILBOX                               = 0x01200000,
	_EMAIL_API_DELETE_MAILBOX                            = 0x01200001,
	_EMAIL_API_SET_MAIL_SLOT_SIZE                        = 0x01200007,
	_EMAIL_API_RENAME_MAILBOX                            = 0x01200008,
	_EMAIL_API_SET_MAILBOX_TYPE                          = 0x01200009,
	_EMAIL_API_SET_LOCAL_MAILBOX                         = 0x0120000A,

	/* Network */
	_EMAIL_API_SEND_MAIL                                 = 0x01300000,
	_EMAIL_API_SYNC_HEADER                               = 0x01300001,
	_EMAIL_API_DOWNLOAD_BODY                             = 0x01300002,
	_EMAIL_API_DOWNLOAD_ATTACHMENT                       = 0x01300003,
	_EMAIL_API_NETWORK_GET_STATUS                        = 0x01300004,
	_EMAIL_API_SEND_SAVED                                = 0x01300005,
	_EMAIL_API_DELETE_EMAIL                              = 0x01300007,
	_EMAIL_API_DELETE_EMAIL_ALL                          = 0x01300008,
	_EMAIL_API_GET_IMAP_MAILBOX_LIST                     = 0x01300015,
	_EMAIL_API_SEND_MAIL_CANCEL_JOB                       = 0x01300017,
	_EMAIL_API_SEARCH_MAIL_ON_SERVER                     = 0x01300019,
	_EMAIL_API_CLEAR_RESULT_OF_SEARCH_MAIL_ON_SERVER     = 0x0130001A,

	/* Rule */
	_EMAIL_API_ADD_RULE                                  = 0x01400000,
	_EMAIL_API_GET_RULE                                  = 0x01400001,
	_EMAIL_API_GET_RULE_LIST                             = 0x01400002,
	_EMAIL_API_FIND_RULE                                 = 0x01400003,
	_EMAIL_API_DELETE_RULE                               = 0x01400004,
	_EMAIL_API_UPDATE_RULE                               = 0x01400005,
	_EMAIL_API_CANCEL_JOB                                = 0x01400006,
	_EMAIL_API_GET_PENDING_JOB                           = 0x01400007,
	_EMAIL_API_SEND_RETRY                                = 0x01400008,
	_EMAIL_API_UPDATE_ACTIVITY                           = 0x01400009,
	_EMAIL_API_SYNC_LOCAL_ACTIVITY                       = 0x0140000A,
	_EMAIL_API_PRINT_RECEIVING_EVENT_QUEUE               = 0x0140000B,

	/* Etc */
	_EMAIL_API_PING_SERVICE                              = 0x01500000,
	_EMAIL_API_UPDATE_NOTIFICATION_BAR_FOR_UNREAD_MAIL   = 0x01500001,
	_EMAIL_API_SHOW_USER_MESSAGE                         = 0x01500002,
	_EMAIL_API_WRITE_MIME_FILE                           = 0x01500003,

	/* Smime */
	_EMAIL_API_ADD_CERTIFICATE                           = 0x01600000,
	_EMAIL_API_DELETE_CERTIFICATE                        = 0x01600001,
	_EMAIL_API_VERIFY_SIGNATURE                          = 0x01600002,
	_EMAIL_API_VERIFY_CERTIFICATE                        = 0x01600003,
};

typedef enum
{
	EMAIL_DELETE_LOCALLY                     = 0,  /**< Specifies Mail Delete local only */
	EMAIL_DELETE_LOCAL_AND_SERVER,                 /**< Specifies Mail Delete local & server */
	EMAIL_DELETE_FOR_SEND_THREAD,                  /**< Created to check which activity to delete in send thread */
	EMAIL_DELETE_FROM_SERVER,
	EMAIL_DELETE_MAIL_AND_MEETING_FOR_EAS,         /**< Delete mails and meetings on an EAS account. */
} email_delete_option_t;

typedef enum
{
	NOTI_MAIL_ADD                            = 10000,
	NOTI_MAIL_DELETE                         = 10001,
	NOTI_MAIL_DELETE_ALL                     = 10002,
	NOTI_MAIL_DELETE_WITH_ACCOUNT            = 10003,
	NOTI_MAIL_DELETE_FAIL                    = 10007,
	NOTI_MAIL_DELETE_FINISH                  = 10008,

	NOTI_MAIL_UPDATE                         = 10004,
	NOTI_MAIL_FIELD_UPDATE                   = 10020,

	NOTI_MAIL_MOVE                           = 10005,
	NOTI_MAIL_MOVE_FAIL                      = 10010,
	NOTI_MAIL_MOVE_FINISH                    = 10006,

	NOTI_THREAD_MOVE                         = 11000,
	NOTI_THREAD_DELETE                       = 11001,
	NOTI_THREAD_MODIFY_SEEN_FLAG             = 11002,

	NOTI_ACCOUNT_ADD                         = 20000,
	NOTI_ACCOUNT_DELETE                      = 20001,
	NOTI_ACCOUNT_DELETE_FAIL                 = 20003,
	NOTI_ACCOUNT_UPDATE                      = 20002,
	NOTI_ACCOUNT_UPDATE_SYNC_STATUS          = 20010,

	NOTI_MAILBOX_ADD                         = 40000,
	NOTI_MAILBOX_DELETE                      = 40001,
	NOTI_MAILBOX_UPDATE                      = 40002,
	NOTI_MAILBOX_FIELD_UPDATE                = 40003,

	NOTI_MAILBOX_RENAME                      = 40010,
	NOTI_MAILBOX_RENAME_FAIL                 = 40011,

	NOTI_CERTIFICATE_ADD                     = 50000,
	NOTI_CERTIFICATE_DELETE                  = 50001,
	NOTI_CERTIFICATE_UPDATE                  = 50002,
	/* To be added more */
} email_noti_on_storage_event;

typedef enum
{
	NOTI_SEND_START                          = 1002,
	NOTI_SEND_FINISH                         = 1004,
	NOTI_SEND_FAIL                           = 1005,
	NOTI_SEND_CANCEL                         = 1003,

	NOTI_DOWNLOAD_START                      = 2000,
	NOTI_DOWNLOAD_FINISH,
	NOTI_DOWNLOAD_FAIL,
	NOTI_DOWNLOAD_CANCEL                     = 2004,
	NOTI_DOWNLOAD_NEW_MAIL                   = 2003,

	NOTI_DOWNLOAD_BODY_START                 = 3000,
	NOTI_DOWNLOAD_BODY_FINISH                = 3002,
	NOTI_DOWNLOAD_BODY_FAIL                  = 3004,
	NOTI_DOWNLOAD_BODY_CANCEL                = 3003,
	NOTI_DOWNLOAD_MULTIPART_BODY             = 3001,

	NOTI_DOWNLOAD_ATTACH_START               = 4000,
	NOTI_DOWNLOAD_ATTACH_FINISH,
	NOTI_DOWNLOAD_ATTACH_FAIL,
	NOTI_DOWNLOAD_ATTACH_CANCEL,

	NOTI_MAIL_DELETE_ON_SERVER_FAIL          = 5000,
	NOTI_MAIL_MOVE_ON_SERVER_FAIL,

	NOTI_SEARCH_ON_SERVER_START              = 6000,
	NOTI_SEARCH_ON_SERVER_FINISH             = 6001,
	NOTI_SEARCH_ON_SERVER_FAIL               = 6002,
	NOTI_SEARCH_ON_SERVER_CANCEL             = 6003,

	NOTI_VALIDATE_ACCOUNT_FINISH             = 7000,
	NOTI_VALIDATE_ACCOUNT_FAIL,
	NOTI_VALIDATE_ACCOUNT_CANCEL,

	NOTI_VALIDATE_AND_CREATE_ACCOUNT_FINISH  = 8000,
	NOTI_VALIDATE_AND_CREATE_ACCOUNT_FAIL,
	NOTI_VALIDATE_AND_CREATE_ACCOUNT_CANCEL,

	NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FINISH  = 9000,
	NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FAIL,
	NOTI_VALIDATE_AND_UPDATE_ACCOUNT_CANCEL,

	NOTI_VALIDATE_CERTIFICATE_FINISH         = 10000,
	NOTI_VALIDATE_CERTIFICATE_FAIL           = 10001,
	NOTI_VALIDATE_CERTIFICATE_CANCEL         = 10002,

	NOTI_RESOLVE_RECIPIENT_START             = 11000,
	NOTI_RESOLVE_RECIPIENT_FINISH,
	NOTI_RESOLVE_RECIPIENT_FAIL,
	NOTI_RESOLVE_RECIPIENT_CANCEL,

	NOTI_RENAME_MAILBOX_START                = 12000,
	NOTI_RENAME_MAILBOX_FINISH,
	NOTI_RENAME_MAILBOX_FAIL,
	NOTI_RENAME_MAILBOX_CANCEL,

	NOTI_ADD_MAILBOX_START                   = 12100,
	NOTI_ADD_MAILBOX_FINISH,
	NOTI_ADD_MAILBOX_FAIL,
	NOTI_ADD_MAILBOX_CANCEL,

	NOTI_DELETE_MAILBOX_START                = 12200,
	NOTI_DELETE_MAILBOX_FINISH,
	NOTI_DELETE_MAILBOX_FAIL,
	NOTI_DELETE_MAILBOX_CANCEL,

	NOTI_SYNC_IMAP_MAILBOX_LIST_START        = 12300,
	NOTI_SYNC_IMAP_MAILBOX_LIST_FINISH,
	NOTI_SYNC_IMAP_MAILBOX_LIST_FAIL,
	NOTI_SYNC_IMAP_MAILBOX_LIST_CANCEL,

	/* To be added more */
} email_noti_on_network_event;

typedef enum
{
	RESPONSE_SUCCEEDED                       = 0,
	RESPONSE_FAILED                          = 1,
	RESPONSE_CANCELED                        = 2
	/* To be added more */
} email_response_to_request;

/**
 * This enumeration specifies the mail type of account.
 */
typedef enum
{
	EMAIL_BIND_TYPE_DISABLE          = 0,          /**< Specifies the bind type for Disabled account.*/
	EMAIL_BIND_TYPE_EM_CORE          = 1,          /**< Specifies the bind type for email-service .*/
} email_account_bind_t DEPRECATED;

/**
 * This enumeration specifies the server type of account.
 */
typedef enum
{
	EMAIL_SERVER_TYPE_POP3           = 1,          /**< Specifies the POP3 Server.*/
	EMAIL_SERVER_TYPE_IMAP4,                       /**< Specifies the IMAP4 Server.*/
	EMAIL_SERVER_TYPE_SMTP,                        /**< Specifies the SMTP Server.*/
	EMAIL_SERVER_TYPE_NONE,                        /**< Specifies the Local.*/
	EMAIL_SERVER_TYPE_ACTIVE_SYNC,                 /** < Specifies the Active Sync.  */
} email_account_server_t;

/**
 * This enumeration specifies the mode of retrieval.
 */
typedef enum
{
	EMAIL_IMAP4_RETRIEVAL_MODE_NEW   = 0,          /**< Specifies the retrieval mode for new email.*/
	EMAIL_IMAP4_RETRIEVAL_MODE_ALL,                /**< Specifies the retrieval mode for all email.*/
} email_imap4_retrieval_mode_t;

/**
 * This enumeration specifies the filtering type.
 */
typedef enum
{
	EMAIL_FILTER_FROM                = 1,          /**< Specifies the filtering of sender.*/
	EMAIL_FILTER_SUBJECT,                          /**< Specifies the filtering of email subject.*/
	EMAIL_FILTER_BODY,                             /**< Specifies the filterinf of email body.*/
	EMAIL_PRIORITY_SENDER,                         /**< Specifies the priority sender of email. */
} email_rule_type_t;


/**
 * This enumeration specifies the rules for filtering type.
 */
typedef enum
{
	RULE_TYPE_INCLUDES             = 1,          /**< Specifies the filtering rule for includes.*/
	RULE_TYPE_EXACTLY,                           /**< Specifies the filtering rule for Exactly same as.*/
} email_filtering_type_t;


/**
 * This enumeration specifies the action for filtering type.
 */
typedef enum
{
	EMAIL_FILTER_MOVE                = 1,          /**< Specifies the move of email.*/
	EMAIL_FILTER_BLOCK               = 2,          /**< Specifies the block of email.*/
	EMAIL_FILTER_DELETE              = 3,          /**< Specifies delete email.*/
} email_rule_action_t;

/**
 * This enumeration specifies the email status.
 */
typedef enum
{
	EMAIL_MAIL_STATUS_NONE           = 0,          /**< The Mail is in No Operation state */
	EMAIL_MAIL_STATUS_RECEIVED,                    /**< The mail is a received mail.*/
	EMAIL_MAIL_STATUS_SENT,                        /**< The mail is a sent mail.*/
	EMAIL_MAIL_STATUS_SAVED,                       /**< The mail is a saved mail.*/
	EMAIL_MAIL_STATUS_SAVED_OFFLINE,               /**< The mail is a saved mail in off-line mode.*/
	EMAIL_MAIL_STATUS_SENDING,                     /**< The mail is being sent.*/
	EMAIL_MAIL_STATUS_SEND_FAILURE,                /**< The mail is a mail to been failed to send.*/
	EMAIL_MAIL_STATUS_SEND_CANCELED,               /**< The mail is a canceled mail.*/
	EMAIL_MAIL_STATUS_SEND_WAIT,                   /**< The mail is a mail to be send .*/
} email_mail_status_t;

/**
 * This enumeration specifies the email priority.
 */
typedef enum
{
	EMAIL_MAIL_PRIORITY_HIGH         = 1,          /**< The priority is high.*/
	EMAIL_MAIL_PRIORITY_NORMAL       = 3,          /**< The priority is normal.*/
	EMAIL_MAIL_PRIORITY_LOW          = 5,          /**< The priority is low.*/
} email_mail_priority_t;

/**
 * This enumeration specifies the email status.
 */
typedef enum
{
	EMAIL_MAIL_REPORT_NONE           = 0x00,       /**< The mail isn't report mail.*/
	EMAIL_MAIL_REPORT_REQUEST        = 0x03,       /* Deprecated */
	EMAIL_MAIL_REPORT_DSN            = 0x04,       /**< The mail is a Delivery Status Notifications mail.*/
	EMAIL_MAIL_REPORT_MDN            = 0x08,       /**< The mail is a Message Disposition Notifications mail.*/
	EMAIL_MAIL_REQUEST_DSN           = 0x10,       /**< The mail requires Delivery Status Notifications.*/
	EMAIL_MAIL_REQUEST_MDN           = 0x20,       /**< The mail requires Message Disposition Notifications.*/
} email_mail_report_t;

/**
 * This enumeration specifies the DRM type
 */
typedef enum
{
	EMAIL_ATTACHMENT_DRM_NONE        = 0,          /**< The mail isn't DRM file.*/
	EMAIL_ATTACHMENT_DRM_OBJECT,                   /**< The mail is a DRM object.*/
	EMAIL_ATTACHMENT_DRM_RIGHTS,                   /**< The mail is a DRM rights as xml format.*/
	EMAIL_ATTACHMENT_DRM_DCF,                      /**< The mail is a DRM dcf.*/
} email_attachment_drm_t;

/**
 * This enumeration specifies the mail type
 */
typedef enum
{
	EMAIL_MAIL_TYPE_NORMAL                     = 0, /**< NOT a meeting request mail. A Normal mail */
	EMAIL_MAIL_TYPE_MEETING_REQUEST            = 1, /**< a meeting request mail from a serve */
	EMAIL_MAIL_TYPE_MEETING_RESPONSE           = 2, /**< a response mail about meeting reques */
	EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST = 3  /**< a originating mail about meeting reques */
} email_mail_type_t;

/**
 * This enumeration specifies the meeting response type
 */
typedef enum
{
	EMAIL_MEETING_RESPONSE_NONE                = 0, /**< NOT response */
	EMAIL_MEETING_RESPONSE_ACCEPT              = 1, /**< The response is acceptance */
	EMAIL_MEETING_RESPONSE_TENTATIVE           = 2, /**< The response is tentative */
	EMAIL_MEETING_RESPONSE_DECLINE             = 3, /**< The response is decline */
	EMAIL_MEETING_RESPONSE_REQUEST             = 4, /**< The response is request */
	EMAIL_MEETING_RESPONSE_CANCEL              = 5, /**< The response is cancelation */
} email_meeting_response_t;

typedef enum
{
	EMAIL_ACTION_SEND_MAIL                     =  0,
	EMAIL_ACTION_SYNC_HEADER                   =  1,
	EMAIL_ACTION_DOWNLOAD_BODY                 =  2,
	EMAIL_ACTION_DOWNLOAD_ATTACHMENT           =  3,
	EMAIL_ACTION_DELETE_MAIL                   =  4,
	EMAIL_ACTION_SEARCH_MAIL                   =  5,
	EMAIL_ACTION_SAVE_MAIL                     =  6,
	EMAIL_ACTION_SYNC_MAIL_FLAG_TO_SERVER      =  7,
	EMAIL_ACTION_SYNC_FLAGS_FIELD_TO_SERVER    =  8,
	EMAIL_ACTION_MOVE_MAIL                     =  9,
	EMAIL_ACTION_CREATE_MAILBOX                = 10,
	EMAIL_ACTION_DELETE_MAILBOX                = 11,
	EMAIL_ACTION_SYNC_HEADER_OMA               = 12,
	EMAIL_ACTION_VALIDATE_ACCOUNT              = 13,
	EMAIL_ACTION_VALIDATE_AND_CREATE_ACCOUNT   = 14,
	EMAIL_ACTION_VALIDATE_AND_UPDATE_ACCOUNT   = 15,
	EMAIL_ACTION_UPDATE_MAIL                   = 30,
	EMAIL_ACTION_SET_MAIL_SLOT_SIZE            = 31,
	EMAIL_ACTION_EXPUNGE_MAILS_DELETED_FLAGGED = 32,
	EMAIL_ACTION_SEARCH_ON_SERVER              = 33,
	EMAIL_ACTION_MOVE_MAILBOX                  = 34,
	EMAIL_ACTION_NUM,
} email_action_t;

/**
 * This enumeration specifies the status of getting envelope list.
 */
typedef enum
{
	EMAIL_LIST_NONE                  = 0,
	EMAIL_LIST_WAITING,
	EMAIL_LIST_PREPARE,                            /**< Specifies the preparation.*/
	EMAIL_LIST_CONNECTION_START,                   /**< Specifies the connection start.*/
	EMAIL_LIST_CONNECTION_SUCCEED,                 /**< Specifies the success of connection.*/
	EMAIL_LIST_CONNECTION_FINISH,                  /**< Specifies the connection finish.*/
	EMAIL_LIST_CONNECTION_FAIL,                    /**< Specifies the connection failure.*/
	EMAIL_LIST_START,                              /**< Specifies the getting start.*/
	EMAIL_LIST_PROGRESS,                           /**< Specifies the status of getting.*/
	EMAIL_LIST_FINISH,                             /**< Specifies the getting complete.*/
	EMAIL_LIST_FAIL,                               /**< Specifies the download failure.*/
} email_envelope_list_status_t;

/**
 * This enumeration specifies the downloaded status of email.
 */
typedef enum
{
	EMAIL_DOWNLOAD_NONE              = 0,
	EMAIL_DOWNLOAD_WAITING,
	EMAIL_DOWNLOAD_PREPARE,                        /**< Specifies the preparation.*/
	EMAIL_DOWNLOAD_CONNECTION_START,               /**< Specifies the connection start.*/
	EMAIL_DOWNLOAD_CONNECTION_SUCCEED,             /**< Specifies the success of connection.*/
	EMAIL_DOWNLOAD_CONNECTION_FINISH,              /**< Specifies the connection finish.*/
	EMAIL_DOWNLOAD_CONNECTION_FAIL,                /**< Specifies the connection failure.*/
	EMAIL_DOWNLOAD_START,                          /**< Specifies the download start.*/
	EMAIL_DOWNLOAD_PROGRESS,                       /**< Specifies the status of download.*/
	EMAIL_DOWNLOAD_FINISH,                         /**< Specifies the download complete.*/
	EMAIL_DOWNLOAD_FAIL,                           /**< Specifies the download failure.*/
} email_download_status_t;

/**
 * This enumeration specifies the status of sending email.
 */
typedef enum
{
	EMAIL_SEND_NONE                  = 0,
	EMAIL_SEND_WAITING,
	EMAIL_SEND_PREPARE,                            /**< Specifies the preparation.*/
	EMAIL_SEND_CONNECTION_START,                   /**< Specifies the connection start.*/
	EMAIL_SEND_CONNECTION_SUCCEED,                 /**< Specifies the success of connection.*/
	EMAIL_SEND_CONNECTION_FINISH,                  /**< Specifies the connection finish.*/
	EMAIL_SEND_CONNECTION_FAIL,                    /**< Specifies the connection failure.*/
	EMAIL_SEND_START,                              /**< Specifies the sending start.*/
	EMAIL_SEND_PROGRESS,                           /**< Specifies the status of sending.*/
	EMAIL_SEND_FINISH,                             /**< Specifies the sending complete.*/
	EMAIL_SEND_FAIL,                               /**< Specifies the sending failure.*/
	EMAIL_SAVE_WAITING,                            /**< Specfies the Waiting of Sync */
} email_send_status_t;

typedef enum
{
	EMAIL_SYNC_NONE                  = 0,
	EMAIL_SYNC_WAITING,
	EMAIL_SYNC_PREPARE,
	EMAIL_SYNC_CONNECTION_START,
	EMAIL_SYNC_CONNECTION_SUCCEED,
	EMAIL_SYNC_CONNECTION_FINISH,
	EMAIL_SYNC_CONNECTION_FAIL,
	EMAIL_SYNC_START,
	EMAIL_SYNC_PROGRESS,
	EMAIL_SYNC_FINISH,
	EMAIL_SYNC_FAIL,
} email_sync_status_t;

/**
* This enumeration specifies the deleting status of email.
*/
typedef enum
{
	EMAIL_DELETE_NONE                = 0,
	EMAIL_DELETE_WAITING,
	EMAIL_DELETE_PREPARE,                          /**< Specifies the preparation.*/
	EMAIL_DELETE_CONNECTION_START,                 /**< Specifies the connection start.*/
	EMAIL_DELETE_CONNECTION_SUCCEED,               /**< Specifies the success of connection.*/
	EMAIL_DELETE_CONNECTION_FINISH,                /**< Specifies the connection finish.*/
	EMAIL_DELETE_CONNECTION_FAIL,                  /**< Specifies the connection failure.*/
	EMAIL_DELETE_START,                            /**< Specifies the deletion start.*/
	EMAIL_DELETE_PROGRESS,                         /**< Specifies the status of deleting.*/
	EMAIL_DELETE_SERVER_PROGRESS,                  /**< Specifies the status of server deleting.*/
	EMAIL_DELETE_LOCAL_PROGRESS,                   /**< Specifies the status of local deleting.*/
	EMAIL_DELETE_FINISH,                           /**< Specifies the deletion complete.*/
	EMAIL_DELETE_FAIL,                             /**< Specifies the deletion failure.*/
} email_delete_status_t;

/**
* This enumeration specifies the status of validating account
*/
typedef enum
{
	EMAIL_VALIDATE_ACCOUNT_NONE = 0,
	EMAIL_VALIDATE_ACCOUNT_WAITING,
	EMAIL_VALIDATE_ACCOUNT_PREPARE,                /**< Specifies the preparation.*/
	EMAIL_VALIDATE_ACCOUNT_CONNECTION_START,       /**< Specifies the connection start.*/
	EMAIL_VALIDATE_ACCOUNT_CONNECTION_SUCCEED,     /**< Specifies the success of connection.*/
	EMAIL_VALIDATE_ACCOUNT_CONNECTION_FINISH,      /**< Specifies the connection finish.*/
	EMAIL_VALIDATE_ACCOUNT_CONNECTION_FAIL,        /**< Specifies the connection failure.*/
	EMAIL_VALIDATE_ACCOUNT_START,                  /**< Specifies the getting start.*/
	EMAIL_VALIDATE_ACCOUNT_PROGRESS,               /**< Specifies the status of getting.*/
	EMAIL_VALIDATE_ACCOUNT_FINISH,                 /**< Specifies the getting complete.*/
	EMAIL_VALIDATE_ACCOUNT_FAIL,                   /**< Specifies the validation failure.*/
} email_validate_account_status_t;

typedef enum
{
	EMAIL_SET_SLOT_SIZE_NONE         = 0,
	EMAIL_SET_SLOT_SIZE_WAITING,
	EMAIL_SET_SLOT_SIZE_START,
	EMAIL_SET_SLOT_SIZE_FINISH,
	EMAIL_SET_SLOT_SIZE_FAIL,
}email_set_slot_size_status_e;

typedef enum
{
	EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_NONE         = 0,
	EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_WAITING,
	EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_START,
	EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_FINISH,
	EMAIL_EXPUNGE_MAILS_DELETED_FLAGGED_FAIL,
}email_expunge_mails_deleted_flagged_status_e;

typedef enum
{
	EMAIL_SEARCH_ON_SERVER_NONE         = 0,
	EMAIL_SEARCH_ON_SERVER_WAITING,
	EMAIL_SEARCH_ON_SERVER_START,
	EMAIL_SEARCH_ON_SERVER_FINISH,
	EMAIL_SEARCH_ON_SERVER_FAIL,
}email_search_on_server_status_e;

typedef enum
{
	EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_NONE         = 0,
	EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_WAITING,
	EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_START,
	EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_FINISH,
	EMAIL_MOVE_MAILBOX_ON_IMAP_SERVER_FAIL,
}email_move_mailbox_status_e;

typedef enum
{
	EMAIL_UPDATE_MAIL_NONE         = 0,
	EMAIL_UPDATE_MAIL_WAITING,
	EMAIL_UPDATE_MAIL_START,
	EMAIL_UPDATE_MAIL_FINISH,
	EMAIL_UPDATE_MAIL_FAIL,
}email_update_mail_status_e;

/**
* This enumeration specifies the type of mailbox
*/
typedef enum
{
	EMAIL_MAILBOX_TYPE_NONE          = 0,         /**< Unspecified mailbox type*/
	EMAIL_MAILBOX_TYPE_INBOX         = 1,         /**< Specified inbox type*/
	EMAIL_MAILBOX_TYPE_SENTBOX       = 2,         /**< Specified sent box type*/
	EMAIL_MAILBOX_TYPE_TRASH         = 3,         /**< Specified trash type*/
	EMAIL_MAILBOX_TYPE_DRAFT         = 4,         /**< Specified draft box type*/
	EMAIL_MAILBOX_TYPE_SPAMBOX       = 5,         /**< Specified spam box type*/
	EMAIL_MAILBOX_TYPE_OUTBOX        = 6,         /**< Specified outbox type*/
	EMAIL_MAILBOX_TYPE_ALL_EMAILS    = 7,         /**< Specified all emails type of gmail*/
	EMAIL_MAILBOX_TYPE_SEARCH_RESULT = 8,         /**< Specified mailbox type for result of search on server */
	EMAIL_MAILBOX_TYPE_FLAGGED       = 9,         /**< Specified flagged mailbox type on gmail */
	EMAIL_MAILBOX_TYPE_USER_DEFINED  = 0xFF,      /**< Specified mailbox type for all other mailboxes */
}email_mailbox_type_e;

typedef enum
{
	EMAIL_SYNC_LATEST_MAILS_FIRST    = 0,
	EMAIL_SYNC_OLDEST_MAILS_FIRST,
	EMAIL_SYNC_ALL_MAILBOX_50_MAILS,
} EMAIL_RETRIEVE_MODE;

/*  event type */
typedef enum
{
	EMAIL_EVENT_NONE                            =  0,
	EMAIL_EVENT_SYNC_HEADER                     =  1,          /*  synchronize mail headers with server (network used) */
	EMAIL_EVENT_DOWNLOAD_BODY                   =  2,          /*  download mail body from server (network used)*/
	EMAIL_EVENT_DOWNLOAD_ATTACHMENT             =  3,          /*  download mail attachment from server (network used) */
	EMAIL_EVENT_SEND_MAIL                       =  4,          /*  send a mail (network used) */
	EMAIL_EVENT_SEND_MAIL_SAVED                 =  5,          /*  send all mails in 'outbox' box (network used) */
	EMAIL_EVENT_SYNC_IMAP_MAILBOX               =  6,          /*  download imap mailboxes from server (network used) */
	EMAIL_EVENT_DELETE_MAIL                     =  7,          /*  delete mails (network unused) */
	EMAIL_EVENT_DELETE_MAIL_ALL                 =  8,          /*  delete all mails (network unused) */
	EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER        =  9,          /*  sync mail flag to server */
	EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER      = 10,          /*  sync a field of flags to server */
	EMAIL_EVENT_SAVE_MAIL                       = 11,          /*  add mail on server */
	EMAIL_EVENT_MOVE_MAIL                       = 12,          /*  move mails to specific mailbox on server */
	EMAIL_EVENT_CREATE_MAILBOX                  = 13,
	EMAIL_EVENT_UPDATE_MAILBOX                  = 14,
	EMAIL_EVENT_DELETE_MAILBOX                  = 15,
	EMAIL_EVENT_ISSUE_IDLE                      = 16,
	EMAIL_EVENT_SYNC_HEADER_OMA                 = 17,
	EMAIL_EVENT_VALIDATE_ACCOUNT                = 18,
	EMAIL_EVENT_VALIDATE_AND_CREATE_ACCOUNT     = 19,
	EMAIL_EVENT_VALIDATE_AND_UPDATE_ACCOUNT     = 20,
	EMAIL_EVENT_SEARCH_ON_SERVER                = 21,
	EMAIL_EVENT_RENAME_MAILBOX_ON_IMAP_SERVER                    = 22,

	EMAIL_EVENT_ADD_MAIL                        = 10001,       /*  Deprecated */
	EMAIL_EVENT_UPDATE_MAIL_OLD                 = 10002,       /*  Deprecated */
	EMAIL_EVENT_UPDATE_MAIL                     = 10003,
	EMAIL_EVENT_SET_MAIL_SLOT_SIZE              = 20000,
	EMAIL_EVENT_EXPUNGE_MAILS_DELETED_FLAGGED   = 20001,

/* 	EMAIL_EVENT_LOCAL_ACTIVITY,                     __LOCAL_ACTIVITY_ */

	EMAIL_EVENT_BULK_PARTIAL_BODY_DOWNLOAD      = 20002,       /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ supported */
	EMAIL_EVENT_LOCAL_ACTIVITY_SYNC_BULK_PBD    = 20003,       /*  __FEATURE_PARTIAL_BODY_DOWNLOAD__ supported */
	EMAIL_EVENT_GET_PASSWORD_LENGTH             = 20004,       /*  get password length of an account */
} email_event_type_t;

/*  event status */
typedef enum
{
	EMAIL_EVENT_STATUS_UNUSED        = 0,
	EMAIL_EVENT_STATUS_WAIT,
	EMAIL_EVENT_STATUS_STARTED,
	EMAIL_EVENT_STATUS_CANCELED,
} email_event_status_type_t;


/* sorting_orde */
typedef enum
{
	EMAIL_SORT_DATETIME_HIGH         = 0,
	EMAIL_SORT_DATETIME_LOW,
	EMAIL_SORT_SENDER_HIGH,
	EMAIL_SORT_SENDER_LOW,
	EMAIL_SORT_RCPT_HIGH,
	EMAIL_SORT_RCPT_LOW,
	EMAIL_SORT_SUBJECT_HIGH,
	EMAIL_SORT_SUBJECT_LOW,
	EMAIL_SORT_PRIORITY_HIGH,
	EMAIL_SORT_PRIORITY_LOW,
	EMAIL_SORT_ATTACHMENT_HIGH,
	EMAIL_SORT_ATTACHMENT_LOW,
	EMAIL_SORT_FAVORITE_HIGH,
	EMAIL_SORT_FAVORITE_LOW,
	EMAIL_SORT_MAILBOX_NAME_HIGH,
	EMAIL_SORT_MAILBOX_NAME_LOW,
	EMAIL_SORT_FLAGGED_FLAG_HIGH,
	EMAIL_SORT_FLAGGED_FLAG_LOW,
	EMAIL_SORT_SEEN_FLAG_HIGH,
	EMAIL_SORT_SEEN_FLAG_LOW,
	EMAIL_SORT_END,
}email_sort_type_t;

typedef enum
{
	EMAIL_MAILBOX_SORT_BY_NAME_ASC  = 0,
	EMAIL_MAILBOX_SORT_BY_NAME_DSC,
	EMAIL_MAILBOX_SORT_BY_TYPE_ASC,
	EMAIL_MAILBOX_SORT_BY_TYPE_DSC,
} email_mailbox_sort_type_t;


/**
* This enumeration specifies the priority.
*/
enum
{
	EMAIL_OPTION_PRIORITY_HIGH       = 1,          /**< Specifies the high priority.*/
	EMAIL_OPTION_PRIORITY_NORMAL     = 3,          /**< Specifies the normal priority*/
	EMAIL_OPTION_PRIORITY_LOW        = 5,          /**< Specifies the low priority.*/
};

/**
* This enumeration specifies the saving save a copy after sending.
*/
enum
{
	EMAIL_OPTION_KEEP_LOCAL_COPY_OFF = 0,          /**< Specifies off the keeping local copy.*/
	EMAIL_OPTION_KEEP_LOCAL_COPY_ON  = 1,          /**< Specifies on the keeping local copy.*/
};

/**
* This enumeration specifies the request of delivery report.
*/
enum
{
	EMAIL_OPTION_REQ_DELIVERY_RECEIPT_OFF = 0,     /**< Specifies off the requesting delivery receipt.*/
	EMAIL_OPTION_REQ_DELIVERY_RECEIPT_ON  = 1,     /**< Specifies on the requesting delivery receipt.*/
};

/**
* This enumeration specifies the request of read receipt.
*/
enum
{
	EMAIL_OPTION_REQ_READ_RECEIPT_OFF = 0,         /**< Specifies off the requesting read receipt.*/
	EMAIL_OPTION_REQ_READ_RECEIPT_ON  = 1,         /**< Specifies on the requesting read receipt.*/
};

/**
* This enumeration specifies the blocking of address.
*/
enum
{
	EMAIL_OPTION_BLOCK_ADDRESS_OFF   = 0,          /**< Specifies off the blocking by address.*/
	EMAIL_OPTION_BLOCK_ADDRESS_ON    = 1,          /**< Specifies on the blocking by address.*/
};

/**
* This enumeration specifies the blocking of subject.
*/
enum
{
	EMAIL_OPTION_BLOCK_SUBJECT_OFF   = 0,          /**< Specifies off the blocking by subject.*/
	EMAIL_OPTION_BLOCK_SUBJECT_ON    = 1,          /**< Specifies on the blocking by subject.*/
};

enum
{
	EMAIL_LIST_TYPE_UNREAD           = -3,
	EMAIL_LIST_TYPE_LOCAL            = -2,
	EMAIL_LIST_TYPE_THREAD           = -1,
	EMAIL_LIST_TYPE_NORMAL           = 0
};

/**
* This enumeration specifies the mailbox sync type.
*/
enum
{
	EMAIL_MAILBOX_ALL                = -1,         /**< All mailboxes.*/
	EMAIL_MAILBOX_FROM_SERVER        = 0,          /**< Mailboxes from server. */
	EMAIL_MAILBOX_FROM_LOCAL         = 1,          /**< Mailboxes from local. */
};

typedef enum
{
	APPEND_BODY                    = 1,
	UPDATE_MAILBOX,
	UPDATE_ATTACHMENT_INFO,
	UPDATE_FLAG,
	UPDATE_SAVENAME,
	UPDATE_EXTRA_FLAG,
	UPDATE_MAIL,
	UPDATE_DATETIME,
	UPDATE_FROM_CONTACT_INFO,
	UPDATE_TO_CONTACT_INFO,
	UPDATE_ALL_CONTACT_NAME,
	UPDATE_ALL_CONTACT_INFO,
	UPDATE_STICKY_EXTRA_FLAG,
	UPDATE_PARTIAL_BODY_DOWNLOAD,
	UPDATE_MEETING,
	UPDATE_SEEN_FLAG_OF_THREAD,
	UPDATE_FILE_PATH,
} email_mail_change_type_t; // Should be moved intenal types */


/**
* This enumeration specifies the address type.
*/
typedef enum
{
	EMAIL_ADDRESS_TYPE_FROM          = 1,          /**< Specifies the from address.*/
	EMAIL_ADDRESS_TYPE_TO,                         /**< Specifies the to recipient address.*/
	EMAIL_ADDRESS_TYPE_CC,                         /**< Specifies the cc recipient address.*/
	EMAIL_ADDRESS_TYPE_BCC,                        /**< Specifies the bcc recipient address.*/
	EMAIL_ADDRESS_TYPE_REPLY,                      /**< Specifies the reply recipient address.*/
	EMAIL_ADDRESS_TYPE_RETURN,                     /**< Specifies the return recipient address.*/
} email_address_type_t;

/**
 * This enumeration specifies the search type for searching mailbox.
 */
typedef enum
{
	EMAIL_MAILBOX_SEARCH_KEY_TYPE_SUBJECT,         /**< The search key is for searching subject.*/
	EMAIL_MAILBOX_SEARCH_KEY_TYPE_FROM,            /**< The search key is for searching sender address.*/
	EMAIL_MAILBOX_SEARCH_KEY_TYPE_BODY,            /**< The search key is for searching body.*/
	EMAIL_MAILBOX_SEARCH_KEY_TYPE_TO,              /**< The search key is for searching recipient address.*/
} email_mailbox_search_key_t;

/**
 * This enumeration specifies the download status of mail body.
 */

typedef enum
{
	EMAIL_BODY_DOWNLOAD_STATUS_NONE = 0,
	EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED = 1,
	EMAIL_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED = 2,
} email_body_download_status_t;

/**
 * This enumeration specifies the moving type.
 */
typedef enum
{
	EMAIL_MOVED_BY_COMMAND = 0,
	EMAIL_MOVED_BY_MOVING_THREAD,
	EMAIL_MOVED_AFTER_SENDING,
	EMAIL_MOVED_CANCELATION_MAIL
} email_move_type;

/**
 * This enumeration specifies the deletion type.
 */
typedef enum
{
	EMAIL_DELETED_BY_COMMAND = 0,
	EMAIL_DELETED_BY_OVERFLOW,
	EMAIL_DELETED_BY_DELETING_THREAD,
	EMAIL_DELETED_BY_MOVING_TO_OTHER_ACCOUNT,
	EMAIL_DELETED_AFTER_SENDING,
	EMAIL_DELETED_FROM_SERVER,
} email_delete_type;

/**
 * This enumeration specifies the status field type.
 */
typedef enum
{
	EMAIL_FLAGS_SEEN_FIELD = 0,
	EMAIL_FLAGS_DELETED_FIELD,
	EMAIL_FLAGS_FLAGGED_FIELD,
	EMAIL_FLAGS_ANSWERED_FIELD,
	EMAIL_FLAGS_RECENT_FIELD,
	EMAIL_FLAGS_DRAFT_FIELD,
	EMAIL_FLAGS_FORWARDED_FIELD,
	EMAIL_FLAGS_FIELD_COUNT,
} email_flags_field_type;

typedef enum {
	EMAIL_FLAG_NONE                  = 0,
	EMAIL_FLAG_FLAGED                = 1,
	EMAIL_FLAG_TASK_STATUS_CLEAR     = 2,
	EMAIL_FLAG_TASK_STATUS_COMPLETE  = 3,
	EMAIL_FLAG_TASK_STATUS_ACTIVE    = 4,
} email_flagged_type;

typedef enum {
	EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       =  1,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_UID              =  2,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_BCC              =  7,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_CC               =  9,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_FROM             = 10,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_KEYWORD          = 11,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_SUBJECT          = 13,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_TO               = 15,  /* string type */
	EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      = 16,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     = 17,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE = 20,  /* time type */
	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     = 21,  /* time type */
	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  = 22,  /* time type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   = 26,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    = 28,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      = 30,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     = 32,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     = 34,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       = 36,  /* integer type */
	EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       = 43,  /* string type */
} email_search_filter_type;

typedef enum {
	EMAIL_DRM_TYPE_NONE                       = 0,
	EMAIL_DRM_TYPE_OBJECT                     = 1,
	EMAIL_DRM_TYPE_RIGHT                      = 2,
	EMAIL_DRM_TYPE_DCF                        = 3
} email_drm_type;

typedef enum {
	EMAIL_DRM_METHOD_NONE                     = 0,
	EMAIL_DRM_METHOD_FL                       = 1,
	EMAIL_DRM_METHOD_CD                       = 2,
	EMAIL_DRM_METHOD_SSD                      = 3,
	EMAIL_DRM_METHOD_SD                       = 4
} email_drm_method;

typedef enum {
	EMAIL_CANCELED_BY_USER                    = 0,
	EMAIL_CANCELED_BY_MDM                     = 1,
} email_cancelation_type;

typedef enum {
	EMAIL_MAIL_ATTRIBUTE_MAIL_ID                 =  0,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID              =  1,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_MAILBOX_ID              =  2,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME            =  3,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE            =  4,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_SUBJECT                 =  5,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_DATE_TIME               =  6,  /* datetime type */
	EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_STATUS      =  7,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_SERVER_MAILBOX_NAME     =  8,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_ID          =  9,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_MESSAGE_ID              = 10,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_REFERENCE_MAIL_ID       = 11,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_FROM                    = 12,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_TO                      = 13,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_CC                      = 14,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_BCC                     = 15,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_BODY_DOWNLOAD_STATUS    = 16,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_FILE_PATH_PLAIN         = 17,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_FILE_PATH_HTML          = 18,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_FILE_SIZE               = 19,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD        = 20,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD     = 21,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD     = 22,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_FLAGS_ANSWERED_FIELD    = 23,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_FLAGS_RECENT_FIELD      = 24,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_FLAGS_DRAFT_FIELD       = 25,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_FLAGS_FORWARDED_FIELD   = 26,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_DRM_STATUS              = 27,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_PRIORITY                = 28,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS             = 29,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_LOCK_STATUS             = 30,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_REPORT_STATUS           = 31,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_ATTACHMENT_COUNT        = 32,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_INLINE_CONTENT_COUNT    = 33,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_THREAD_ID               = 34,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_THREAD_ITEM_COUNT       = 35,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_PREVIEW_TEXT            = 36,  /* string type */
	EMAIL_MAIL_ATTRIBUTE_MEETING_REQUEST_STATUS  = 37,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_MESSAGE_CLASS           = 38,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_DIGEST_TYPE             = 39,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_SMIME_TYPE              = 40,  /* integer type */
	EMAIL_MAIL_ATTRIBUTE_END                         
} email_mail_attribute_type;

typedef enum {
	EMAIL_ADD_MY_ADDRESS_OPTION_DO_NOT_ADD            = 0,
	EMAIL_ADD_MY_ADDRESS_OPTION_ALWAYS_ADD_TO_CC      = 1,
	EMAIL_ADD_MY_ADDRESS_OPTION_ALWAYS_ADD_TO_BCC     = 2,
} email_add_my_address_option_type;

typedef enum {
	EMAIL_MESSAGE_CLASS_UNSPECIFIED,
	EMAIL_MESSAGE_CLASS_UNKNOWN,
	EMAIL_MESSAGE_CLASS_NOTE,
	EMAIL_MESSAGE_CLASS_NOTE_RULES_OF_TEMPLATE_MICROSOFT,
	EMAIL_MESSAGE_CLASS_NOTE_SMIME,
	EMAIL_MESSAGE_CLASS_NOTE_SMIME_MULTIPART_SIGNED,
	EMAIL_MESSAGE_CLASS_NOTIFICATION_MEETING,
	EMAIL_MESSAGE_CLASS_OCTEL_VOICE,
	EMAIL_MESSAGE_CLASS_SCHEDULE_MEETING_REQUEST,
	EMAIL_MESSAGE_CLASS_SCHEDULE_MEETING_CANCELED,
	EMAIL_MESSAGE_CLASS_SCHEDULE_MEETING_RESP_POS,
	EMAIL_MESSAGE_CLASS_SCHEDULE_MEETING_RESP_TENT,
	EMAIL_MESSAGE_CLASS_SCHEDULE_MEETING_RESP_NEG,
	EMAIL_MESSAGE_CLASS_POST,
	EMAIL_MESSAGE_CLASS_INFO_PATH_FORM,
	EMAIL_MESSAGE_CLASS_VOICE_NOTES,
	EMAIL_MESSAGE_CLASS_SHARING,
	EMAIL_MESSAGE_CLASS_NOTE_EXCHANGE_ACTIVE_SYNC_REMOTE_WIPE_CONFIRMATION,
	EMAIL_MESSAGE_CLASS_VOICE_MAIL,
	EMAIL_MESSAGE_CLASS_SMS,
	EMAIL_MESSAGE_CLASS_IRM_MESSAGE                                         = 0x00100000,
	EMAIL_MESSAGE_CLASS_SMART_REPLY                                         = 0x01000000,
	EMAIL_MESSAGE_CLASS_SMART_FORWARD                                       = 0x02000000,
	EMAIL_MESSAGE_CLASS_REPORT_NOT_READ_REPORT                              = 0x10000000,
	EMAIL_MESSAGE_CLASS_REPORT_READ_REPORT                                  = 0x20000000,
	EMAIL_MESSAGE_CLASS_REPORT_NON_DELIVERY_RECEIPT                         = 0x40000000,
	EMAIL_MESSAGE_CLASS_REPORT_DELIVERY_RECEIPT                             = 0x80000000,
} email_message_class; 

typedef enum{
	EMAIL_SMIME_NONE                          = 0,   /* Not use smime */
	EMAIL_SMIME_SIGNED,                               
	EMAIL_SMIME_ENCRYPTED,
	EMAIL_SMIME_SIGNED_AND_ENCRYPTED,
} email_smime_type;

typedef enum {
	CIPHER_TYPE_DES3                          = 0,
	CIPHER_TYPE_DES,
	CIPHER_TYPE_RC2_128,
	CIPHER_TYPE_RC2_64,
	CIPHER_TYPE_RC2_40,                       
} email_cipher_type;

typedef enum {
	DIGEST_TYPE_SHA1                          = 0,
	DIGEST_TYPE_MD5,
} email_digest_type;

/*****************************************************************************/
/*  Data Structures                                                          */
/*****************************************************************************/

/**
 * This structure is used to save mail time.
 */
typedef struct
{
	unsigned short year;                         /**< Specifies the Year.*/
	unsigned short month;                        /**< Specifies the Month.*/
	unsigned short day;                          /**< Specifies the Day.*/
	unsigned short hour;                         /**< Specifies the Hour.*/
	unsigned short minute;                       /**< Specifies the Minute.*/
	unsigned short second;                       /**< Specifies the Second.*/
} email_datetime_t DEPRECATED;

/**
 * This structure is used to save the options.
 */
typedef struct
{
	email_mail_priority_t              priority;               /**< Specifies the prority. 1=high 3=normal 5=low.*/
	int                                keep_local_copy;        /**< Specifies the saving save a copy after sending.*/
	int                                req_delivery_receipt;   /**< Specifies the request of delivery report. 0=off 1=on*/
	int                                req_read_receipt;       /**< Specifies the request of read receipt. 0=off 1=on*/
	int                                download_limit;         /**< Specifies the limit of size for downloading.*/
	int                                block_address;          /**< Specifies the blocking of address. 0=off 1=on*/
	int                                block_subject;          /**< Specifies the blocking of subject. 0=off 1=on*/
	char                              *display_name_from;      /**< Specifies the display name of from.*/
	int                                reply_with_body;        /**< Specifies the replying with body 0=off 1=on*/
	int                                forward_with_files;     /**< Specifies the fowarding with files 0=off 1=on*/
	int                                add_myname_card;        /**< Specifies the adding name card 0=off 1=on*/
	int                                add_signature;          /**< Specifies the adding signature 0=off 1=on*/
	char                              *signature;              /**< Specifies the signature*/
	email_add_my_address_option_type   add_my_address_to_bcc;  /**< Specifies whether cc or bcc field should be always filled with my address. */
} email_option_t;

/**
 * This structure is used to save the information of email account.
 */
typedef struct
{
	/* General account information */
	int                          account_id;                               /**< Account id  */
	char                        *account_name;                             /**< Account name */
	int                          account_svc_id;                           /**< AccountSvc priv data: Specifies id from account-svc */
	int                          sync_status;                              /**< Sync Status. SYNC_STATUS_FINISHED, SYNC_STATUS_SYNCING, SYNC_STATUS_HAVE_NEW_MAILS */
	int                          sync_disabled;                            /**< If this attriube is set as true, email-service will not synchronize this account. */
	int                          default_mail_slot_size;
	char                        *logo_icon_path;                           /**< account logo icon (used by account svc and email app) */
	void                        *user_data;                                /**< binary user data */
	int				             user_data_length;                         /**< user data length */

	/* User information */
	char                     	*user_display_name;                        /**< User's display */
	char                     	*user_email_address;                       /**< User's email address */
	char                        *reply_to_address;                         /**< Email address for reply */
	char                        *return_address;                           /**< Email address for error from server*/

	/* Configuration for incoming server */
	email_account_server_t       incoming_server_type;                     /**< Incoming server type */
	char                        *incoming_server_address;                  /**< Incoming server address */
	int                          incoming_server_port_number;              /**< Incoming server port number */
	char                        *incoming_server_user_name;                /**< Incoming server user name */
	char                        *incoming_server_password;                 /**< Incoming server password */
	int                          incoming_server_secure_connection;        /**< Does incoming server requires secured connection? */

	/* Options for incoming server */
	email_imap4_retrieval_mode_t retrieval_mode;                           /**< Retrieval mode : EMAIL_IMAP4_RETRIEVAL_MODE_NEW or EMAIL_IMAP4_RETRIEVAL_MODE_ALL */
	int                          keep_mails_on_pop_server_after_download;  /**< Keep mails on POP server after download */
	int                          check_interval;                           /**< Specifies the interval for checking new mail periodically */
	int                          auto_download_size;                       /**< Specifies the size for auto download in bytes. -1 means entire mails body */

	/* Configuration for outgoing server */
	email_account_server_t       outgoing_server_type;                     /**< Outgoing server type */
	char				        *outgoing_server_address;                  /**< Outgoing server address */
	int					         outgoing_server_port_number;              /**< Outgoing server port number */
	char 				        *outgoing_server_user_name;                /**< Outgoing server user name */
	char 				        *outgoing_server_password;                 /**< Outgoing server password */
	int 				         outgoing_server_secure_connection;        /**< Does outgoing server requires secured connection? */
	int 				         outgoing_server_need_authentication;      /**< Does outgoing server requires authentication? */
	int 				         outgoing_server_use_same_authenticator;   /**< Use same authenticator for outgoing server */


	/* Options for outgoing server */
	email_option_t 		         options;

	/* Authentication Options */
	int	                         pop_before_smtp;                          /**< POP before SMTP Authentication */
	int                          incoming_server_requires_apop;            /**< APOP authentication */

	/* S/MIME Options */
	email_smime_type             smime_type;                               /**< Specifies the smime type 0=Normal 1=Clear signed 2=encrypted 3=Signed + encrypted */
	char                        *certificate_path;                         /**< Specifies the certificate path of private*/
	email_cipher_type            cipher_type;                              /**< Specifies the encryption algorithm*/
	email_digest_type            digest_type;                              /**< Specifies the digest algorithm*/
} email_account_t;

/**
 * This structure is used to save the information of certificate
 */

typedef struct 
{
	int certificate_id;
	int issue_year;
	int issue_month;
	int issue_day;
	int expiration_year;
	int expiration_month;
	int expiration_day;
	char *issue_organization_name;
	char *email_address;
	char *subject_str;
	char *filepath;
} email_certificate_t;

/**
 * This structure is used to save the information of email server.
 */

typedef struct
{
	int                         configuration_id;      /**< Specifies the id of configuration.*/
	email_account_server_t      protocol_type;         /**< Specifies the type of configuration.*/
	char                       *server_addr;           /**< Specifies the address of configuration.*/
	int                         port_number;           /**< Specifies the port number of configuration.*/
	int                         security_type;         /**< Specifies the security such as SSL.*/
	int                         auth_type;             /**< Specifies the authentication type of configuration.*/
} email_protocol_config_t;

typedef struct
{
	char                       *service_name;           /**< Specifies the name of service.*/
	int                         authname_format;        /**< Specifies the type of user name for authentication.*/
	int                         protocol_conf_count;    /**< Specifies count of protocol configurations.*/
	email_protocol_config_t    *protocol_config_array;  /**< Specifies array of protocol configurations.*/
} email_server_info_t;

typedef struct
{
	int   mailbox_type;
	char  mailbox_name[MAILBOX_NAME_LENGTH];
} email_mailbox_type_item_t;

/**
 * This structure is contains the Mail information.
 */

typedef struct
{
	int                   mail_id;                 /**< Specifies the Mail ID.*/
	int                   account_id;              /**< Specifies the Account ID.*/
	int                   mailbox_id;              /**< Specifies the Mailbox ID.*/
	email_mailbox_type_e  mailbox_type;            /**< Specifies the mailbox type of the mail. */
	char                 *subject;                 /**< Specifies the subject.*/
	time_t                date_time;               /**< Specifies the Date time.*/
	int                   server_mail_status;      /**< Specifies the Whether sever mail or not.*/
	char                 *server_mailbox_name;     /**< Specifies the server mailbox.*/
	char                 *server_mail_id;          /**< Specifies the Server Mail ID.*/
	char                 *message_id;              /**< Specifies the message id */
	int                   reference_mail_id;       /**< Specifies the reference mail id */
	char                 *full_address_from;       /**< Specifies the From address.*/
	char                 *full_address_reply;      /**< Specifies the Reply to address */
	char                 *full_address_to;         /**< Specifies the To address.*/
	char                 *full_address_cc;         /**< Specifies the CC address.*/
	char                 *full_address_bcc;        /**< Specifies the BCC address*/
	char                 *full_address_return;     /**< Specifies the return Path*/
	char                 *email_address_sender;    /**< Specifies the email address of sender.*/
	char                 *email_address_recipient; /**< Specifies the email address of recipients.*/
	char                 *alias_sender;            /**< Specifies the alias of sender. */
	char                 *alias_recipient;         /**< Specifies the alias of recipients. */
	int                   body_download_status;    /**< Specifies the Text downloaded or not.*/
	char                 *file_path_plain;         /**< Specifies the path of text mail body.*/
	char                 *file_path_html;          /**< Specifies the path of HTML mail body.*/
	char                 *file_path_mime_entity;   /**< Specifies the path of MIME entity. */
	int                   mail_size;               /**< Specifies the Mail Size.*/
	char                  flags_seen_field;        /**< Specifies the seen flags*/
	char                  flags_deleted_field;     /**< Specifies the deleted flags*/
	char                  flags_flagged_field;     /**< Specifies the flagged flags*/
	char                  flags_answered_field;    /**< Specifies the answered flags*/
	char                  flags_recent_field;      /**< Specifies the recent flags*/
	char                  flags_draft_field;       /**< Specifies the draft flags*/
	char                  flags_forwarded_field;   /**< Specifies the forwarded flags*/
	int                   DRM_status;              /**< Has the mail DRM content? (1 : true, 0 : false) */
	email_mail_priority_t priority;                /**< Specifies the priority of the mail.*/
	email_mail_status_t   save_status;             /**< Specifies the save status*/
	int                   lock_status;             /**< Specifies the mail is locked*/
	email_mail_report_t   report_status;           /**< Specifies the Mail Report.*/
	int                   attachment_count;        /**< Specifies the attachment count. */
	int                   inline_content_count;    /**< Specifies the inline content count. */
	int                   thread_id;               /**< Specifies the thread id for thread view. */
	int                   thread_item_count;       /**< Specifies the item count of specific thread. */
	char                 *preview_text;            /**< Specifies the preview body. */
	email_mail_type_t     meeting_request_status;  /**< Specifies the status of meeting request. */
	int                   message_class;           /**< Specifies the class of message for EAS. */ /* email_message_class */
	email_digest_type     digest_type;             /**< Specifies the digest algorithm*/
	email_smime_type      smime_type;              /**< Specifies the SMIME type. */
} email_mail_data_t;

/**
 * This structure is contains information for displaying mail list.
 */
typedef struct
{
	int                   mail_id;                                            /**< Specifies the mail id.*/
	int                   account_id;                                         /**< Specifies the account id.*/
	int                   mailbox_id;                                         /**< Specifies the mailbox id.*/
	char                  full_address_from[STRING_LENGTH_FOR_DISPLAY];       /**< Specifies the full from email address.*/
	char                  email_address_sender[MAX_EMAIL_ADDRESS_LENGTH];     /**< Specifies the sender email address.*/
	char                  email_address_recipient[STRING_LENGTH_FOR_DISPLAY]; /**< Specifies the recipients email address.*/
	char                  subject[STRING_LENGTH_FOR_DISPLAY];                 /**< Specifies the subject.*/
	int                   body_download_status;                               /**< Specifies the text donwloaded or not.*/
	time_t                date_time;                                          /**< Specifies the date time.*/
	char                  flags_seen_field;                                   /**< Specifies the seen flags*/
	char                  flags_deleted_field;                                /**< Specifies the deleted flags*/
	char                  flags_flagged_field;                                /**< Specifies the flagged flags*/
	char                  flags_answered_field;                               /**< Specifies the answered flags*/
	char                  flags_recent_field;                                 /**< Specifies the recent flags*/
	char                  flags_draft_field;                                  /**< Specifies the draft flags*/
	char                  flags_forwarded_field;                              /**< Specifies the forwarded flags*/
	int                   DRM_status;                                         /**< Has the mail DRM content? (1 : true, 0 : false) */
	email_mail_priority_t priority;                                           /**< Specifies the priority of Mails.*/ /* email_mail_priority_t*/
	email_mail_status_t   save_status;                                        /**< Specifies the save status*/ /* email_mail_status_t */
	int                   lock_status;                                        /**< Specifies the locked*/
	email_mail_report_t   report_status;                                      /**< Specifies the mail report.*/ /* email_mail_report_t */
	int                   attachment_count;                                   /**< Specifies the attachment count. */
	int                   inline_content_count;                               /**< Specifies the inline content count. */
	char                  preview_text[MAX_PREVIEW_TEXT_LENGTH];              /**< text for preview body*/
	int                   thread_id;                                          /**< thread id for thread view*/
	int                   thread_item_count;                                  /**< item count of specific thread */
	email_mail_type_t     meeting_request_status;                             /**< Whether the mail is a meeting request or not */ /* email_mail_type_t */
	int                   message_class;                                      /**< Specifies the message class */ /* email_message_class */
	email_smime_type      smime_type;                                         /**< Specifies the smime type */ /* email_smime_type */
} email_mail_list_item_t;

/**
 * This structure is used to save the filtering structure.
 */
typedef struct
{
	int                  account_id;          /**< Specifies the account ID.*/
	int                  filter_id;           /**< Specifies the filtering ID.*/
	email_rule_type_t    type;                /**< Specifies the filtering type.*/
	char                *value;               /**< Specifies the filtering value.*/
	email_rule_action_t  faction;             /**< Specifies the action type for filtering.*/
	int                  target_mailbox_id;   /**< Specifies the mail box if action type means move.*/
	int                  flag1;               /**< Specifies the activation.*/
	int                  flag2;               /**< Reserved.*/
} email_rule_t;

/**
 * This structure is used to save the information of mail flag.
 */
typedef struct
{
	unsigned int  seen           : 1; /**< Specifies the read email.*/
	unsigned int  deleted        : 1; /**< Reserved.*/
	unsigned int  flagged        : 1; /**< Specifies the flagged email.*/
	unsigned int  answered       : 1; /**< Reserved.*/
	unsigned int  recent         : 1; /**< Reserved.*/
	unsigned int  draft          : 1; /**< Specifies the draft email.*/
	unsigned int  attachment_count : 1; /**< Reserved.*/
	unsigned int  forwarded      : 1; /**< Reserved.*/
	unsigned int  sticky         : 1; /**< Sticky flagged mails cannot be deleted */
} email_mail_flag_t /* DEPRECATED */;


/**
 * This structure is used to save the information of mail extra flag.
 */
typedef struct
{
	unsigned int  priority         : 3; /**< Specifies the mail priority.
                                           The value is greater than or equal to EMAIL_MAIL_PRIORITY_HIGH.
                                           The value is less than or equal to EMAIL_MAIL_PRIORITY_LOW.*/
	unsigned int  status           : 4; /**< Specifies the mail status.
	                                       The value is a value of enumeration email_mail_status_t.*/
	unsigned int  noti             : 1; /**< Specifies the notified mail.*/
	unsigned int  lock             : 1; /**< Specifies the locked mail.*/
	unsigned int  report           : 2; /**< Specifies the MDN/DSN mail. The value is a value of enumeration email_mail_report_t.*/
	unsigned int  drm              : 1; /**< Specifies the drm mail.*/
	unsigned int  text_download_yn : 2; /**< body download y/n*/
} email_extra_flag_t DEPRECATED;

/**
 * This structure is used to save the information of attachment.
 */
typedef struct
{
	int   attachment_id;
	char *attachment_name;
	char *attachment_path;
	int   attachment_size;
	int   mail_id;
	int   account_id;
	char  mailbox_id;
	int   save_status;
	int   drm_status;
	int   inline_content_status;
	char *attachment_mime_type; /**< Specifies the context mime type.*/
} email_attachment_data_t;

typedef struct
{
	int        offset_from_GMT;
	char       standard_name[32];
	struct tm  standard_time_start_date;
	int        standard_bias;
	char       daylight_name[32];
	struct tm  daylight_time_start_date;
	int        daylight_bias;
} email_time_zone_t;

typedef struct
{
	int                        mail_id;                                        /**< Specifies the mail id of meeting request on DB. This is the primary key. */
	email_meeting_response_t   meeting_response;                               /**< Specifies the meeting response. */
	struct tm                  start_time;
	struct tm                  end_time;
	char                      *location;                                       /**< Specifies the location of meeting. Maximum length of this string is 32768 */
	char                      *global_object_id;                               /**< Specifies the object id. */
	email_time_zone_t          time_zone;
} email_meeting_request_t;

/**
 * This structure is used to save the information of sender list with unread/total mail counts
 */
typedef struct
{
	char *address;         /**< Specifies the address of a sender.*/
	char *display_name;    /**< Specifies a display name. This may be one of contact name, alias in original mail and email address of sender. (Priority order : contact name, alias, email address) */
	int   unread_count;    /**< Specifies the number of unread mails received from sender address*/
	int   total_count;     /**< Specifies the total number of  mails which are received from sender address*/
} email_sender_list_t /* DEPRECATED */;


/**
 * This structure is used to save the information of mailbox.
 */
typedef struct
{
	int                   mailbox_id;                 /**< Unique id on mailbox table.*/
	char                 *mailbox_name;               /**< Specifies the path of mailbox.*/
	email_mailbox_type_e  mailbox_type;               /**< Specifies the type of mailbox */
	char                 *alias;                      /**< Specifies the display name of mailbox.*/
	int                   unread_count;               /**< Specifies the unread mail count in the mailbox.*/
	int                   total_mail_count_on_local;  /**< Specifies the total number of mails in the mailbox in the local DB.*/
	int                   total_mail_count_on_server; /**< Specifies the total number of mails in the mailbox in the mail server.*/
	int                   local;                      /**< Specifies the local mailbox.*/
	int                   account_id;                 /**< Specifies the account ID for mailbox.*/
	int                   mail_slot_size;             /**< Specifies how many mails can be stored in local mailbox.*/
	int                   no_select;                  /**< Specifies the 'no_select' attribute from xlist.*/
	time_t                last_sync_time;
	int                   deleted_flag;               /**< Specifies whether mailbox is deleted.*/
} email_mailbox_t;

typedef struct
{
	email_address_type_t  address_type;
	char                 *address;
	char                 *display_name;
	int                   storage_type;
	int                   contact_id;
} email_address_info_t;

typedef struct
{
	GList *from;
	GList *to;
	GList *cc;
	GList *bcc;
} email_address_info_list_t;

typedef struct
{
	int	address_type;		/* type of mail (sender : 0, recipient : 1)*/
	int     address_count;  /*  The number of email addresses */
	char  **address_list;   /*  strings of email addresses */
} email_email_address_list_t;


typedef struct _email_search_filter_t {
	email_search_filter_type search_filter_type; /* type of search filter */
	union {
		int            integer_type_key_value;
		time_t         time_type_key_value;
		char          *string_type_key_value;
	} search_filter_key_value;
} email_search_filter_t;

typedef enum {
	EMAIL_LIST_FILTER_RULE_EQUAL                  = 0,
	EMAIL_LIST_FILTER_RULE_NOT_EQUAL              = 1,
	EMAIL_LIST_FILTER_RULE_LESS_THAN              = 2,
	EMAIL_LIST_FILTER_RULE_GREATER_THAN           = 3,
	EMAIL_LIST_FILTER_RULE_LESS_THAN_OR_EQUAL     = 4,
	EMAIL_LIST_FILTER_RULE_GREATER_THAN_OR_EQUAL  = 5,
	EMAIL_LIST_FILTER_RULE_INCLUDE                = 6,
	EMAIL_LIST_FILTER_RULE_IN                     = 7,
	EMAIL_LIST_FILTER_RULE_NOT_IN                 = 8
} email_list_filter_rule_type_t;

typedef enum {
	EMAIL_CASE_SENSITIVE                          = 0,
	EMAIL_CASE_INSENSITIVE                        = 1,
} email_list_filter_case_sensitivity_t;

typedef union {
	int                                    integer_type_value;
	char                                  *string_type_value;
	time_t                                 datetime_type_value;
} email_mail_attribute_value_t;

typedef struct {
	email_list_filter_rule_type_t          rule_type;
	email_mail_attribute_type              target_attribute;
	email_mail_attribute_value_t           key_value;
	email_list_filter_case_sensitivity_t   case_sensitivity;
} email_list_filter_rule_t;

typedef enum {
	EMAIL_LIST_FILTER_ITEM_RULE                     = 0,
	EMAIL_LIST_FILTER_ITEM_OPERATOR                 = 1,
} email_list_filter_item_type_t;

typedef enum {
	EMAIL_LIST_FILTER_OPERATOR_AND                  = 0,
	EMAIL_LIST_FILTER_OPERATOR_OR                   = 1,
	EMAIL_LIST_FILTER_OPERATOR_LEFT_PARENTHESIS     = 2,
	EMAIL_LIST_FILTER_OPERATOR_RIGHT_PARENTHESIS    = 3
} email_list_filter_operator_type_t;

typedef struct {
	email_list_filter_item_type_t          list_filter_item_type;

	union {
		email_list_filter_rule_t           rule;
		email_list_filter_operator_type_t  operator_type;
	} list_filter_item;

} email_list_filter_t;

typedef enum {
	EMAIL_SORT_ORDER_ASCEND                             = 0,
	EMAIL_SORT_ORDER_DESCEND                            = 1
} email_list_filter_sort_order_t;

typedef struct {
	email_mail_attribute_type              target_attribute;
	bool                                   force_boolean_check;
	email_list_filter_sort_order_t         sort_order;
} email_list_sorting_rule_t;

/*****************************************************************************/
/*  For Active Sync                                                          */
/*****************************************************************************/

#define VCONFKEY_EMAIL_SERVICE_ACTIVE_SYNC_HANDLE "db/email_handle/active_sync_handle"
#define EMAIL_ACTIVE_SYNC_NOTI                      "User.Email.ActiveSync"

typedef enum
{
	ACTIVE_SYNC_NOTI_SEND_MAIL,                           /*  a sending notification to ASE (active sync engine */
	ACTIVE_SYNC_NOTI_SEND_SAVED,                          /*  a sending notification to ASE (active sync engine), All saved mails should be sent */
	ACTIVE_SYNC_NOTI_SEND_REPORT,                         /*  a sending notification to ASE (active sync engine), report should be sen */
	ACTIVE_SYNC_NOTI_SYNC_HEADER,                         /*  a sync header - download mails from server. */
                                                          /*  It is depended on account/s flag1 field whether it excutes downloading header only or downloading header + body */
                                                          /*  downloading option : 0 is subject only, 1 is text body, 2 is normal */
	ACTIVE_SYNC_NOTI_DOWNLOAD_BODY,                       /*  a downloading body notification to AS */
	ACTIVE_SYNC_NOTI_DOWNLOAD_ATTACHMENT,                 /*  a downloading attachment notification to AS */
	ACTIVE_SYNC_NOTI_VALIDATE_ACCOUNT,                    /*  a account validating notification to AS */
	ACTIVE_SYNC_NOTI_CANCEL_JOB,                          /*  a cancling job notification to AS */
	ACTIVE_SYNC_NOTI_SEARCH_ON_SERVER,                    /*  a searching on server notification to AS */
	ACTIVE_SYNC_NOTI_CLEAR_RESULT_OF_SEARCH_ON_SERVER,    /*  a notification for clearing result of search on server to AS */
	ACTIVE_SYNC_NOTI_EXPUNGE_MAILS_DELETED_FLAGGED,       /*  a notification to expunge deleted flagged mails */
	ACTIVE_SYNC_NOTI_RESOLVE_RECIPIENT,                   /*  a notification to get the resolve recipients */
	ACTIVE_SYNC_NOTI_VALIDATE_CERTIFICATE,                /*  a notification to validate certificate */
	ACTIVE_SYNC_NOTI_ADD_MAILBOX,                         /*  a notification to add mailbox */
	ACTIVE_SYNC_NOTI_RENAME_MAILBOX,                      /*  a notification to rename mailbox */
	ACTIVE_SYNC_NOTI_DELETE_MAILBOX,                      /*  a notification to delete mailbox */
	ACTIVE_SYNC_NOTI_CANCEL_SENDING_MAIL,                 /*  a notification to cancel a sending mail */
	ACTIVE_SYNC_NOTI_DELETE_MAILBOX_EX,                   /*  a notification to delete multiple mailboxes */
}	eactivesync_noti_t;

typedef union
{
	struct _send_mail
	{
		int             handle;
		int             account_id;
		int             mail_id;
	} send_mail;

	struct _send_mail_saved
	{/*  not defined ye */
		int             handle;
		int             account_id;
	} send_mail_saved;

	struct _send_report
	{/*  not defined ye */
		int             handle;
		int             account_id;
	} send_report;

	struct _sync_header
	{
		int             handle;
		int             account_id;
		int             mailbox_id;
	} sync_header;

	struct _download_body
	{
		int             handle;
		int             account_id;
		int             mail_id;
		int             with_attachment;      /*  0: without attachments, 1: with attachment */
	} download_body;

	struct _download_attachment
	{
		int             handle;
		int             account_id;
		int             mail_id;
		int             attachment_order;
	} download_attachment;

	struct _cancel_job
	{
		int             account_id;
		int             handle;               /*  job handle to be canceled. this value is issued by email-service. */
		int             cancel_type;
	} cancel_job;

	struct _validate_account
	{/*  not defined yet */
		int             handle;
		int             account_id;
	} validate_account;

	struct _search_mail_on_server
	{
		int                    handle;
		int                    account_id;
		int					   mailbox_id;
		email_search_filter_t *search_filter_list;
		int                    search_filter_count;
	} search_mail_on_server;

	struct _clear_result_of_search_mail_on_server
	{
		int                    handle;
		int                    account_id;
	} clear_result_of_search_mail_on_server;

	struct _expunge_mails_deleted_flagged
	{
		int                    handle;
		int                    mailbox_id;
		int                    on_server;
	} expunge_mails_deleted_flagged;

	struct _get_resolve_recipients
	{
		int                     handle;
		int                     account_id;
		char                   *email_address;
	} get_resolve_recipients;

	struct _validate_certificate
	{
		int                     handle;
		int                     account_id;
		char                   *email_address;
	} validate_certificate;

	struct _add_mailbox
	{
		int                     handle;
		int                     account_id;
		char                   *mailbox_path;
		char                   *mailbox_alias;
	} add_mailbox;

	struct _rename_mailbox
	{
		int                     handle;
		int                     account_id;
		int                     mailbox_id;
		char                   *mailbox_name;
		char                   *mailbox_alias;
	} rename_mailbox;

	struct _delete_mailbox
	{
		int                     handle;
		int                     account_id;
		int                     mailbox_id;
	} delete_mailbox;

	struct _cancel_sending_mail
	{
		int                     mail_id;
	} cancel_sending_mail;

	struct _delete_mailbox_ex
	{
		int                     handle;
		int                     account_id;
		int                    *mailbox_id_array;
		int                     mailbox_id_count;
		int                     on_server;
	} delete_mailbox_ex;

} ASNotiData;

/*  types for noti string */
typedef enum
{
	EMAIL_CONVERT_STRUCT_TYPE_MAIL_LIST_ITEM,      /** specifies email_mail_list_t */
} email_convert_struct_type_e;

/* Tasks */
typedef enum {
	/* Sync tasks */
	/* Sync tasks for account - from 11000 */
	EMAIL_SYNC_TASK_ADD_ACCOUNT                                      = 11010,
	EMAIL_SYNC_TASK_DELETE_ACCOUNT                                   = 11020,
	EMAIL_SYNC_TASK_UPDATE_ACCOUNT                                   = 11030,
	EMAIL_SYNC_TASK_GET_ACCOUNT                                      = 11040,
	EMAIL_SYNC_TASK_GET_ACCOUNT_LIST                                 = 11050,
	EMAIL_SYNC_TASK_BACKUP_ACCOUNTS                                  = 11060,
	EMAIL_SYNC_TASK_RESTORE_ACCOUNTS                                 = 11070,
	EMAIL_SYNC_TASK_GET_PASSWORD_LENGTH_OF_ACCOUNT                   = 11090,

	/* Sync tasks for mailbox - from 12000 */
	EMAIL_SYNC_TASK_GET_MAILBOX_COUNT                                = 12010,
	EMAIL_SYNC_TASK_GET_MAILBOX_LIST                                 = 12020,
	EMAIL_SYNC_TASK_GET_SUB_MAILBOX_LIST                             = 12030,
	EMAIL_SYNC_TASK_SET_MAIL_SLOT_SIZE                               = 12040,
	EMAIL_SYNC_TASK_SET_MAILBOX_TYPE                                 = 12050,
	EMAIL_SYNC_TASK_SET_LOCAL_MAILBOX                                = 12060,

	/* Sync tasks for mail - from 13000 */
	EMAIL_SYNC_GET_ATTACHMENT                                        = 13010,
	EMAIL_SYNC_CLEAR_RESULT_OF_SEARCH_MAIL_ON_SERVER                 = 13020,

	/* Sync tasks for mail thread - from 14000 */

	/* Sync tasks for rule - from 15000 */

	/* Sync tasks for etc - from 16000 */

	/* Async tasks */
	EMAIL_ASYNC_TASK_BOUNDARY                                        = 60000,
	/* Async tasks for account - from 61000 */
	EMAIL_ASYNC_TASK_VALIDATE_ACCOUNT                                = 61010,
	EMAIL_ASYNC_TASK_ADD_ACCOUNT_WITH_VALIDATION                     = 61020,

	/* Async tasks for mailbox - from 62000 */
	EMAIL_ASYNC_TASK_ADD_MAILBOX                                     = 62010,
	EMAIL_ASYNC_TASK_DELETE_MAILBOX                                  = 62020,
	EMAIL_ASYNC_TASK_RENAME_MAILBOX                                  = 62030,
	EMAIL_ASYNC_TASK_DOWNLOAD_IMAP_MAILBOX_LIST                      = 62040,
	EMAIL_ASYNC_TASK_DELETE_MAILBOX_EX                               = 62050,

	/* Async tasks for mail - from 63000 */
	EMAIL_ASYNC_TASK_ADD_MAIL                                        = 63010,
	EMAIL_ASYNC_TASK_ADD_READ_RECEIPT                                = 63020,

	EMAIL_ASYNC_TASK_UPDATE_MAIL                                     = 63030,

	EMAIL_ASYNC_TASK_DELETE_MAIL                                     = 63040,
	EMAIL_ASYNC_TASK_DELETE_ALL_MAIL                                 = 63050,
	EMAIL_ASYNC_TASK_EXPUNGE_MAILS_DELETED_FLAGGED                   = 63060,

	EMAIL_ASYNC_TASK_MOVE_MAIL                                       = 63070,
	EMAIL_ASYNC_TASK_MOVE_ALL_MAIL                                   = 63080,
	EMAIL_ASYNC_TASK_MOVE_MAILS_TO_MAILBOX_OF_ANOTHER_ACCOUNT        = 63090,

	EMAIL_ASYNC_TASK_SET_FLAGS_FIELD                                 = 63100,

	EMAIL_ASYNC_TASK_DOWNLOAD_MAIL_LIST                              = 63110,
	EMAIL_ASYNC_TASK_DOWNLOAD_BODY                                   = 63120,
	EMAIL_ASYNC_TASK_DOWNLOAD_ATTACHMENT                             = 63130,

	EMAIL_ASYNC_TASK_SEND_MAIL                                       = 63140,
	EMAIL_ASYNC_TASK_SEND_SAVED                                      = 63150,

	EMAIL_ASYNC_TASK_SEARCH_MAIL_ON_SERVER                           = 63160,

	/* Async tasks for mail thread - from 64000 */
	EMAIL_ASYNC_TASK_MOVE_THREAD_TO_MAILBOX                          = 64010,
	EMAIL_ASYNC_TASK_DELETE_THREAD                                   = 64020,
	EMAIL_ASYNC_TASK_MODIFY_SEEN_FLAG_OF_THREAD                      = 64030,

	/* Async tasks for rule - from 65000 */

	/* Async tasks for etc - from 66000 */

} email_task_type_t;

typedef enum
{
	EMAIL_TASK_STATUS_UNUSED                   = 0,
	EMAIL_TASK_STATUS_WAIT                     = 1,
	EMAIL_TASK_STATUS_STARTED                  = 2,
	EMAIL_TASK_STATUS_IN_PROGRESS              = 3,
	EMAIL_TASK_STATUS_FINISHED                 = 4,
	EMAIL_TASK_STATUS_FAILED                   = 5,
	EMAIL_TASK_STATUS_CANCELED                 = 6,
} email_task_status_type_t;

typedef enum
{
	EMAIL_TASK_PRIORITY_UNUSED                 = 0,
	EMAIL_TASK_PRIORITY_SCHEDULED              = 1,
	EMAIL_TASK_PRIORITY_LOW                    = 3,
	EMAIL_TASK_PRIORITY_MID                    = 5,
	EMAIL_TASK_PRIORITY_HIGH                   = 7,
	EMAIL_TASK_PRIORITY_BACK_GROUND_TASK       = 9,
} email_task_priority_t;
/* Tasks */

#ifdef __cplusplus
}
#endif

/**
* @} @}
*/

#endif /* __EMAIL_TYPES_H__ */
