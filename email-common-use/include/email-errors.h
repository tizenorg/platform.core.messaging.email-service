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


#ifndef __EMAIL_ERRORS_H__
#define __EMAIL_ERRORS_H__

#include <tizen_error.h>
/**
 * @file email-errors.h
 */

/**
 * @addtogroup EMAIL_SERVICE_FRAMEWORK
 * @{
 */

/*****************************************************************************/
/*  Errors                                                                   */
/*****************************************************************************/
#define EMAIL_ERROR_NONE                                 1       /**<  There is no error */

/* Error codes for invalid input */
#define EMAIL_ERROR_INVALID_PARAM                       -1001    /**<  Invalid parameter was given. - Invalid input parameter */
#define EMAIL_ERROR_INVALID_ACCOUNT                     -1002    /**<  Invalid account information was given. - Unsupported account */
#define EMAIL_ERROR_INVALID_SERVER                      -1005    /**<  Invalid server information was given. - Server unavailable */
#define EMAIL_ERROR_INVALID_MAIL                        -1006    /**<  Invalid mail information was given */
#define EMAIL_ERROR_INVALID_ADDRESS                     -1007    /**<  Invalid address information was given. - Incorrect address */
#define EMAIL_ERROR_INVALID_ATTACHMENT                  -1008    /**<  Invalid attachment information was given */
#define EMAIL_ERROR_INVALID_MAILBOX                     -1009    /**<  Invalid mailbox information was given */
#define EMAIL_ERROR_INVALID_FILTER                      -1010    /**<  Invalid filter information was given */
#define EMAIL_ERROR_INVALID_DATA                        -1012    /**<  Invalid data */
#define EMAIL_ERROR_INVALID_RESPONSE                    -1013    /**<  Unexpected network response was given. - Invalid server response */
#define EMAIL_ERROR_NO_RECIPIENT                        -1062    /**<  No recipients information was found */
#define EMAIL_ERROR_INVALID_FILE_PATH                   -4101    /**<  Invalid file path was given */
#define EMAIL_ERROR_INVALID_REFERENCE_MAIL              -4102    /**<  Invalid reference mail was given */

/* Error codes for missing data */
#define EMAIL_ERROR_ACCOUNT_NOT_FOUND                   -1014    /**<  No matched account was found */
#define EMAIL_ERROR_MAIL_NOT_FOUND                      -1015    /**<  No matched mail was found */
#define EMAIL_ERROR_MAILBOX_NOT_FOUND                   -1016    /**<  No matched mailbox was found */
#define EMAIL_ERROR_ATTACHMENT_NOT_FOUND                -1017    /**<  No matched attachment was found */
#define EMAIL_ERROR_FILTER_NOT_FOUND                    -1018    /**<  No matched filter was found */
#define EMAIL_ERROR_CONTACT_NOT_FOUND                   -1019    /**<  No matched contact was found */
#define EMAIL_ERROR_FILE_NOT_FOUND                      -1020    /**<  No matched file was found */
#define EMAIL_ERROR_DATA_NOT_FOUND                      -1021    /**<  No matched data was found */
#define EMAIL_ERROR_TASK_BINDER_NOT_FOUND               -1023    /**<  No matched task binder was found */
#define EMAIL_ERROR_TASK_NOT_FOUND                      -1168    /**<  No matched task was found */
#define EMAIL_ERROR_HANDLE_NOT_FOUND                    -1169    /**<  No matched handle was found */
#define EMAIL_ERROR_ALARM_DATA_NOT_FOUND                -1933    /**<  No matched alarm data was found */

