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

#ifndef __EMAIL_API_SMIME_H__
#define __EMAIL_API_SMIME_H__

#include "email-types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

EXPORT_API int email_add_certificate(char *certificate_path, char *email_address);

EXPORT_API int email_delete_certificate(char *email_address);

EXPORT_API int email_get_certificate(char *email_address, email_certificate_t **certificate);

EXPORT_API int email_get_decrypt_message(int mail_id, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count);

EXPORT_API int email_verify_signature(char *certificate_path, int mail_id, int *verify);

EXPORT_API int email_verify_certificate(char *certificate_path, int *verify);

EXPORT_API int email_get_resolve_recipients(int account_id, char *email_address, unsigned *handle);

EXPORT_API int email_validate_certificate(int account_id, char *email_address, unsigned *handle);

EXPORT_API int email_free_certificate(email_certificate_t **certificate, int count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EMAIL_API_SMIME_H__ */
