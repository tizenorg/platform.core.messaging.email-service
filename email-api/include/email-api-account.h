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
* @defgroup EMAIL_SERVICE Email Service
* @{
*/


/**
* @ingroup EMAIL_SERVICE
* @defgroup EMAIL_API_ACCOUNT Email Account API
* @{
*/

/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-account.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 		This file contains the data structures and interfaces of Accounts provided by
 *			email-service .
 *
* @{

* @code
*	#include "email-api-account.h"
*	 bool
*      	other_app_invoke_uniform_api_sample(int* error_code)
*	{
*	email_account_t *account = NULL;
*	email_account_t *new_account = NULL;
*
*	account = malloc(sizeof(email_account_t));
*	memset(account, 0x00, sizeof(email_account_t));
*
*	account->retrieval_mode         = 1;
*	account->incoming_server_secure_connection           = 1;
*	account->outgoing_server_type    = EMAIL_SERVER_TYPE_SMTP;
*	account->outgoing_server_port_number       = EMAIL_SMTP_PORT;
*	account->outgoing_server_need_authentication           = 1;
*	account->account_name           = strdup("gmail");
*	account->display_name           = strdup("Tom");
*	account->user_email_address             = strdup("tom@gmail.com");
*	account->reply_to_addr          = strdup("tom@gmail.com");
*	account->return_addr            = strdup("tom@gmail.com");
*	account->incoming_server_type  = EMAIL_SERVER_TYPE_POP3;
*	account->incoming_server_address  = strdup("pop3.gmail.com");
*	account->incoming_server_port_number               = 995;
*	account->incoming_server_secure_connection           = 1;
*	account->retrieval_mode         = EMAIL_IMAP4_RETRIEVAL_MODE_ALL;
*	account->incoming_server_user_name              = strdup("tom");
*	account->password               = strdup("password");
*	account->outgoing_server_type    = EMAIL_SERVER_TYPE_SMTP;
*	account->outgoing_server_address    = strdup("smtp.gmail.com");
*	account->outgoing_server_port_number       = 587;
*	account->outgoing_server_secure_connection       = 0x02;
*	account->outgoing_server_need_authentication           = 1;
*	account->outgoing_server_user_name           = strdup("tom@gmail.com");
*	account->sending_password       = strdup("password");
*	account->pop_before_smtp        = 0;
*	account->incoming_server_requires_apop                   = 0;
*	account->flag1                  = 2;
*	account->flag2                  = 1;
*	account->is_preset_account         = 1;
*	account->logo_icon_path         = strdup("Logo Icon Path");
*	account->options.priority = 3;
*	account->options.keep_local_copy = 0;
*	account->options.req_delivery_receipt = 0;
*	account->options.req_read_receipt = 0;
*	account->options.download_limit = 0;
*	account->options.block_address = 0;
*	account->options.block_subject = 0;
*	account->options.display_name_from = strdup("Display name from");
*	account->options.reply_with_body = 0;
*	account->options.forward_with_files = 0;
*	account->options.add_myname_card = 0;
*    	account->options.add_signature = 0;
*	account->options.signature= strdup("Signature");
*	account->check_interval = 0;
*	// Add account
*	if(EMAIL_ERROR_NONE != email_add_account(account))
*		//failure
*	//else
*	{
*		//success
*		if(account_id)
*			*account_id = account->account_id;
*	}
*	if(EMAIL_ERROR_NONE != email_validate_account(account_id,&account_handle))
*		//failure
*	else
*	      //success
*	if(EMAIL_ERROR_NONE != email_delete_account(account_id))
*		//failure
*	else
*		//success
*	new_account = malloc(sizeof(email_account_t));
*	memset(new_account, 0x00, sizeof(email_account_t));
*	new_account->flag1                   = 1;
*	new_account->account_name            = strdup("samsung001");
*	new_account->display_name            = strdup("Tom001");
*	new_account->options.keep_local_copy = 1;
*	new_account->check_interval          = 55;
*	// Update account
*	if(EMAIL_ERROR_NONE != email_update_account(acount_id,new_account))
*		//failure
*	else
*		//success
*	// Get account
*	if(EMAIL_ERROR_NONE != email_get_account(account_id,GET_FULL_DATA,&account))
*		//failure
*	else
*		//success
*	// Get list of accounts
*	if(EMAIL_ERROR_NONE != email_get_account_list(&account_list,&count))
*		//failure
*	else
*		//success
*	// free account
*	email_free_account(&account, 1);
*	}
 *
 * @endcode
 * @}
 */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**

 * @fn email_add_account(email_account_t* account)
 * @brief	Create a new email account.This function is invoked when user wants to add new email account
 *
 * @param[in] account	Specifies the structure pointer of account.
 * @exception  none
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see         email_account_t
 * @remarks N/A
 */
EXPORT_API int email_add_account(email_account_t* account);

/**

 * @fn email_delete_account(int account_id)
 * @brief	 Delete a email account.This function is invoked when user wants to delete an existing email account
 *
 * @param[in] account_id	Specifies the account ID.
 * @exception  EMAIL_ERROR_INVALID_PARAM		-Invalid argument
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see
 * @remarks N/A
 */
EXPORT_API int email_delete_account(int account_id);

/**

 * @fn email_update_account(int account_id, email_account_t* new_account)
 * @brief	Change the information of a email account.This function is getting invoked when user wants to change some information of existing email account.
 *
 * @param[in] account_id	Specifies the orignal account ID.
 * @param[in] new_account	Specifies the information of new account.
 * @param[in] with_validation	If this is 1, email-service will validate the account before updating. If this is 0, email-service will update the account without validation.
 * @exception EMAIL_ERROR_INVALID_PARAM      	-Invalid argument
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see email_account_t
 * @remarks N/A
 */
EXPORT_API int email_update_account(int account_id, email_account_t* new_account);

/**

 * @fn email_update_account_with_validation(int account_id, email_account_t* new_account)
 * @brief	Change the information of a email account.This function is getting invoked when user wants to change some information of existing email account.
 *
 * @param[in] account_id	Specifies the orignal account ID.
 * @param[in] new_account	Specifies the information of new account.
 * @param[in] with_validation	If this is 1, email-service will validate the account before updating. If this is 0, email-service will update the account without validation.
 * @exception EMAIL_ERROR_INVALID_PARAM      	-Invalid argument
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see email_account_t
 * @remarks N/A
 */
EXPORT_API int email_update_account_with_validation(int account_id, email_account_t* new_account);

/**

 * @fn  email_get_account(int account_id, int pulloption, email_account_t** account)
 * @brief	Get an email account by ID. This function is getting invoked when user wants to get the account informantion based on account id and option (GET_FULL_DATA/WITHOUT_OPTION/ONLY_OPTION).<br>
 * 			Memory for account information will be allocated to 3rd param(account). The allocated memory should be freed by email_free_account().
 *
 * @param[in] account_id	Specifies the account ID.This function is invoked when user
 * @param[in] pulloption	Option to specify to get full details or partial, see definition of EMAIL_ACC_GET_OPT_XXX
 * @param[out] account		The returned account is saved here.
 * @exception EMAIL_ERROR_INVALID_PARAM     	-Invalid argument
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see email_account_t
 * @remarks N/A
 */

EXPORT_API int email_get_account(int account_id, int pulloption, email_account_t** account);

/**

 * @fn email_get_account_list(email_account_t** account_list, int* count);
 * @brief	Get Account List.This function is getting invoked when user wants to get all account information based on the count of accounts provided by user.<br>
 * 			Memory for account information will be allocated to 3rd param(account). The allocated memory should be freed by email_free_account().
 *
 * @param[in] account_list	Specifies the structure pointer of account.
 * @param[out] count			Specifies the count of accounts.
 * @exception EMAIL_ERROR_INVALID_PARAM     	-Invalid argument
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see email_account_t
 * @remarks N/A
 */
EXPORT_API int email_get_account_list(email_account_t** account_list, int* count);

 /**

 * @fn   email_free_account(email_account_t** account_list, int count);
 * @brief	Free allocated memory.This function is getting invoked when user wants to delete all account information.
 *
 * @param[in] account_list	Specifies the structure pointer of account.
 * @param[out] count			Specifies the count of accounts.
 * @exception EMAIL_ERROR_INVALID_PARAM     	-Invalid argument
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see email_account_t
 * @remarks N/A
 */
EXPORT_API int email_free_account(email_account_t** account_list, int count);


/**

 * @fn email_validate_account(int account_id, int *handle)
 * @brief	Validate account.This function is getting invoked  after adding one account to validate it.If account is not validated then user should retry once again to add the account .
 *
 * @param[in] account_id	       Specifies the account Id to validate.
 * @param[out] handle 		Specifies the sending handle.
 * @remarks N/A
 * @exception EMAIL_ERROR_INVALID_PARAM     	-Invalid argument
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @see		none
 * @remarks N/A
 */
EXPORT_API int email_validate_account(int account_id, int *handle) DEPRECATED; /* Will be replaced with email_validate_account_ex */

EXPORT_API int email_validate_account_ex(email_account_t* account, int *handle);

/**

 * @fn email_add_account_with_validation(email_account_t* account, int *handle)
 * @brief	Add an account when the account is validated. This function is getting invoked when user want to validate an account. If account is not validated then user should retry once again to add the account.<br>
 *              Validation is executed without saving an account to DB
 *
 * @param[in] account	    Specifies the structure pointer of account.
 * @param[out] handle 		Specifies the sending handle.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_add_account_with_validation(email_account_t* account, int *handle);


/**

 * @fn email_backup_accounts_into_secure_storage(const char *file_name)
 * @brief	Back up information of all accounts into secure storage.
 *          This function is getting invoked when user want to backup account information safely.
 *
 * @param[in] file_name	    Specifies the file name in secure storage
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_backup_accounts_into_secure_storage(const char *file_name);

/**

 * @fn email_restore_accounts_from_secure_storage(const char *file_name)
 * @brief	Restore accounts from stored file in secure storage.
 *          This function is getting invoked when user want to restore accounts.
 *
 * @param[in] file_name	    Specifies the file name in secure storage
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_restore_accounts_from_secure_storage(const char * file_name);

/**

 * @fn email_get_password_length_of_account(const int account_id, int *password_length)
 * @brief	Get password length of an account.
 *          This function is getting invoked when user want to know the length of an account.
 *
 * @param[in] account_id    Specifies the account id
 * @param[out] handle 		Specifies the password length.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_get_password_length_of_account(const int account_id, int *password_length);


/**

 * @fn email_query_server_info(const char* domain_name, email_server_info_t **result_server_info)
 * @brief	Query email server information.
 *          This function is getting invoked when user want to get email server information.
 *
 * @param[in] domain_name	        Specifies the domain name of server
 * @param[out] result_server_info 	Specifies the information of email server.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_query_server_info(const char* domain_name, email_server_info_t **result_server_info);

/**

 * @fn email_free_server_info(email_server_info_t **result_server_info)
 * @brief	Free email_server_info_t.
 *          This function is getting invoked when user want to free email_server_info_t.
 *
 * @param[in] result_server_info	Specifies the pointer of  in secure storage
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_free_server_info(email_server_info_t **result_server_info);

/**

 * @fn email_update_notification_bar(int account_id)
 * @brief	Update notifications on notification bar.
 *          This function is getting invoked when user want to update notification bar.
 *
 * @param[in] account_id	        Specifies the id of account.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_update_notification_bar(int account_id);

/**

 * @fn email_clear_all_notification_bar()
 * @brief	Clear all notification on notification bar.
 *          This function is getting invoked when user want to clear notification bar.
 *
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_clear_all_notification_bar();


/**

 * @fn email_save_default_account_id()
 * @brief	Save default account id to vconf storage.
 *          This function is getting invoked when user want to save default account id.
 *
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_save_default_account_id(int input_account_id);

/**

 * @fn email_load_default_account_id()
 * @brief	Load default account id to vconf storage.
 *          This function is getting invoked when user want to load default account id.
 *
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_load_default_account_id(int *output_account_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @} @}
*/

#endif /* __EMAIL_API_ACCOUNT_H__ */


