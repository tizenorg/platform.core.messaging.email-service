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


#ifndef __EMAIL_ERRORS_H__
#define __EMAIL_ERRORS_H__

/*****************************************************************************/
/*  Errors                                                                   */
/*****************************************************************************/


#define EMF_ERROR_NONE                                 1       /*  There is no error */
#define EMF_ERROR_INVALID_PARAM                       -1001    /*  invalid parameter was given. - Invalid input parameter */
#define EMF_ERROR_INVALID_ACCOUNT                     -1002    /*  invalid account information was given. - Unsupported account */
#define EMF_ERROR_INVALID_USER                        -1003    /*  invalid user ID was given. - Invalid user or password */
#define EMF_ERROR_INVALID_PASSWORD                    -1004    /*  invalid password was given. - Invalid user or password */
#define EMF_ERROR_INVALID_SERVER                      -1005    /*  invalid server information was given. - Server unavailable */
#define EMF_ERROR_INVALID_MAIL                        -1006    /*  invalid mail information was given */
#define EMF_ERROR_INVALID_ADDRESS                     -1007    /*  invalid address information was given. - Incorrect address */
#define EMF_ERROR_INVALID_ATTACHMENT                  -1008    /*  invalid attachment information was given */
#define EMF_ERROR_INVALID_MAILBOX                     -1009    /*  invalid mailbox information was given */
#define EMF_ERROR_INVALID_FILTER                      -1010    /*  invalid filter information was given */
#define EMF_ERROR_INVALID_PATH                        -1011    /*  invalid flle path was given */
#define EMF_ERROR_INVALID_DATA                        -1012    /*  invalid data */
#define EMF_ERROR_INVALID_RESPONSE                    -1013    /*  unexpected network response was given. - Invalid server response */
#define EMF_ERROR_ACCOUNT_NOT_FOUND                   -1014    /*  no matched account was found */
#define EMF_ERROR_MAIL_NOT_FOUND                      -1015    /*  no matched mail was found */
#define EMF_ERROR_MAILBOX_NOT_FOUND                   -1016    /*  no matched mailbox was found */
#define EMF_ERROR_ATTACHMENT_NOT_FOUND                -1017    /*  no matched attachment was found */
#define EMF_ERROR_FILTER_NOT_FOUND                    -1018    /*  no matched filter was found */
#define EMF_ERROR_CONTACT_NOT_FOUND                   -1019    /*  no matched contact was found */
#define EMF_ERROR_FILE_NOT_FOUND                      -1020    /*  no matched file was found */
#define EMF_ERROR_DATA_NOT_FOUND                      -1021    /*  no matched data was found */
#define EMF_ERROR_NO_MORE_DATA                        -1022    /*  No more data available */
#define EMF_ERROR_ALREADY_EXISTS                      -1023    /*  data duplicated */
#define EMF_ERROR_MAX_EXCEEDED                        -1024    /*  Can't handle more data */
#define EMF_ERROR_DATA_TOO_LONG                       -1025    /*  Data is too long */
#define EMF_ERROR_DATA_TOO_SMALL                      -1026    /*  Data is too small */
#define EMF_ERROR_NETWORK_TOO_BUSY                    -1027    /*  Network is busy */
#define EMF_ERROR_OUT_OF_MEMORY                       -1028    /*  There is not enough memory */
#define EMF_ERROR_DB_FAILURE                          -1029    /*  database operation failed */
#define EMF_ERROR_PROFILE_FAILURE                     -1030    /*  no proper profile was found */
#define EMF_ERROR_SOCKET_FAILURE                      -1031    /*  socket operation failed */
#define EMF_ERROR_CONNECTION_FAILURE                  -1032    /*  network connection failed */
#define EMF_ERROR_CONNECTION_BROKEN                   -1033    /*  network connection was broken */
#define EMF_ERROR_DISCONNECTED                        -1034    /*  connection was disconnected */
#define EMF_ERROR_LOGIN_FAILURE                       -1035    /*  login failed */
#define EMF_ERROR_NO_RESPONSE                         -1036    /*  There is no server response */
#define EMF_ERROR_MAILBOX_FAILURE                     -1037    /*  The agent failed to scan mailboxes in server */
#define EMF_ERROR_AUTH_NOT_SUPPORTED                  -1038    /*  The server doesn't support authentication */
#define EMF_ERROR_AUTHENTICATE                        -1039    /*  The server failed to authenticate user */
#define EMF_ERROR_TLS_NOT_SUPPORTED                   -1040    /*  The server doesn't support TLS */
#define EMF_ERROR_TLS_SSL_FAILURE                     -1041    /*  The agent failed TLS/SSL */
#define EMF_ERROR_APPEND_FAILURE                      -1042    /*  The agent failed to append mail to server */
#define EMF_ERROR_COMMAND_NOT_SUPPORTED               -1043    /*  The server doesn't support this command */
#define EMF_ERROR_ANNONYM_NOT_SUPPORTED               -1044    /*  The server doesn't support anonymous user */
#define EMF_ERROR_CERTIFICATE_FAILURE                 -1045    /*  certificate failure - Invalid server certificate */
#define EMF_ERROR_CANCELLED                           -1046    /*  The job was canceled by user */
#define EMF_ERROR_NOT_IMPLEMENTED                     -1047    /*  The function was not implemented */
#define EMF_ERROR_NOT_SUPPORTED                       -1048    /*  The function is not supported */
#define EMF_ERROR_MAIL_LOCKED                         -1049    /*  The mail was locked */
#define EMF_ERROR_SYSTEM_FAILURE                      -1050    /*  There is a system error */
#define EMF_ERROR_MAIL_MAX_COUNT                      -1052    /*  The mailbox is full */
#define EMF_ERROR_ACCOUNT_MAX_COUNT                   -1053    /*  There is too many account */
#define EMF_ERROR_MAIL_MEMORY_FULL                    -1054    /*  There is no more storage */
#define EMF_ERROR_MAIL_NOT_FOUND_ON_SERVER            -1055    /*  The expected mail is not found in server */
#define EMF_ERROR_LOAD_ENGINE_FAILURE                 -1056    /*  loading engine failed */
#define EMF_ERROR_CLOSE_FAILURE                       -1057    /*  engine is still used */
#define EMF_ERROR_GCONF_FAILURE                       -1058    /*  The error occurred on accessing Gconf */
#define EMF_ERROR_NO_SUCH_HOST                        -1059    /*  no such host was found */
#define EMF_ERROR_EVENT_QUEUE_FULL                    -1060    /*  event queue is full */
#define EMF_ERROR_EVENT_QUEUE_EMPTY                   -1061    /*  event queue is empty */
#define EMF_ERROR_NO_RECIPIENT                        -1062    /*  no recipients information was found */
#define EMF_ERROR_SMTP_SEND_FAILURE                   -1063    /*  SMTP send failed */
#define EMF_ERROR_MAILBOX_OPEN_FAILURE                -1064    /*  accessing mailbox failed */
#define EMF_ERROR_RETRIEVE_HEADER_DATA_FAILURE        -1065    /*  retrieving header failed */
#define EMF_ERROR_XML_PARSER_FAILURE                  -1066    /*  XML parsing failed */
#define EMF_ERROR_SESSION_NOT_FOUND                   -1067    /*  no matched session was found */
#define EMF_ERROR_INVALID_STREAM                      -1068
#define EMF_ERROR_AUTH_REQUIRED                       -1069    /*  SMTP Authentication needed */
#define EMF_ERROR_POP3_DELE_FAILURE                   -1100
#define EMF_ERROR_POP3_UIDL_FAILURE                   -1101
#define EMF_ERROR_POP3_LIST_FAILURE                   -1102
#define EMF_ERROR_IMAP4_STORE_FAILURE                 -1200
#define EMF_ERROR_IMAP4_EXPUNGE_FAILURE               -1201
#define EMF_ERROR_IMAP4_FETCH_UID_FAILURE             -1202
#define EMF_ERROR_IMAP4_FETCH_SIZE_FAILURE            -1203
#define EMF_ERROR_IMAP4_IDLE_FAILURE                  -1204    /*  IDLE faile */
#define EMF_ERROR_NO_SIM_INSERTED                     -1205
#define EMF_ERROR_FLIGHT_MODE                         -1206
#define EMF_ERROR_VALIDATE_ACCOUNT                    -1208
#define EMF_ERROR_NO_MMC_INSERTED                     -1209
#define EMF_ERROR_ACTIVE_SYNC_NOTI_FAILURE            -1300
#define EMF_ERROR_HANDLE_NOT_FOUND                    -1301
#define EMF_ERROR_NULL_VALUE                          -1302
#define EMF_ERROR_FAILED_BY_SECURITY_POLICY           -1303
#define EMF_ERROR_CANNOT_NEGOTIATE_TLS                -1400    /*  "Cannot negotiate TLS" */
#define EMF_ERROR_STARTLS                             -1401    /*  "STARTLS" */
#define EMF_ERROR_IPC_CRASH                           -1500
#define EMF_ERROR_IPC_CONNECTION_FAILURE              -1501
#define EMF_ERROR_IPC_SOCKET_FAILURE                  -1502
#define EMF_ERROR_IPC_PROTOCOL_FAILURE                -1503
#define EMF_ERROR_LOGIN_ALLOWED_EVERY_15_MINS         -1600    /*  "login allowed only every 15 minutes" */
#define EMF_ERROR_TOO_MANY_LOGIN_FAILURE              -1601    /*  "Too many login failure" */
#define EMF_ERROR_ON_PARSING                          -1700
#define EMF_ERROR_NETWORK_NOT_AVAILABLE               -1800    /*  WIFI not availble*/
#define EMF_ERROR_CANNOT_STOP_THREAD                  -2000
#define EMF_ERROR_UNKNOWN                             -8000    /*  unknown error */

#endif /* __EMAIL_ERRORS_H__ */