/* Error codes for specification for maximum data */
#define EMAIL_ERROR_NO_MORE_DATA                        -1022    /**<  No more data available */
#define EMAIL_ERROR_MAX_EXCEEDED                        -1024    /**<  Cannot handle more data */
#define EMAIL_ERROR_OUT_OF_MEMORY                       -1028    /**<  There is not enough memory */
#define EMAIL_ERROR_ACCOUNT_MAX_COUNT                   -1053    /**<  There is too many account */
#define EMAIL_ERROR_MAIL_MEMORY_FULL                    -1054    /**<  There is no more storage */
#define EMAIL_ERROR_DATA_TOO_LONG                       -1025    /**<  Data is too long */
#define EMAIL_ERROR_MAXIMUM_DEVICES_LIMIT_REACHED       -1530    /**<  EAS - Maximum devices limit reached */

/* Error codes for storage */
#define EMAIL_ERROR_DB_FAILURE                          -1029    /**<  Database operation failed */
#define EMAIL_ERROR_SECURED_STORAGE_FAILURE             -2100    /**<  Error from secured storage */
#define EMAIL_ERROR_GCONF_FAILURE                       -1058    /**<  The error occurred on accessing Gconf */
#define EMAIL_ERROR_FILE                                -1059    /**<  File related error */
#define EMAIL_ERROR_SERVER_STORAGE_FULL                 -2101    /**<  There is no more storage in server */

/* Error codes for network */
#define EMAIL_ERROR_SOCKET_FAILURE                      -1031    /**<  Socket operation failed */
#define EMAIL_ERROR_CONNECTION_FAILURE                  -1032    /**<  Network connection failed */
#define EMAIL_ERROR_CONNECTION_BROKEN                   -1033    /**<  Network connection was broken */
#define EMAIL_ERROR_NO_SUCH_HOST                        -1802    /**<  No such host was found */
#define EMAIL_ERROR_NETWORK_NOT_AVAILABLE               -1800    /**<  WIFI not available*/
#define EMAIL_ERROR_INVALID_STREAM                      -1068
#define EMAIL_ERROR_FLIGHT_MODE_ENABLE                  -1801    /**<  Flight mode enable : network not available */

/* Error codes for SSL/TLS */
#define EMAIL_ERROR_STARTLS                             -1401    /**<  "STARTLS" */
#define EMAIL_ERROR_TLS_NOT_SUPPORTED                   -1040    /**<  The server doesn't support TLS */
#define EMAIL_ERROR_TLS_SSL_FAILURE                     -1041    /**<  The agent failed TLS/SSL */
#define EMAIL_ERROR_CANNOT_NEGOTIATE_TLS                -1400    /**<  "Cannot negotiate TLS" */
#define EMAIL_ERROR_NO_RESPONSE                         -1036    /**<  There is no server response */

/* Error codes for authentication */
#define EMAIL_ERROR_AUTH_NOT_SUPPORTED                  -1038    /**<  The server does not support authentication */
#define EMAIL_ERROR_AUTHENTICATE                        -1039    /**<  The server failed to authenticate user */
#define EMAIL_ERROR_AUTH_REQUIRED                       -1069    /**<  SMTP Authentication needed */
#define EMAIL_ERROR_LOGIN_FAILURE                       -1035    /**<  Login failed */
#define EMAIL_ERROR_LOGIN_ALLOWED_EVERY_15_MINS         -1600    /**<  "login allowed only every 15 minutes" */
#define EMAIL_ERROR_TOO_MANY_LOGIN_FAILURE              -1601    /**<  "Too many login failure" */
#define EMAIL_ERROR_XOAUTH_BAD_REQUEST                  -1602    /**<  "{"status":"400"..." */
#define EMAIL_ERROR_XOAUTH_INVALID_UNAUTHORIZED         -1603    /**<  "{"status":"401"..." */
#define EMAIL_ERROR_XOAUTH_INVALID_GRANT                -1604    /**<  "error" : "invalid_grant" */


/* Error codes for functionality */
#define EMAIL_ERROR_NOT_IMPLEMENTED                     -1047    /**<  The function is not implemented yet */
#define EMAIL_ERROR_NOT_SUPPORTED                       TIZEN_ERROR_PERMISSION_DENIED /**<  The function is not supported */
#define EMAIL_ERROR_SERVER_NOT_SUPPORT_FUNCTION         -1823    /**<  The function is not supported in server */

