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


#ifndef __EMF_MAPI_FOLDER_H__   /* mailbox */
#define __EMF_MAPI_FOLDER_H__

#include "emf-types.h"

/**
* @defgroup EMAIL_FRAMEWORK Email Service
* @{
*/


/**
* @ingroup EMAIL_FRAMEWORK
* @defgroup EMAIL_MAPI_FOLDER Email mailbox(Mailbox) API
* @{
*/

/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with Email Engine.
 * @file		Emf_Mapi_Folder.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 		This file contains the data structures and interfaces of mailbox [mailbox] provided by 
 *			Email Engine . 
 * @{
 * @code
 *
 *  	#include "emf_mapi_folder.h"
 *
 *  	bool 
 *  	other_app_invoke_uniform_api_sample(int *error_code)	
 *	{
 *		emf_mailbox_t mbox;
 *		emf_mailbox_t *new_mailbox =NULL;
 *		emf_mailbox_t *mailbox_list = NULL;
 *		int count = 0;
 *		int mailbox_type;
 *		unsigned handle = 0;
 *		char *pMaiboxName;
 *		char *pParentMailbox;
 *		
 *		memset(&mbox,0x00,sizeof(emf_mailbox_t));
 *		mbox.name = strdup("test");
 *		mbox.alias = strdup("Personal");
 *		mbox.account_id = 1;
 *		printf("Enter local_yn(1/0)");
 *		scanf("%d",&local_yn);
 *		mbox.local=local_yn;
 *		mbox.mailbox_type = 7;
 *		
 *		//create new mailbox
 *		
 *		if(EMF_ERR_NONE != email_add_mailbox(&mbox,local_yn,&handle))
 *			printf("email_add_mailbox failed\n");
 *		else
 *			printf("email_add_mailbox success");
 *			
 *		//update mailbox	
 *	       new_mailbox = malloc(sizeof(emf_mailbox_t));
 *	       memset(new_mailbox,0x00,sizeof(emf_mailbox_t));
 *
 *	       new_mailbox->name = strdup("PersonalUse");
 *
 *  	       if(EMF_ERROR_NONE != email_update_mailbox(&mbox,new_mailbox))
 *	       	 	printf("email_update_mailbox failed\n");
 *	       else
 *     			printf("email_update_mailbox success\n");
 *		//delete mailbox
 *
 *		if(EMF_ERROR_NONE != email_delete_mailbox(mbox,local_yn,&handle))
 *			printf("email_delete_mailbox failed\n");
 *		else
 *			printf("email_delete_mailbox success\n");
 *			
 *		//free mailbox
 *		email_free_mailbox("new_mailbox,1");
 *		
 *		//Get mailbox list
 *		if(EMF_ERROR_NONE != email_get_mailbox_list(account_id,local_yn,&mailbox_list,&count))
 *			//failure
 *		else
 *			//success
 *		
 *		//Get mailbox by name
 *		pMailboxName = strdup("test");
 *		if(EMF_ERROR_NONE != email_get_mailbox_by_name(account_id,pMailboxName,&mailbox_list))
 *			//failure
 *		else
 *			//success
 *
 *		//Get child mailbox list
 *		pParentMailbox = strdup("test");
 *		if(EMF_ERROR_NONE != email_get_child_mailbox_list(account_id, paerent_mailbox,&mailbox_list,&count))
 *			//failure
 *		else
 *			//success
 *
 *		//Get mailbox by mailbox_type
 *		printf("Enter mailbox_type\n");
 *		scanf("%d",&mailbox_type);
 *		if(EMF_ERROR_NONE != email_get_mailbox_by_mailbox_type(account_id,mailbox_type,&mailbox_list))
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
 
 * @open
 * @fn EXPORT_API int email_add_mailbox(emf_mailbox_t* new_mailbox, int on_server, unsigned* handle)
 * @brief	Create a new mailbox or mailbox.This function is invoked when user wants to create a new mailbox for the specified account.
 * 		If On_server is true then it will create the mailbox on server as well as in local also.   
 *
 * @return This function returns EMF_ERROR_NONE on success or error code(refer to EMF_ERROR_XXX) on failure.
 * @param[in] new_mailbox	Specifies the pointer of creating mailbox information.
*  @param[in] on_server		Specifies the creating mailbox information on server.
 * @param[out] handle 		Specifies the sending handle.
 * @exception 	none
 * @see 	emf_mailbox_t
  * @remarks N/A
 */
EXPORT_API int email_add_mailbox(emf_mailbox_t* new_mailbox, int on_server, unsigned* handle);


/**
 
 * @open
 * @fn EXPORT_API int email_delete_mailbox(emf_mailbox_t* mailbox, int on_server,  unsigned* handle)
 * @brief	Delete a mailbox or mailbox.This function deletes the existing mailbox for specified account based on the option on_server.
 * 		If the on_server is true then it deletes mailbox from server as well as locally.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code(refer to EMF_ERROR_XXX) on failure.
 * @param[in] mailbox	Specifies the pointer of deleting mailbox information.
 * @param[in] on_server		Specifies the creating mailbox information on server.
 * @param[out] handle 		Specifies the sending handle.
 * @exception 	#EMF_ERROR_INVALID_PARAM	-Invaid argument
 * @see 	emf_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_delete_mailbox(emf_mailbox_t* mailbox, int on_server,  unsigned* handle);


/**
 
 * @fn EXPORT_API int email_update_mailbox(emf_mailbox_t* old_mailbox, emf_mailbox_t* new_mailbox)
 * @brief	Change mailbox or mailbox information.This function is invoked when user wants to change the existing mail box information. 
 *			This supports ONLY updating mailbox_type in local db. This can be used to match a specific mail box and a specific mailbox_type.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code(refer to EMF_ERROR_XXX) on failure.
 * @param[in] old_mailbox	Specifies the information of previous mailbox. <br>mandatory field : account_id, name
 * @param[in] new_mailbox	Specifies the information of new mailbox. <br
 * @exception #EMF_ERROR_INVALID_PARAM 		-Invaid argument
 * @see 	emf_mailbox_t, emf_mailbox_type_e
  * @remarks N/A
 */
EXPORT_API int email_update_mailbox(emf_mailbox_t* old_mailbox, emf_mailbox_t* new_mailbox);

EXPORT_API int email_get_sync_mailbox_list(int account_id, emf_mailbox_t** mailbox_list, int* count) DEPRECATED;


/**
 
 * @open
 * @fn email_get_mailbox_list(int account_id, int mailbox_sync_type, emf_mailbox_t** mailbox_list, int* count)
 * @brief	Get all mailboxes from account.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code(refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id		Specifies the account ID.
 * @param[in] mailbox_sync_type		Specifies the sync type.
 * @param[out] mailbox_list	Specifies the pointer of mailbox structure pointer.(possibly NULL)
 * @param[out] count			The mailbox count is saved here.(possibly 0)
 * @exception 		none
 * @see 	emf_mailbox_t

 * @code
 *    	#include "Emf_Mapi_Message.h"
 *   	bool
 *   	_api_sample_get_mailbox_list()
 *   	{
 *   		int account_id =0,count = 0;
 *   		int mailbox_sync_type;
 *   		int error_code = EMF_ERROR_NONE;
 *   		emf_mailbox_t *mailbox_list=NULL;
 *   		
 *   		printf("\n > Enter account id: ");
 *   		scanf("%d", &account_id);
 *   		printf("\n > Enter mailbox_sync_type: ");
 *   		scanf("%d", &mailbox_sync_type);
 *
 *   		if((EMF_ERROR_NONE != email_get_mailbox_list(account_id, mailbox_sync_type, &mailbox_list, &count)))
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
EXPORT_API int  email_get_mailbox_list(int account_id, int mailbox_sync_type, emf_mailbox_t** mailbox_list, int* count);

EXPORT_API int email_get_mailbox_list_ex(int account_id, int mailbox_sync_type, int with_count, emf_mailbox_t** mailbox_list, int* count);

/**
 
 * @open
 * @fn EXPORT_API int email_get_mailbox_by_name(int account_id, const char *pMailboxName, emf_mailbox_t **pMailbox);
 * @brief 	Get the mailbox information by name.This function gets the mailbox by given mailbox name for a specified account.  	
 *
 * @return This function returns EMF_ERROR_NONE on success or error code(refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id		Specifies the information of account Id.
 * @param[in] pMailboxName		Specifies the mailbox name.
 * @param[out] pMailbox			Specifies the information of mailbox 
 * @exception none
 * @see 	emf_mailbox_t
 * @remarks N/A
 */

EXPORT_API int email_get_mailbox_by_name(int account_id, const char *pMailboxName, emf_mailbox_t **pMailbox);

// Belows are for A Project

/**
 
 * @open
 * @fn email_get_child_mailbox_list(int account_id, char *parent_mailbox,  emf_mailbox_t** mailbox_list, int* count)
 * @brief	Get all sub mailboxes for given parent mailbox.This function gives all the child mailbox list for a given parent mailbox for specified account.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code(refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id		Specifies the account ID.
 * @param[in] parent_mailbox		Specifies the parent mailbox
 * @param[out] mailbox_list	       Specifies the pointer of mailbox structure pointer.(possibly NULL)
 * @param[out] count			The mailbox count
 * @exception  #EMF_ERROR_INVALID_PARAM  	-Invalid argument
 * @see 	emf_mailbox_t
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
EXPORT_API int email_get_child_mailbox_list(int account_id, const char *parent_mailbox,  emf_mailbox_t** mailbox_list, int* count);


/**
 * @open
 * @fn email_get_mailbox_by_mailbox_type(int account_id, emf_mailbox_type_e mailbox_type,  emf_mailbox_t** mailbox)
 * @brief	Get mailbox by mailbox_type.This function is invoked when user wants to know the mailbox information by mailbox_type for the given account.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code(refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id		Specifies the account ID.
 * @param[in] mailbox_type		Specifies the mailbox type.
 * @param[out] mailbox		Specifies the pointer of mailbox structure pointer.(possibly NULL)
 * @exception none
 * @see  	emf_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_get_mailbox_by_mailbox_type(int account_id, emf_mailbox_type_e mailbox_type,  emf_mailbox_t** mailbox);

/**
 * @open
 * @fn email_set_mail_slot_size(int account_id, char* mailbox_name, int new_slot_size, unsigned* handle)
 * @brief	Set mail slot size.This function is invoked when user wants to set the size of mail slot.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code(refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id		Specifies the account ID.
 * @param[in] mailbox_name		Specifies the mailbox name.
 * @param[in] new_slot_size		Specifies the mail slot size.
 * @exception none
 * @see  	emf_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_set_mail_slot_size(int account_id, char* mailbox_name, int new_slot_size/*, unsigned* handle*/);


#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @} @}
*/



#endif /* __EMF_MAPI_FOLDER_H__ */


