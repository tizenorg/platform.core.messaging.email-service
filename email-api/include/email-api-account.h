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


#ifndef __EMAIL_API_ACCOUNT_H__
#define __EMAIL_API_ACCOUNT_H__

#include "email-types.h"

/**
 * @ingroup EMAIL_SERVICE_FRAMEWORK
 * @defgroup EMAIL_SERVICE_ACCOUNT_MODULE Account API
 * @brief Account API is a set of operations to manage email accounts like add, update, delete or get account related details.
 *
 * @section EMAIL_SERVICE_ACCOUNT_MODULE_HEADER Required Header
 *   \#include <email-api.h>
 *
 * @section EMAIL_SERVICE_ACCOUNT_MODULE_OVERVIEW Overview
 * Account API is a set of operations to manage email accounts like add, update, delete or get account related details.
 */

/**
 * @file email-api-account.h
 * @brief This file contains the data structures and interfaces of Accounts provided by email-service.
 * @{
 * @code
 *   #include "email-api-account.h"
 *    bool
 *       other_app_invoke_uniform_api_sample(int* error_code)
 *   {
 *   email_account_t *account = NULL;
 *   email_account_t *new_account = NULL;
 *
 *   account = malloc(sizeof(email_account_t));
 *   memset(account, 0x00, sizeof(email_account_t));
 *
 *   account->retrieval_mode         = 1;
 *   account->incoming_server_secure_connection           = 1;
 *   account->outgoing_server_type    = EMAIL_SERVER_TYPE_SMTP;
 *   account->outgoing_server_port_number       = EMAIL_SMTP_PORT;
 *   account->outgoing_server_need_authentication           = 1;
 *   account->account_name           = strdup("gmail");
 *   account->display_name           = strdup("Tom");
 *   account->user_email_address             = strdup("tom@gmail.com");
 *   account->reply_to_addr          = strdup("tom@gmail.com");
 *   account->return_addr            = strdup("tom@gmail.com");
 *   account->incoming_server_type  = EMAIL_SERVER_TYPE_POP3;
 *   account->incoming_server_address  = strdup("pop3.gmail.com");
 *   account->incoming_server_port_number               = 995;
 *   account->incoming_server_secure_connection           = 1;
 *   account->retrieval_mode         = EMAIL_IMAP4_RETRIEVAL_MODE_ALL;
 *   account->incoming_server_user_name              = strdup("tom");
 *   account->password               = strdup("password");
 *   account->outgoing_server_type    = EMAIL_SERVER_TYPE_SMTP;
 *   account->outgoing_server_address    = strdup("smtp.gmail.com");
 *   account->outgoing_server_port_number       = 587;
 *   account->outgoing_server_secure_connection       = 0x02;
 *   account->outgoing_server_need_authentication           = 1;
 *   account->outgoing_server_user_name           = strdup("tom@gmail.com");
 *   account->sending_password       = strdup("password");
 *   account->auto_resend_times        = 0;
 *   account->pop_before_smtp        = 0;
 *   account->incoming_server_requires_apop                   = 0;
 *   account->incoming_server_authentication_method                   = 0;
 *   account->is_preset_account         = 1;
 *   account->logo_icon_path         = strdup("Logo Icon Path");
 *   account->options.priority = 3;
 *   account->options.keep_local_copy = 0;
 *   account->options.req_delivery_receipt = 0;
 *   account->options.req_read_receipt = 0;
 *   account->options.download_limit = 0;
 *   account->options.block_address = 0;
 *   account->options.block_subject = 0;
 *   account->options.display_name_from = strdup("Display name from");
 *   account->options.reply_with_body = 0;
 *   account->options.forward_with_files = 0;
 *   account->options.add_myname_card = 0;
 *       account->options.add_signature = 0;
 *   account->options.signature= strdup("Signature");
 *   account->check_interval = 0;
 *   // Add account
 *   if(EMAIL_ERROR_NONE != email_add_account(account))
 *       //failure
 *   //else
 *   {
 *       //success
 *       if(account_id)
 *           *account_id = account->account_id;
 *   }
 *   if(EMAIL_ERROR_NONE != email_validate_account(account_id,&account_handle))
 *       //failure
 *   else
 *         //success
 *   if(EMAIL_ERROR_NONE != email_delete_account(account_id))
 *       //failure
 *   else
 *       //success
 *   new_account = malloc(sizeof(email_account_t));
 *   memset(new_account, 0x00, sizeof(email_account_t));
 *   new_account->flag1                   = 1;
 *   new_account->account_name            = strdup("tizen001");
 *   new_account->display_name            = strdup("Tom001");
 *   new_account->options.keep_local_copy = 1;
 *   new_account->check_interval          = 55;
 *   // Update account
 *   if(EMAIL_ERROR_NONE != email_update_account(acount_id,new_account))
 *       //failure
 *   else
 *       //success
 *   // Get account
 *   if(EMAIL_ERROR_NONE != email_get_account(account_id,GET_FULL_DATA,&account))
 *       //failure
 *   else
 *       //success
 *   // Get list of accounts
 *   if(EMAIL_ERROR_NONE != email_get_account_list(&account_list,&count))
 *       //failure
 *   else
 *       //success
 *   // free account
 *   email_free_account(&account, 1);
 *   }
 *
 * @endcode
 * @}
 */