/* Error codes for from system */
#define EMAIL_ERROR_NO_SIM_INSERTED                     -1205    /**<  The SIM card did not insert */
#define EMAIL_ERROR_BADGE_API_FAILED                    -3004    /**<  The badge API returned false */

/* Error codes for event handling */
#define EMAIL_ERROR_EVENT_QUEUE_FULL                    -1060    /**<  Event queue is full */
#define EMAIL_ERROR_EVENT_QUEUE_EMPTY                   -1061    /**<  Event queue is empty */
#define EMAIL_ERROR_SESSION_NOT_FOUND                   -1067    /**<  No matched session was found */
#define EMAIL_ERROR_CANNOT_STOP_THREAD                  -2000
#define EMAIL_ERROR_CANCELLED                           -1046    /**<  The job was canceled by user */
#define EMAIL_NO_AVAILABLE_TASK_SLOT                    -2656    /**<  There is no available task slot */

/* Error codes for IPC*/
#define EMAIL_ERROR_IPC_CRASH                           -1500    /**<  The IPC connection is broken */
#define EMAIL_ERROR_IPC_CONNECTION_FAILURE              -1501    /**<  The server(daemon) and client(app) did not connet */
#define EMAIL_ERROR_IPC_SOCKET_FAILURE                  -1502    /**<  The IPC socket failed read and write */
#define EMAIL_ERROR_IPC_PROTOCOL_FAILURE                -1503    /**<  The IPC protocol failed */
#define EMAIL_ERROR_IPC_ALREADY_INITIALIZED             -1504    /**<  The IPC already is initialized */
#define EMAIL_ERROR_ACTIVE_SYNC_NOTI_FAILURE            -1300    /**<  The active sync noti failed */

/* Error codes for error from server */
#define EMAIL_ERROR_COMMAND_NOT_SUPPORTED               -1043    /**< The server does not support this command */
#define EMAIL_ERROR_ANNONYM_NOT_SUPPORTED               -1044    /**< The server does not support anonymous user */
#define EMAIL_ERROR_SCAN_NOT_SUPPORTED                  -1037    /**< The server does not support 'scan mailbox' */

#define EMAIL_ERROR_SMTP_SEND_FAILURE                   -1063    /**< SMTP send failed */
#define EMAIL_ERROR_SMTP_SEND_FAILURE_BY_OVERSIZE       -1073    /**< SMTP send failed by too large mail size*/

#define EMAIL_ERROR_POP3_DELE_FAILURE                   -1100    /**< Failed to run the command 'Dele' on POP server */
#define EMAIL_ERROR_POP3_UIDL_FAILURE                   -1101    /**< Failed to run the command 'Uidl' on POP server */
#define EMAIL_ERROR_POP3_LIST_FAILURE                   -1102    /**< Failed to run the command 'List' on POP server */
#define EMAIL_ERROR_POP3_NOOP_FAILURE                   -1103    /**< Failed to run the command 'NOOP' on POP server */

#define EMAIL_ERROR_IMAP4_APPEND_FAILURE                -1042    /**< Failed to run the command 'Append' on IMAP server */
#define EMAIL_ERROR_IMAP4_STORE_FAILURE                 -1200    /**< Failed to run the command 'Store' on IMAP server */
#define EMAIL_ERROR_IMAP4_EXPUNGE_FAILURE               -1201    /**< Failed to run the command 'Expunge' on IMAP server */
#define EMAIL_ERROR_IMAP4_FETCH_UID_FAILURE             -1202    /**< Failed to run the command 'Fetch UID' on IMAP server */
#define EMAIL_ERROR_IMAP4_FETCH_SIZE_FAILURE            -1203    /**< Failed to run the command 'Fetch SIZE' on IMAP server */
#define EMAIL_ERROR_IMAP4_IDLE_FAILURE                  -1204    /**< Failed to run the command 'Idle' on IMAP server */
#define EMAIL_ERROR_IMAP4_CREATE_FAILURE                -1210    /**< Failed to run the command 'Create' on IMAP server */
#define EMAIL_ERROR_IMAP4_DELETE_FAILURE                -1211    /**< Failed to run the command 'Delete' on IMAP server */
#define EMAIL_ERROR_IMAP4_RENAME_FAILURE                -1212    /**< Failed to run the command 'Rename' on IMAP server */
#define EMAIL_ERROR_IMAP4_COPY_FAILURE                  -1213    /**< Failed to run the command 'Copy' on IMAP server */
#define EMAIL_ERROR_IMAP4_NOOP_FAILURE                  -1214    /**< Failed to run the command 'NOOP' on IMAP server */

