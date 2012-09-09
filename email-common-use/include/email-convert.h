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

#ifndef __EMAIL_CONVERT_H__
#define __EMAIL_CONVERT_H__

#include <time.h>
#include "email-internal-types.h"
#include "email-debug-log.h"
#include "email-storage.h"

/* Account */
INTERNAL_FUNC char* em_convert_account_to_byte_stream(email_account_t* account, int* stream_len);
INTERNAL_FUNC void  em_convert_byte_stream_to_account(char* stream,  int stream_len, email_account_t* account);
INTERNAL_FUNC int   em_convert_account_to_account_tbl(email_account_t *account, emstorage_account_tbl_t *account_tbl);
INTERNAL_FUNC int   em_convert_account_tbl_to_account(emstorage_account_tbl_t *account_tbl, email_account_t *account);


/* Mail */
INTERNAL_FUNC char* em_convert_mail_data_to_byte_stream(email_mail_data_t *mail_data, int input_mail_data_count, int *stream_size);
INTERNAL_FUNC void  em_convert_byte_stream_to_mail_data(char *stream, email_mail_data_t **mail_data, int *output_mail_data_count);

INTERNAL_FUNC int   em_convert_mail_tbl_to_mail_data(emstorage_mail_tbl_t *mail_table_data, int item_count, email_mail_data_t **mail_data, int *error);
INTERNAL_FUNC int   em_convert_mail_data_to_mail_tbl(email_mail_data_t *mail_data, int item_count, emstorage_mail_tbl_t **mail_table_data, int *error);

/* Attachment */
INTERNAL_FUNC char* em_convert_attachment_data_to_byte_stream(email_attachment_data_t *attachment_data, int attachment_count, int* stream_len);
INTERNAL_FUNC void  em_convert_byte_stream_to_attachment_data(char *stream, int stream_len, email_attachment_data_t **attachment_data, int *attachment_count);

/* Mailbox */
INTERNAL_FUNC int   em_convert_mailbox_to_mailbox_tbl(email_mailbox_t *mailbox, emstorage_mailbox_tbl_t *mailbox_tbl);
INTERNAL_FUNC int   em_convert_mailbox_tbl_to_mailbox(emstorage_mailbox_tbl_t *mailbox_tbl, email_mailbox_t *mailbox);


INTERNAL_FUNC char* em_convert_mailbox_to_byte_stream(email_mailbox_t* mailbox, int* stream_len);
INTERNAL_FUNC void  em_convert_byte_stream_to_mailbox(char* stream, int stream_len, email_mailbox_t* mailbox);

/* Rule */
INTERNAL_FUNC char* em_convert_rule_to_byte_stream(email_rule_t* rule, int* stream_len);
INTERNAL_FUNC void  em_convert_byte_stream_to_rule(char *stream, int stream_len, email_rule_t *rule);

/* Sending options */
INTERNAL_FUNC char* em_convert_option_to_byte_stream(email_option_t* option, int* stream_len);
INTERNAL_FUNC void em_convert_byte_stream_to_option(char *stream, int stream_len, email_option_t *option);

/* time_t */
INTERNAL_FUNC int   em_convert_string_to_time_t(char *input_datetime_string, time_t *output_time);
INTERNAL_FUNC int   em_convert_time_t_to_string(time_t *input_time, char **output_datetime_string);

/* email_extra_flag_t */
INTERNAL_FUNC char* em_convert_extra_flags_to_byte_stream(email_extra_flag_t new_flag, int* StreamSize);
INTERNAL_FUNC void  em_convert_byte_stream_to_extra_flags(char* pStream, email_extra_flag_t* new_flag);

/* email_meeting_request_t */
INTERNAL_FUNC char* em_convert_meeting_req_to_byte_stream(email_meeting_request_t* meeting_req, int* nStreamSize);
INTERNAL_FUNC void  em_convert_byte_stream_to_meeting_req(char* stream, int stream_len, email_meeting_request_t* meeting_req);

INTERNAL_FUNC int   em_convert_mail_flag_to_int(email_mail_flag_t flag, int* i_flag, int* err_code);
INTERNAL_FUNC int   em_convert_mail_int_to_flag(int i_flag, email_mail_flag_t* flag, int* err_code);
INTERNAL_FUNC int   em_convert_mail_status_to_mail_tbl(int mail_status, emstorage_mail_tbl_t *result_mail_tbl_data, int* err_code);
INTERNAL_FUNC int   em_convert_mail_tbl_to_mail_status(emstorage_mail_tbl_t *mail_tbl_data, int *result_mail_status, int* err_code);
INTERNAL_FUNC int   em_convert_mail_tbl_to_mail_flag(emstorage_mail_tbl_t *mail_tbl_data, email_mail_flag_t *result_flag, int* err_code);
INTERNAL_FUNC int   em_convert_mail_flag_to_mail_tbl(email_mail_flag_t *flag, emstorage_mail_tbl_t *result_mail_tbl_data,  int* err_code);

/* Search filter options */
INTERNAL_FUNC char* em_convert_search_filter_to_byte_stream(email_search_filter_t *input_search_filter_list, int input_search_filter_count, int *output_stream_size);
INTERNAL_FUNC void  em_convert_byte_stream_to_search_filter(char *input_stream, email_search_filter_t **output_search_filter_list, int *output_search_filter_count);

/* convert certificate */
INTERNAL_FUNC int em_convert_certificate_tbl_to_certificate(emstorage_certificate_tbl_t *certificate_tbl, email_certificate_t **certificate, int *error);

INTERNAL_FUNC int em_convert_certificate_to_certificate_tbl(email_certificate_t *certificate, emstorage_certificate_tbl_t *certificate_tbl);

#endif /* __EMAIL_CONVERT_H__ */
