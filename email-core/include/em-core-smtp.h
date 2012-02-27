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



/******************************************************************************
 * File :  em-core-smtp.h
 * Desc :  Mail SMTP Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EM_CORE_SMTP_H__
#define __EM_CORE_SMTP_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* /#include "emflib.h" */
#include <stdio.h>
#include "c-client.h"

#if !defined(EXPORT_API)
#define EXPORT_API __attribute__((visibility("default")))
#endif


EXPORT_API int em_core_mail_send(int account_id, char *mailbox, int mail_id, emf_option_t *sending_option, int *err_code);

EXPORT_API int em_core_mail_send_saved(int account_id, char *mailbox, emf_option_t *sending_option, int *err_code);

EXPORT_API int em_core_mail_get_rfc822(emf_mail_t *mail, ENVELOPE **env, char **file_path, emf_option_t *sending_option, int *err_code);

EXPORT_API int em_core_mail_save(int account_id, char *mailbox_name, emf_mail_t *mail_src, emf_meeting_request_t *meeting_req, int from_composer, int *err_code);

EXPORT_API int em_core_add_mail(emf_mail_data_t *input_mail_data, emf_attachment_data_t *input_attachment_data_list, int input_attachment_count, emf_meeting_request_t *input_meeting_request, int input_sync_server);

EXPORT_API int em_core_mail_add_meeting_request(int account_id, char *mailbox_name, emf_meeting_request_t *meeting_req, int *err_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