#define EMAIL_ERROR_INVALID_ATTACHMENT_SAVE_NAME        -1301    /**< Invalid attachment save name */

#define EMAIL_ERROR_DOMAIN_LOOKUP_FAILED                -1423    /**< The domain name provided was not found */

/* Error codes for certificate */
#define EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE            -3001    /**<  Cannot load the certificate */
#define EMAIL_ERROR_INVALID_CERTIFICATE                 -3002    /**<  Invalid certificate */
#define EMAIL_ERROR_DECRYPT_FAILED                      -3003    /**<  Cannot decipher the encrypt mail */
#define EMAIL_ERROR_CERTIFICATE_FAILURE                 -1045    /**<  Certificate failure - Invalid server certificate */

/* Error codes for account */
#define EMAIL_ERROR_ACCOUNT_IS_QUARANTINED              -5001    /**< Account is quarantined */
#define EMAIL_ERROR_ACCOUNT_IS_BLOCKED                  -5002    /**< Account is blocked */
#define EMAIL_ERROR_ACCOUNT_SYNC_IS_DISALBED            -5003    /**< Account sync is disabled */

/* Error codes for mails */
#define EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER            -1055    /**<  The expected mail is not found in server */
#define EMAIL_ERROR_MAIL_IS_NOT_DOWNLOADED              -1095    /**<  The mail is not downloaded */
#define EMAIL_ERROR_MAIL_IS_ALREADY_DOWNLOADED          -1456    /**<  The mail is already downloaded */

/* Error codes for attachment */
#define EMAIL_ERROR_ATTACHMENT_SIZE_EXCEED_POLICY_LIMIT -7001    /**<  The attachment size is exceeded the policy value */

/* Error codes for Other Module */
#define EMAIL_ERROR_MDM_SERVICE_FAILURE                 -7100    /**<  The MDM service did not work */
#define EMAIL_ERROR_MDM_RESTRICTED_MODE                 -7101    /**<  The MDM service is in restricted mode */
#define EMAIL_ERROR_NOTI                                -7110    /**<  The Notification API returned the error */
#define EMAIL_ERROR_DPM_RESTRICTED_MODE			-7201	 /**<  The DPM service is in restricted mode */

/* Etc */
#define EMAIL_ERROR_ALREADY_INITIALIZED                 -7321    /**<  The thread is already intialized */
#define EMAIL_ERROR_NOT_INITIALIZED                     -7322    /**<  The thread is not intialized */
#define EMAIL_ERROR_UNKNOWN                             -8000    /**<  Unknown error */

/* Should be replaced with proper name */
#define EMAIL_ERROR_LOAD_ENGINE_FAILURE                 -1056    /**<  Loading engine failed */
#define EMAIL_ERROR_CLOSE_FAILURE                       -1057    /**<  Engine is still used */
#define EMAIL_ERROR_NULL_VALUE                          -1302    /**<  The value is null */
#define EMAIL_ERROR_EMPTY_FILE                          -1304    /**<  The file did not exist */

