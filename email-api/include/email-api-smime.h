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

#ifndef __EMAIL_API_SMIME_H__
#define __EMAIL_API_SMIME_H__

#include "email-types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @file email-api-smime.h
 */

/**
 * @internal
 * @ingroup EMAIL_SERVICE_FRAMEWORK
 * @defgroup EMAIL_SERVICE_SMIME_MODULE SMIME API
 * @brief SMIME API is a set of operations to handle SMIME data for secured email.
 *
 * @section EMAIL_SERVICE_SMIME_MODULE_HEADER Required Header
 *   \#include <email-api-smime.h>
 *
 * @section EMAIL_SERVICE_SMIME_MODULE_OVERVIEW Overview
 * SMIME API is a set of operations to handle SMIME data for secured email.
 */

/**
 * @internal
 * @addtogroup EMAIL_SERVICE_SMIME_MODULE
 * @{
 */


/**
 * @brief Stores a public certificate information in the database.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] certificate_path  The file path of public certificate
 * @param[in] email_address     The keyword for searching the certificate information
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_add_certificate(char *certificate_path, char *email_address);

/**
 * @brief Deletes a public certificate information from the database.
 *
 * @param[in]  email_address  The keyword for deleting the certificate information
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_delete_certificate(char *email_address);

/**
 * @brief Gets the the public certificate information from the database.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  email_address  The keyword for getting the certificate information
 * @param[out] certificate    The certificate
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_get_certificate(char *email_address, email_certificate_t **certificate);

/**
 * @brief Gets a decrypted message.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  mail_id                  The mail ID
 * @param[out] output_mail_data         The mail data
 * @param[out] output_attachment_data   The mail attachment data
 * @param[out] output_attachment_count  The count of attachment
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_get_decrypt_message(int mail_id, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count);

/**
 * @brief Gets a decrypted message.
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 * @param[in] input_mail_data		Specifies the signed mail data
 * @param[in] input_attachment_data	Specifies the attachment of signed mail
 * @param[in] input_attachment_count	Specifies the attachment count of signed mail
 * @param[out] output_mail_data		Specifies the mail_data
 * @param[out] output_attachment_data	Specifies the mail_attachment_data
 * @param[out] output_attachment_count	Specifies the count of attachment
 * @return EMAIL_ERROR_NONE on success or an error code (refer to EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_get_decrypt_message_ex(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data, int input_attachment_count,
                                            email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count);
/**
 * @brief Verifies a signed mail.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] mail_id  The mail ID
 * @param[out] verify  The verification state \n
 *                     [false : failed verification, true : verification successful]
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_verify_signature(int mail_id, int *verify);

/**
* @brief Verifies a signed mail.
* @since_tizen 2.3
*
* @param[in]  input_mail_data         The signed mail data
* @param[in]  input_attachment_data   The attachment of signed mail
* @param[in]  input_attachment_count  The attachment count of signed mail
* @param[out] verify                  The verification status \n
*                                     false : failed verification, true : verification successful
*
* @return  #EMAIL_ERROR_NONE on success,
*          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
*/
EXPORT_API int email_verify_signature_ex(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data, int input_attachment_count, int *verify);

/**
 * @brief Verifies a certificate.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]   certificate_path  The path of the certificate
 * @param[out]  verify            The verification status \n
 *                                false : failed verification, true : verification successful
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_verify_certificate(char *certificate_path, int *verify);

/**
 * @brief Gets the certificate from the server (using exchange server).
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id     The account ID
 * @param[in]  email_address  The email address that gets a certificate
 * @param[out] handle         The handle for stopping
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_get_resolve_recipients(int account_id, char *email_address, unsigned *handle);

/**
 * @brief Verifies the certificate to the server (using exchange server).
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id     The account ID
 * @param[in]  email_address  The email address that validates a certificate
 * @param[out] handle         The handle for stopping
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_validate_certificate(int account_id, char *email_address, unsigned *handle);

/**
 * @brief Frees the memory of the certificate.
 *
 * @since_tizen 2.3
 * @privlevel N/P
 *
 * @param[in] certificate  The certificate
 * @param[in] count        The count of certificates
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_free_certificate(email_certificate_t **certificate, int count);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EMAIL_API_SMIME_H__ */
