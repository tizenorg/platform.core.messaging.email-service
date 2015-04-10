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

/**
 * @file email-api-etc.h
 * @brief This file contains the data structures and interfaces of etc APIs provided by email-service.
 */

/**
 * @internal
 * @ingroup EMAIL_SERVICE_FRAMEWORK
 * @defgroup EMAIL_SERVICE_ETC_MODULE Other API
 * @brief Various API set for initializing and MIME operations and verifying email address.
 *
 * @section EMAIL_SERVICE_ETC_MODULE_HEADER Required Header
 *   \#include <email-api-etc.h>
 *   \#include <email-api-init.h>
 *
 * @section EMAIL_SERVICE_ETC_MODULE_OVERVIEW Overview
 */

/**
 * @internal
 * @addtogroup EMAIL_SERVICE_FRAMEWORK
 * @{
 */

#ifndef __EMAIL_API_ETC_H__
#define __EMAIL_API_ETC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "email-types.h"

/**
 * @brief Shows a user message.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  id          (need to be updated)
 * @param[in]  action      (need to be updated)
 * @param[in]  error_code  (need to be updated)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_action_t
 */
EXPORT_API int email_show_user_message(int id, email_action_t action, int error_code);

/**
 * @brief Parses a MIME file.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]   eml_file_path            (need to be updated)
 * @param[out]  output_mail_data         (need to be updated)
 * @param[out]  output_attachment_data   (need to be updated)
 * @param[out]  output_attachment_count  (need to be updated)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_parse_mime_file(char *eml_file_path, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data, int *output_attachment_count);

/**
 * @brief Creates a MIME file from input data.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]   input_mail_data         (need to be updated)
 * @param[in]   input_attachment_data   (need to be updated)
 * @param[in]   input_attachment_count  (need to be updated)
 * @param[out]  output_file_path        (need to be updated)
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_write_mime_file(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data, int input_attachment_count, char **output_file_path);

/**
 * @brief Deletes the parsed files of MIME.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  input_mail_data  (need to be updated)
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_delete_parsed_data(email_mail_data_t *input_mail_data);


/**
 * @brief Gets a MIME entity.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]   mime_path    (need to be updated)
 * @param[out]  mime_entity  (need to be updated)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_get_mime_entity(char *mime_path, char **mime_entity);

/**
 * @brief Validates email address.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  input_email_address  The email address string	(need to be updated)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_verify_email_address(char *input_email_address);

/**
 * @brief   Convert mutf7 string to utf8 string.
 *
 * @since_tizen 2.4
 *
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] mutf7_str     The original mutf7 string	(need to be updated)
 * @param[out] utf8_str     Thr utf8 string converted	(need to be updated)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_convert_mutf7_to_utf8(const char *mutf7_str, char **utf8_str);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EMAIL_API_ETC_H__ */

/**
 * @}
 */
