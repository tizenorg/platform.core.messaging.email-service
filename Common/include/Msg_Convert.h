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


#include "emflib.h"
#include <time.h>
#include "emf-dbglog.h"
#include "em-storage.h"

/* Account */
EXPORT_API char* em_convert_account_to_byte_stream(emf_account_t* account, int* nStreamSize);
EXPORT_API void  em_convert_byte_stream_to_account(char* pAccountStream,  emf_account_t* account);
EXPORT_API int   em_convert_account_to_account_tbl(emf_account_t *account, emf_mail_account_tbl_t *account_tbl);
EXPORT_API int   em_convert_account_tbl_to_account(emf_mail_account_tbl_t *account_tbl, emf_account_t *account);


/* Mail */
EXPORT_API char* em_convert_mail_to_byte_stream(emf_mail_t* mail, int* nStreamSize);
EXPORT_API void  em_convert_byte_stream_to_mail(char* pStream,  emf_mail_t* mail);

EXPORT_API char* em_convert_mail_head_to_byte_stream(emf_mail_head_t* mail_head, int* nStreamSize);
EXPORT_API void  em_convert_byte_stream_to_mail_head(char* pStream,  emf_mail_head_t* mail_head);

EXPORT_API char* em_convert_mail_body_to_byte_stream(emf_mail_body_t* mail_body, int* nStreamSize);
EXPORT_API void  em_convert_byte_stream_to_mail_body(char* pStream,  emf_mail_body_t* mail_body);

EXPORT_API char* em_convert_mail_info_to_byte_stream(emf_mail_info_t* mail_info, int* nStreamSize);
EXPORT_API void  em_convert_byte_stream_to_mail_info(char* pStream,  emf_mail_info_t* mail_info);

EXPORT_API char* em_convert_mail_data_to_byte_stream(emf_mail_data_t *input_mail_data, int intput_mail_data_count, int *output_stream_size);
EXPORT_API void em_convert_byte_stream_to_mail_data(char *intput_stream, emf_mail_data_t **output_mail_data, int *output_mail_data_count);

EXPORT_API int   em_convert_mail_tbl_to_mail(emf_mail_tbl_t *mail_table_data, emf_mail_t *mail_data);

EXPORT_API int   em_convert_mail_tbl_to_mail_data(emf_mail_tbl_t *mail_table_data, int item_count, emf_mail_data_t **mail_data, int *error);
EXPORT_API int   em_convert_mail_data_to_mail_tbl(emf_mail_data_t *mail_data, int item_count, emf_mail_tbl_t **mail_table_data, int *error);

/* Attachment */
EXPORT_API char* em_convert_attachment_data_to_byte_stream(emf_attachment_data_t *input_attachment_data, int intput_attachment_count, int* output_stream_size);
EXPORT_API void em_convert_byte_stream_to_attachment_data(char *intput_stream, emf_attachment_data_t **output_attachment_data, int *output_attachment_count);

EXPORT_API char* em_convert_attachment_info_to_byte_stream(emf_attachment_info_t* mail_atch, int* nStreamSize);
EXPORT_API void  em_convert_byte_stream_to_attachment_info(char* pStream,  int attachment_num, emf_attachment_info_t** mail_atch);


/* Mailbox */
EXPORT_API char* em_convert_mailbox_to_byte_stream(emf_mailbox_t* pMailbox, int* nStreamSize);
EXPORT_API void  em_convert_byte_stream_to_mailbox(char* pStream, emf_mailbox_t* pMailbox);


/* Rule */
EXPORT_API char* em_convert_rule_to_byte_stream(emf_rule_t* pRule, int* nStreamSize);
EXPORT_API void  em_convert_byte_stream_to_rule(char* pStream, emf_rule_t* pMailbox);	

/* Sending options */
EXPORT_API char* em_convert_option_to_byte_stream(emf_option_t* pOption, int* nStreamSize);
EXPORT_API void  em_convert_byte_stream_to_option(char* pStream, emf_option_t* pOption);

EXPORT_API int   em_convert_string_to_datetime(char *datetime_str, emf_datetime_t *datetime, int *err_code);
EXPORT_API int   em_convert_datetime_to_string(emf_datetime_t *input_datetime, char **output_datetime_str, int *err_code);


EXPORT_API char* em_convert_extra_flags_to_byte_stream(emf_extra_flag_t new_flag, int* StreamSize);
EXPORT_API void  em_convert_byte_stream_to_extra_flags(char* pStream, emf_extra_flag_t* new_flag);

EXPORT_API char* em_convert_meeting_req_to_byte_stream(emf_meeting_request_t* meeting_req, int* nStreamSize);
EXPORT_API void  em_convert_byte_stream_to_meeting_req(char* pStream,  emf_meeting_request_t* meeting_req);

EXPORT_API int   em_convert_mail_flag_to_int(emf_mail_flag_t flag, int* i_flag, int* err_code);
EXPORT_API int   em_convert_mail_int_to_flag(int i_flag, emf_mail_flag_t* flag, int* err_code);
EXPORT_API int   em_convert_mail_status_to_mail_tbl(int mail_status, emf_mail_tbl_t *result_mail_tbl_data, int* err_code);
EXPORT_API int   em_convert_mail_tbl_to_mail_status(emf_mail_tbl_t *mail_tbl_data, int *result_mail_status, int* err_code);
EXPORT_API int   em_convert_mail_tbl_to_mail_flag(emf_mail_tbl_t *mail_tbl_data, emf_mail_flag_t *result_flag, int* err_code);
EXPORT_API int   em_convert_mail_flag_to_mail_tbl(emf_mail_flag_t *flag, emf_mail_tbl_t *result_mail_tbl_data,  int* err_code);

