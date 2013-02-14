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


#ifndef __EMAIL_API_MAILBOX_H__
#define __EMAIL_API_MAILBOX_H__

#include "email-types.h"

/**
* @defgroup EMAIL_SERVICE Email Service
* @{
*/


/**
* @ingroup EMAIL_SERVICE
* @defgroup EMAIL_API_MAILBOX Email Mailbox API
* @{
*/

/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-mailbox.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 		This file contains the data structures and interfaces of mailbox provided by
 *			email-service .
 * @{
 * @code
 *
 *  #include "email_api_mailbox.h"
 *
 *  bool other_app_invoke_uniform_api_sample(int *error_code)
 *	{
 *		email_mailbox_t mailbox;
 *		email_mailbox_t *new_mailbox =NULL;
 *		email_mailbox_t *mailbox_list = NULL;
 *		int count = 0;
 *		int mailbox_type;
 *		int handle = 0;
 *		char *pMaiboxName;
 *		char *pParentMailbox;
 *
 *		memset(&mailbox,0x00,sizeof(email_mailbox_t));
 *		mailbox.mailbox_name = strdup("test");
 *		mailbox.alias = strdup("Personal");
 *		mailbox.account_id = 1;
 *		printf("Enter local_yn(1/0)");
 *		scanf("%d",&local_yn);
 *		mailbox.local=local_yn;
 *		mailbox.mailbox_type = 7;
 *
 *		//create new mailbox
 *
 *		if(EMAIL_ERR_NONE != email_add_mailbox(&mailbox,local_yn,&handle))
 *			printf("email_add_mailbox failed\n");
 *		else
 *			printf("email_add_mailbox success");
 *
 *		//delete mailbox
 *
 *		if(EMAIL_ERROR_NONE != email_delete_mailbox(mailbox,local_yn,&handle))
 *			printf("email_delete_mailbox failed\n");
 *		else
 *			printf("email_delete_mailbox success\n");
 *
 *		//free mailbox
 *		email_free_mailbox("new_mailbox,1");
 *
 *		//Get mailbox list
 *		if(EMAIL_ERROR_NONE != email_get_mailbox_list(account_id,local_yn,&mailbox_list,&count))
 *			//failure
 *		else
 *			//success
 *
 *		//Get mailbox by name
 *		pMailboxName = strdup("test");
 *		if(EMAIL_ERROR_NONE != email_get_mailbox_by_name(account_id,pMailboxName,&mailbox_list))
 *			//failure
 *		else
 *			//success
 *
 *		//Get child mailbox list
 *		pParentMailbox = strdup("test");
 *		if(EMAIL_ERROR_NONE != email_get_child_mailbox_list(account_id, paerent_mailbox,&mailbox_list,&count))
 *			//failure
 *		else
 *			//success
 *
 *		//Get mailbox by mailbox_type
 *		printf("Enter mailbox_type\n");
 *		scanf("%d",&mailbox_type);
 *		if(EMAIL_ERROR_NONE != email_get_mailbox_by_mailbox_type(account_id,mailbox_type,&mailbox_list))
 *			//failure
 *		else
 *			//success
 *
 *	}
 *
 * @endcode
 * @}
 */



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/**
 * @fn int email_add_mailbox(email_mailbox_t* new_mailbox, int on_server, int *handle)
 * @brief	Create a new mailbox or mailbox.This function is invoked when user wants to create a new mailbox for the specified account.
 * 		If On_server is true then it will create the mailbox on server as well as in local also.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code(refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] new_mailbox	Specifies the pointer of creating mailbox information.
*  @param[in] on_server		Specifies the creating mailbox on server.
 * @param[out] handle 		Specifies the sending handle.
 * @exception 	none
 * @see 	email_mailbox_t
  * @remarks N/A
 */
EXPORT_API int email_add_mailbox(email_mailbox_t *new_mailbox, int on_server, int *handle);

/**
 * @fn int email_rename_mailbox(int input_mailbox_id, char *input_mailbox_name, char *input_mailbox_alias)
 * @brief	Change mailbox name. This function is invoked when user wants to change the name of existing mail box.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code(refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_mailbox_id	Specifies the id of the mailbox.
 * @param[in] input_mailbox_name	Specifies the name of the mailbox.
 * @param[in] input_mailbox_alias	Specifies the alias of the mailbox.
 * @param[in] input_on_server	Specifies the moving mailbox on server.
 * @param[out] output_handle	Specifies the handle to manage tasks.
 *
 * @exception see email-errors.h
 * @see 	email_mailbox_t, email_mailbox_type_e
 * @remarks N/A
 */
EXPORT_API int email_rename_mailbox(int input_mailbox_id, char *input_mailbox_name, char *input_mailbox_alias, int input_on_server, int *output_handle);

/**
 * @fn int email_delete_mailbox(int input_mailbox_id, int input_on_server, int *output_handle)
 * @brief	Delete a mailbox or mailbox.This function deletes the existing mailbox for specified account based on the option on_server.
 * 		If the on_server is true then it deletes mailbox from server as well as locally.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code(refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_mailbox_id	Specifies the id of target mailbox .
 * @param[in] input_on_server	Specifies the deleting mailbox on server.
 * @param[out] output_handle 	Specifies the sending handle.
 * @exception 	see email-errors.h
 * @see 	email_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_delete_mailbox(int input_mailbox_id, int input_on_server, int *output_handle);

EXPORT_API int email_delete_mailbox_ex(int input_account_id, int *input_mailbox_id_array, int input_mailbox_id_count, int input_on_server, int *output_handle);

/**
 * @fn int email_set_mailbox_type(int input_mailbox_id, email_mailbox_type_e input_mailbox_type)
 * @brief	Change the mailbox type. This function is invoked when user wants to change the mailbox type.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code(refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_mailbox_id		Specifies the id of the mailbox.
 * @param[in] input_mailbox_type	Specifies the mailbox type.
 * @exception see email-errors.h
 * @see 	email_mailbox_type_e
 * @remarks N/A
 */