/* Should be classified */
#define EMAIL_ERROR_SYSTEM_FAILURE                      -1050    /**<  There is a system error */
#define EMAIL_ERROR_ON_PARSING                          -1700    /**<  MIME parsering is failed */
#define EMAIL_ERROR_ALREADY_EXISTS                      -1322    /**<  Data duplicated */
#define EMAIL_ERROR_INPROPER_RESPONSE_FROM_MSG_SERVICE  -1323    /**<  Returned the error from msg server */

/* smack */
#define EMAIL_ERROR_NO_SMACK_RULE                       -1710    /**<  No smack rule exist for file access */
#define EMAIL_ERROR_PERMISSION_DENIED                   TIZEN_ERROR_PERMISSION_DENIED   /**<  Failed to check privilege */

/* Not used */
#define EMAIL_ERROR_INVALID_USER                        -1003    /**<  Invalid user ID was given. - Invalid user or password */
#define EMAIL_ERROR_INVALID_PASSWORD                    -1004    /**<  Invalid password was given. - Invalid user or password */
#define EMAIL_ERROR_INVALID_PATH                        -1011    /**<  Invalid flle path was given */
#define EMAIL_ERROR_MAIL_MAX_COUNT                      -1052    /**<  The mailbox is full */
#define EMAIL_ERROR_DATA_TOO_SMALL                      -1026    /**<  Data is too small */
#define EMAIL_ERROR_PROFILE_FAILURE                     -1030    /**<  No proper profile was found */
#define EMAIL_ERROR_NO_MMC_INSERTED                     -1209    /**<  The MMC card did not exist */
#define EMAIL_ERROR_VALIDATE_ACCOUNT                    -1208    /**<  Account validation failed */
#define EMAIL_ERROR_VALIDATE_ACCOUNT_OF_SMTP            -1215    /**<  Account validation of SMTP failed */
#define EMAIL_ERROR_NETWORK_TOO_BUSY                    -1027    /**<  Network is busy */
#define EMAIL_ERROR_DISCONNECTED                        -1034    /**<  Connection was disconnected */
#define EMAIL_ERROR_MAIL_LOCKED                         -1049    /**<  The mail was locked */
#define EMAIL_ERROR_RETRIEVE_HEADER_DATA_FAILURE        -1065    /**<  Retrieving header failed */
#define EMAIL_ERROR_MAILBOX_OPEN_FAILURE                -1064    /**<  Accessing mailbox failed */
#define EMAIL_ERROR_XML_PARSER_FAILURE                  -1066    /**<  XML parsing failed */
#define EMAIL_ERROR_FAILED_BY_SECURITY_POLICY           -1303    /**<  Failed by security policy */
#define EMAIL_ERROR_FLIGHT_MODE                         -1206    /**<  The network is flight mode */

/* Container : KNOX */
#define EMAIL_ERROR_CONTAINER_CREATE_FAILED             -2001    /**<  Failed to create the container (Since 2.4) */
#define EMAIL_ERROR_CONTAINER_GET_DOMAIN                -2002    /**<  Failed to get the domain information (Since 2.4) */
#define EMAIL_ERROR_CONTAINER_NOT_FOUND                 -2003    /**<  Failed to found the container (Since 2.4) */
#define EMAIL_ERROR_CONTAINER_SET_LINK                  -2004    /**<  Failed to set shared socket link (Since 2.4) */
#define EMAIL_ERROR_CONTAINER_ITERATE_DOMAIN            -2005    /**<  Failed to set interate domain (Since 2.4) */
#define EMAIL_ERROR_CONTAINER_NOT_INITIALIZATION        -2006    /**<  Not created the container yet (Since 2.4) */
#define EMAIL_ERROR_CONTAINER_JOIN_ZONE_FAILED          -2007    /**<  Failed to join zone in the container (Since 2.4) */
#define EMAIL_ERROR_CONTAINER_LOOKUP_ZONE_FAILED        -2008    /**<  Failed to lookup the zone (Since 2.4) */
 /**
 * @}
 */
#endif /* __EMAIL_ERRORS_H__ */
