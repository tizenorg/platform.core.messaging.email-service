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



/******************************************************************************
 * File :  email-core-smtp.h
 * Desc :  Mail SMTP Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EMAIL_CORE_SMTP_H__
#define __EMAIL_CORE_SMTP_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <stdio.h>
#include "c-client.h"
#include "email-internal-types.h"

INTERNAL_FUNC int emcore_send_mail(int mail_id, int *err_code);

INTERNAL_FUNC int emcore_send_saved_mail(int account_id, char *mailbox, int *err_code);

INTERNAL_FUNC int emcore_send_mail_with_downloading_attachment_of_original_mail(int input_mail_id);

INTERNAL_FUNC int emcore_schedule_sending_mail(int input_mail_id, time_t input_time_to_send);

INTERNAL_FUNC int emcore_make_rfc822_file_from_mail(emstorage_mail_tbl_t *input_mail_tbl_data, emstorage_attachment_tbl_t *input_attachment_tbl_t, int input_attachment_count, ENVELOPE **env, char **file_path, email_option_t *sending_option, int *err_code);

INTERNAL_FUNC int emcore_make_rfc822_file(email_mail_data_t *input_mail_tbl_data, email_attachment_data_t *input_attachment_tbl, int input_attachment_count, char **file_path, int *err_code);

INTERNAL_FUNC int emcore_add_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t *input_meeting_request, int input_from_eas);

INTERNAL_FUNC int emcore_add_read_receipt(int input_read_mail_id, int *output_receipt_mail_id);

INTERNAL_FUNC int emcore_add_meeting_request(int account_id, int input_mailbox_id, email_meeting_request_t *meeting_req, int *err_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