EXPORT_API int email_set_mailbox_type(int input_mailbox_id, email_mailbox_type_e input_mailbox_type);


/**
 * @fn int email_set_local_mailbox(int input_mailbox_id, int input_is_local_mailbox)
 * @brief	Change the attribute 'local' of email_mailbox_t. This function is invoked when user wants to change the attribute 'local'.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code(refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_mailbox_id			Specifies the id of the mailbox.
 * @param[in] input_is_local_mailbox	Specifies the value of the attribute 'local' of email_mailbox_t.
 * @exception see email-errors.h
 * @see		none
 * @remarks N/A
 */
EXPORT_API int email_set_local_mailbox(int input_mailbox_id, int input_is_local_mailbox);

/**
 * @fn email_get_mailbox_list(int account_id, int mailbox_sync_type, email_mailbox_t** mailbox_list, int* count)
 * @brief	Get all mailboxes from account.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code(refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] account_id		Specifies the account ID.
 * @param[in] mailbox_sync_type		Specifies the sync type.
 * @param[out] mailbox_list	Specifies the pointer of mailbox structure pointer.(possibly NULL)
 * @param[out] count			The mailbox count is saved here.(possibly 0)
 * @exception 		none
 * @see 	email_mailbox_t

 * @code
 *    	#include "email-api-mailbox.h"
 *   	bool
 *   	_api_sample_get_mailbox_list()
 *   	{
 *   		int account_id =0,count = 0;
 *   		int mailbox_sync_type;
 *   		int error_code = EMAIL_ERROR_NONE;
 *   		email_mailbox_t *mailbox_list=NULL;
 *
 *   		printf("\n > Enter account id: ");
 *   		scanf("%d", &account_id);
 *   		printf("\n > Enter mailbox_sync_type: ");
 *   		scanf("%d", &mailbox_sync_type);
 *
 *   		if((EMAIL_ERROR_NONE != email_get_mailbox_list(account_id, mailbox_sync_type, &mailbox_list, &count)))
 *   		{
 *   			printf(" Error\n");
 *   		}
 *   		else
 *   		{
 *			printf("Success\n");
 *			email_free_mailbox(&mailbox_list,count);
 *   		}
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_get_mailbox_list(int account_id, int mailbox_sync_type, email_mailbox_t** mailbox_list, int* count);

EXPORT_API int email_get_mailbox_list_ex(int account_id, int mailbox_sync_type, int with_count, email_mailbox_t** mailbox_list, int* count);

/**
 * @fn email_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type,  email_mailbox_t** mailbox)
 * @brief	Get mailbox by mailbox_type.This function is invoked when user wants to know the mailbox information by mailbox_type for the given account.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code(refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] account_id		Specifies the account ID.
 * @param[in] mailbox_type		Specifies the mailbox type.
 * @param[out] mailbox		Specifies the pointer of mailbox structure pointer.(possibly NULL)
 * @exception none
 * @see  	email_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type,  email_mailbox_t** mailbox);

/**
 * @fn email_get_mailbox_by_mailbox_id(int input_mailbox_id, email_mailbox_t** output_mailbox)
 * @brief	Get mailbox by mailbox_id. This function is invoked when user wants to know the mailbox information by mailbox id.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code(refer to EMAIL_ERROR_XXX) on failure.
 * @param[in]  input_mailbox_id	Specifies the mailbox id.
 * @param[out] output_mailbox	Specifies the pointer of mailbox structure pointer.(possibly NULL)
 * @exception none
 * @see  	email_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_get_mailbox_by_mailbox_id(int input_mailbox_id, email_mailbox_t** output_mailbox);

/**
 * @fn email_set_mail_slot_size(int input_account_id, int input_mailbox_id, int input_new_slot_size)
 * @brief	Set mail slot size.This function is invoked when user wants to set the size of mail slot.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code(refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_account_id		Specifies the account ID.
 * @param[in] input_mailbox_id		Specifies the mailbox id.
 * @param[in] input_new_slot_size	Specifies the mail slot size.
 * @exception none
 * @see  	email_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_set_mail_slot_size(int input_account_id, int input_mailbox_id, int input_new_slot_size);

/**
 * @fn email_stamp_sync_time_of_mailbox(int input_mailbox_id)
 * @brief	Stamp sync time of mailbox. This function is invoked when user wants to set the sync time of the mailbox.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code(refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_mailbox_id		Specifies the mailbox id.
 * @exception none
 * @see  	email_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_stamp_sync_time_of_mailbox(int input_mailbox_id);


/**
 * @fn email_free_mailbox(email_mailbox_t** mailbox_list, int count)
 * @brief	Free allocated memory for mailbox information.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mailbox_list	Specifies the pointer for searching mailbox structure pointer.
 * @param[in] count			Specifies the count of mailboxes.
 * @exception 		none
 * @see                 email_mailbox_t

 * @code
 *    	#include "email-api-mailbox.h"
 *   	bool
 *   	_api_sample_free_mailbox_info()
 *   	{
 *		email_mailbox_t *mailbox;
 *
 *		//fill the mailbox structure
 *		//count - number of mailbox structure user want to free
 *		 if(EMAIL_ERROR_NONE == email_free_mailbox(&mailbox,count))
 *		 	//success
 *		 else
 *		 	//failure
 *
 *   	}
 * @endcode
 * @remarks N/A
 */

EXPORT_API int email_free_mailbox(email_mailbox_t** mailbox_list, int count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @} @}
*/

#endif /* __EMAIL_API_MAILBOX_H__ */


