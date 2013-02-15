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
 * File :  email-core-cert.h
 * Desc :  Certificate operation Header
 *
 * Auth : 
 *
 * History : 
 *    2011.04.14  :  created
 *****************************************************************************/

#ifndef EM_CORE_CERT_H_
#define EM_CORE_CERT_H_

#include <openssl/x509.h>
#include <openssl/evp.h>

#include "email-utilities.h"
#include "email-types.h"

INTERNAL_FUNC int emcore_add_public_certificate(char *public_cert_path, char *save_name, int *err_code);

INTERNAL_FUNC int emcore_delete_public_certificate(char *email_address, int *err_code);

INTERNAL_FUNC int emcore_verify_signature(char *p7s_file_path, char *mime_entity, int *validity, int *err_code);

INTERNAL_FUNC int emcore_verify_certificate(char *certificate, int *validity, int *err_code);

INTERNAL_FUNC int emcore_free_certificate(email_certificate_t **certificate, int count, int *err_code);
/*
INTERNAL_FUNC int emcore_load_PFX_file(char *certificate, char *password, EVP_PKEY **pri_key, X509 **cert, STACK_OF(X509) **ca, int *err_code);
*/
INTERNAL_FUNC int emcore_load_PFX_file(char *certificate, EVP_PKEY **pri_key, X509 **cert, STACK_OF(X509) **ca, int *err_code);

#endif