/**
 * @addtogroup EMAIL_SERVICE_ACCOUNT_MODULE
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Creates a new email account.
 * @details This function is invoked when the user wants to add a new email account.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 * @param[in] account  The structure pointer of an account
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_account_t
 */
EXPORT_API int email_add_account(email_account_t* account);

/**
 * @brief Deletes an email account.
 * @details This function is invoked when the user wants to delete an existing email account.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id  The account ID
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval  EMAIL_ERROR_INVALID_PARAM  Invalid argument
 */
EXPORT_API int email_delete_account(int account_id);

/**
 * @brief Changes the information of an email account.
 * @details This function is invoked when the user wants to change some information of the existing email account.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] account_id       The original account ID
 * @param[in] new_account      The information of new account
 * @param[in] with_validation  The validation flag \n
 *                             If this is @c 1, email-service will validate the account before updating. 
 *                             If this is @c 0, email-service will update the account without validation.
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval #EMAIL_ERROR_INVALID_PARAM  Invalid argument
 *
 * @see #email_account_t
 */
EXPORT_API int email_update_account(int account_id, email_account_t* new_account);

/**
 * @brief Changes the information of an email account.
 * @details This function is invoked when the user wants to change some information of the existing email account.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] account_id       The original account ID
 * @param[in] new_account      The information of new account
 * @param[in] with_validation  The validation tag \n
 *                             If this is @c 1, email-service will validate the account before updating. 
 *                             If this is @c 0, email-service will update the account without validation.
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval #EMAIL_ERROR_INVALID_PARAM  Invalid argument
 *
 * @see #email_account_t
 */
EXPORT_API int email_update_account_with_validation(int account_id, email_account_t* new_account);

/**
 * @brief Gets an email account by ID.
 * @details This function is invoked when the user wants to get the account information based on account ID and option (GET_FULL_DATA/WITHOUT_OPTION/ONLY_OPTION). 
 *          Memory for account information will be allocated to the 3rd param (@a account). 
 *          You must free the allocated memory using email_free_account().
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id  The account ID
 * @param[in]  pulloption  The option to specify to get full details or partial \n 
 *                         See definition of #EMAIL_ACC_GET_OPT_XXX.
 * @param[out] account     The returned account is saved here
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval #EMAIL_ERROR_INVALID_PARAM  Invalid argument
 *
 * @see #email_account_t
 */
EXPORT_API int email_get_account(int account_id, int pulloption, email_account_t** account);

/**
 * @brief Gets an account list.
 * @details This function is invoked when the user wants to get all account information based on the count of accounts provided by user. 
 *          Memory for account information will be allocated to 3rd param (@a account). 
 *          You must free the allocated memory using email_free_account().
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_list  The structure pointer of an account
 * @param[out] count         The count of accounts
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval #EMAIL_ERROR_INVALID_PARAM  Invalid argument
 *
 * @see #email_account_t
 */
