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

#ifndef EM_CORE_GPG_H_
#define EM_CORE_GPG_H_

#include <cert-service.h>

#include "email-types.h"

INTERNAL_FUNC int emcore_pgp_set_signed_message(char *certificate, char *password, char *mime_entity, char *user_id, email_digest_type digest_type, char **file_path);

INTERNAL_FUNC int emcore_pgp_set_encrypted_message(char *recipient_list, char *certificate, char *password, char *mime_entity, char *user_id, email_digest_type digest_type, char **file_path);

INTERNAL_FUNC int emcore_pgp_set_signed_and_encrypted_message(char *recipient_list, char *certificate, char *password, char *mime_entity, char *user_id, email_digest_type digest_type, char **file_path);

INTERNAL_FUNC int emcore_pgp_get_verify_signature(char *signature_path, char *mime_entity, email_digest_type digest_type, int *verify);

INTERNAL_FUNC int emcore_pgp_get_decrypted_message(char *encrypted_message, char *password, int sign, char **decrypted_file, int *verify);

#endif /* EM_CORE_GPG_H_ */
