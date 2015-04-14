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

 * @fn email_add_certificate(char *certificate_path, char *email_address)
 * @brief	 Store infomations of public certificate in database.
 *
 * @param[in] certificate_path  	File path of public certificate.
 * @param[in] email_address     	Keyword for searching the certificate information
 * @exception  none
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see
 * @remarks N/A
 */
EXPORT_API int email_add_certificate(char *certificate_path, char *email_address);

/**

 * @fn email_delete_certificate(char *email_address)
 * @brief	 Delete infomations of public certificate in database.
 *
 * @param[in] email_address     	Keyword for deleting the certificate information
 * @exception  none
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see
 * @remarks N/A
 */
EXPORT_API int email_delete_certificate(char *email_address);

/**

 * @fn email_get_certificate(char *email_address, email_certificate_t **certificate)
 * @brief	 Get infomations of public certificate in database.
 *
 * @param[in] email_address     	Keyword for geting the certificate information
 * @param[out] certificate              Specifies the certificate
 * @exception  none
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see
 * @remarks N/A
 */
EXPORT_API int email_get_certificate(char *email_address, email_certificate_t **certificate);

/**

 * @fn email_get_decrypt_message(int mail_id, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count);

 * @brief	 Get the decrypted message 
 * @param[in] mail_id			Specifies the mail_id
 * @param[out] output_mail_data		Specifies the mail_data 
 * @param[out] output_attachment_data	Specifies the mail_attachment_data
 * @param[out] output_attachment_count	Specifies the count of attachment
 * @exception  none
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see
 * @remarks N/A
 */
EXPORT_API int email_get_decrypt_message(int mail_id, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count);

/**

 * @fn email_verify_signature(int mail_id, int *verify);

 * @brief	 Verify the signed mail 
 * @param[in] mail_id		Specifies the mail_id
 * @param[out] verify		Specifies verify [false : failed verify, true : success the verification]
 * @exception  none
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see
 * @remarks N/A
 */
EXPORT_API int email_verify_signature(int mail_id, int *verify);

EXPORT_API int email_verify_signature_ex(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data, int input_attachment_count, int *verify);

/**

 * @fn email_verify_certificate(char *certificate_path, int *verify);

 * @brief	 Verify the certificate 
 * @param[in] certificate_path	Specifies the path of certificate
 * @param[out] verify		Specifies verify [false : failed verify, true : success the verification]
 * @exception  none
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see
 * @remarks N/A
 */
EXPORT_API int email_verify_certificate(char *certificate_path, int *verify);

/**

 * @fn email_get_resolve_recipients(int account_id, char *email_address, unsigned *handle);

 * @brief	 Get the certificate at the server [Using exchange server] 
 * @param[in] account_id	Specifies an account_id
 * @param[in] email_address	Specifies email address for getting certificate
 * @param[out] handle		Specifies the handle for stopping 
 * @exception  none
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see
 * @remarks N/A
 */
EXPORT_API int email_get_resolve_recipients(int account_id, char *email_address, unsigned *handle);

/**

 * @fn email_validate_certificate(int account_id, char *email_address, unsigned *handle);

 * @brief	 Verfiy the certificate to the server [Using exchange server] 
 * @param[in] account_id	Specifies an account_id
 * @param[in] email_address	Specifies email address for validating certificate
 * @param[out] handle		Specifies the handle for stopping 
 * @exception  none
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see
 * @remarks N/A
 */
EXPORT_API int email_validate_certificate(int account_id, char *email_address, unsigned *handle);

/**

 * @fn email_free_certificate(email_certificate_t **certificate, int count);

 * @brief	 Free the memory of certificate 
 * @param[in] certificate	Specifies the certificate
 * @param[in] count		Specifies the count of certificates
 * @exception  none
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see
 * @remarks N/A
 */
EXPORT_API int email_free_certificate(email_certificate_t **certificate, int count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EMAIL_API_SMIME_H__ */