EXPORT_API int email_get_account_list(email_account_t** account_list, int* count);

/**
 * @brief Frees allocated memory.
 * @details This function is invoked when the user wants to delete all account information.
 *
 * @since_tizen 2.3
 * @privlevel N/P
 *
 * @param[in]  account_list  The structure pointer of an account
 * @param[out] count         The count of accounts
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval #EMAIL_ERROR_INVALID_PARAM     Invalid argument
 *
 * @see #email_account_t
 */
EXPORT_API int email_free_account(email_account_t** account_list, int count);

EXPORT_API int email_validate_account(int account_id, int *handle) DEPRECATED; /* Will be replaced with email_validate_account_ex */

/**
 * @brief Validates an account.
 * @details This function is invoked after adding one account to validate it.
 *          If the account is not validated then t user should retry once again to add the account.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account  The account structure
 * @param[out] handle   The sending handle
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @retval #EMAIL_ERROR_INVALID_PARAM  Invalid argument
 *
 * @see #email_account_t
 */
EXPORT_API int email_validate_account_ex(email_account_t* account, int *handle);

/**
 * @brief Adds an account when the account is validated.
 * @details This function is invoked when a user wants to validate an account. 
 *          If the account is not validated then user should retry once again to add the account. 
 *          Validation is executed without saving an account to DB.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account  The structure pointer of an account
 * @param[out] handle   The sending handle
 *
 * @return  @c true on success,
 *          otherwise @c false on failure
 */
EXPORT_API int email_add_account_with_validation(email_account_t* account, int *handle);


/**
 * @brief Backs up information of all accounts into the secure storage.
 * @details This function is invoked when a user wants to backup account information safely.
 *
 * @param[in]  file_name  The file name in secure storage
 *
 * @return  @c true on success,
 *          otherwise @c false on failure
 */
EXPORT_API int email_backup_accounts_into_secure_storage(const char *file_name);

/**
 * @brief Restores accounts from a file stored in the secure storage.
 * @details This function is invoked when a user wants to restore accounts.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  file_name  The file name in the secure storage
 *
 * @return  @c true on success,
 *          otherwise @c false on failure
 */
EXPORT_API int email_restore_accounts_from_secure_storage(const char * file_name);

/**
 * @brief Gets the password length of an account.
 * @details This function is invoked when a user wants to know the length of an account.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id       The account ID
 * @param[in]  password_type    The password type
 * @param[out] password_length  The password length
 *
 * @return  @c true on success,
 *          otherwise @c false on failure
 */
EXPORT_API int email_get_password_length_of_account(int account_id, email_get_password_length_type password_type, int *password_length);

/**
 * @brief Updates notifications on the notification bar.
 * @details This function is invoked when user want to update notification bar.
 *
 * @param[in] account_id         The account ID
 * @param[in] total_mail_count   The total number of synced mail
 * @param[in] unread_mail_count  The unread number of synced mail
 * @param[in] input_from_eas     The flag that specifies whether the mail is from EAS
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_update_notification_bar(int account_id, int total_mail_count, int unread_mail_count, int input_from_eas);

/**
 * @brief Clears all notification on the notification bar.
 * @details This function is invoked when a user wants to clear notification bar.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_clear_all_notification_bar();

/**
 * @fn email_clear_notification_bar(int account_id)
 * @brief  Clear notification of account on notification bar.
 *         This function is getting invoked when user want to clear notification bar.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @remarks N/A
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_clear_notification_bar(int account_id);

/**
 * @brief Saves the default account ID to the vconf storage.
 * @details This function is invoked when a user wants to save a default account ID.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_save_default_account_id(int input_account_id);

/**
 * @brief Loads the default account ID to the vconf storage.
 * @details This function is invoked when a user wants to load a default account ID.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return  @c true on success,
 *          otherwise @c false on failure
 */
EXPORT_API int email_load_default_account_id(int *output_account_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* __EMAIL_API_ACCOUNT_H__ */


