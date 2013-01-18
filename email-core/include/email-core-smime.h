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
 * File :  email-core-smime.h
 * Desc :  Mail Operation Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/

#ifndef EM_CORE_SMIME_H_
#define EM_CORE_SMIME_H_

#include <cert-service.h>

#include "email-types.h"

INTERNAL_FUNC int emcore_smime_set_signed_message(char *certificate, char *mime_entity, email_digest_type digest_type, char **file_path, int *err_code);

INTERNAL_FUNC int emcore_smime_set_encrypt_message(char *other_certificate_list, char *mime_entity, email_cipher_type cipher_type, char **file_path, int *err_code);

INTERNAL_FUNC int emcore_smime_set_signed_and_encrypt_message(char *recipient_list, char *certificate, char *mime_entity, email_cipher_type cipher_type, email_digest_type digest_type, char **file_path, int *err_code);

INTERNAL_FUNC int emcore_smime_verify_signed_message(char *signed_message, char *ca_file, char *ca_path, int *verify);

INTERNAL_FUNC int emcore_smime_set_decrypt_message(char *encrypt_message, char *from_address, char **decrypt_message, int *err_code);

INTERNAL_FUNC int emcore_convert_mail_data_to_smime_data(emstorage_account_tbl_t *account_tbl_item, email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data_list, int *output_attachment_count);


#endif /* EM_CORE_SMIME_H_ */
