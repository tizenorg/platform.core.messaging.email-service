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
 * @file email-api-mailbox.h
 * @brief This file contains the data structures and interfaces of mailbox provided by email-service.
 * @{
 * @code
 *
 *  #include "email_api_mailbox.h"
 *
 *  bool other_app_invoke_uniform_api_sample(int *error_code)
 *  {
 *      email_mailbox_t mailbox;
 *      email_mailbox_t *new_mailbox =NULL;
 *      email_mailbox_t *mailbox_list = NULL;
 *      int count = 0;
 *      int mailbox_type;
 *      int handle = 0;
 *      char *pMaiboxName;
 *      char *pParentMailbox;
 *
 *      memset(&mailbox,0x00,sizeof(email_mailbox_t));
 *      mailbox.mailbox_name = strdup("test");
 *      mailbox.alias = strdup("Personal");
 *      mailbox.account_id = 1;
 *      printf("Enter local_yn(1/0)");
 *      scanf("%d",&local_yn);
 *      mailbox.local=local_yn;
 *      mailbox.mailbox_type = 7;
 *
 *      //create new mailbox
 *
 *      if(EMAIL_ERR_NONE != email_add_mailbox(&mailbox,local_yn,&handle))
 *          printf("email_add_mailbox failed\n");
 *      else
 *          printf("email_add_mailbox success");
 *
 *      //delete mailbox
 *
 *      if(EMAIL_ERROR_NONE != email_delete_mailbox(mailbox,local_yn,&handle))
 *          printf("email_delete_mailbox failed\n");
 *      else
 *          printf("email_delete_mailbox success\n");
 *
 *      //free mailbox
 *      email_free_mailbox("new_mailbox,1");
 *
 *      //Get mailbox list
 *      if(EMAIL_ERROR_NONE != email_get_mailbox_list(account_id,local_yn,&mailbox_list,&count))
 *          //failure
 *      else
 *          //success
 *
 *      //Get mailbox by name
 *      pMailboxName = strdup("test");
 *      if(EMAIL_ERROR_NONE != email_get_mailbox_by_name(account_id,pMailboxName,&mailbox_list))
 *          //failure
 *      else
 *          //success
 *
 *      //Get child mailbox list
 *      pParentMailbox = strdup("test");
 *      if(EMAIL_ERROR_NONE != email_get_child_mailbox_list(account_id, paerent_mailbox,&mailbox_list,&count))
 *          //failure
 *      else
 *          //success
 *
 *      //Get mailbox by mailbox_type
 *      printf("Enter mailbox_type\n");
 *      scanf("%d",&mailbox_type);
 *      if(EMAIL_ERROR_NONE != email_get_mailbox_by_mailbox_type(account_id,mailbox_type,&mailbox_list))
 *          //failure
 *      else
 *          //success
 *
 *  }
 *
 * @endcode
 * @}
 */

/**
 * @internal
 * @ingroup EMAIL_SERVICE_FRAMEWORK
 * @defgroup EMAIL_SERVICE_MAILBOX_MODULE Mailbox API
 * @brief Mailbox API is a set of operations to manage email mailboxes like add, update, delete or get mailbox related details.
 *
 * @section EMAIL_SERVICE_MAILBOX_MODULE_HEADER Required Header
 *   \#include <email-api-mailbox.h>
 *
 * @section EMAIL_SERVICE_MAILBOX_MODULE_OVERVIEW Overview
 * Mailbox API is a set of operations to manage email mailboxes like add, update, delete or get mailbox related details.
 */

/**
 * @internal
 * @addtogroup EMAIL_SERVICE_MAILBOX_MODULE
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/**
 * @brief Creates a new mailbox.
 * @details This function is invoked when a user wants to create a new mailbox for the specified account.\n
 *          If on_server is true, it will create the mailbox on the server as well as locally.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  new_mailbox  The pointer of creating mailbox information
 * @param[in]  on_server    The creating mailbox on server
 * @param[out] handle       The sending handle
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_add_mailbox(email_mailbox_t *new_mailbox, int on_server, int *handle);

/**
 * @brief Changes the name of a mailbox.
 * @details This function is invoked when a user wants to change the name of an existing mail box.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @remarks See email-errors.h for exception.
 *
 * @param[in]  input_mailbox_id     The mailbox ID
 * @param[in]  input_mailbox_name   The name of the mailbox
 * @param[in]  input_mailbox_alias  The alias of the mailbox
 * @param[in]  input_on_server      The moving mailbox on server
 * @param[out] output_handle        The handle to manage tasks
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t, #email_mailbox_type_e
 */
EXPORT_API int email_rename_mailbox(int input_mailbox_id, char *input_mailbox_name, char *input_mailbox_alias, int input_on_server, int *output_handle);

/**
 * @brief Changes the name of a mailbox.
 * @details This function is invoked when a user wants to change the name of an existing mail box.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @remarks See email-errors.h for exception.
 *
 * @param[in]  input_mailbox_id       The mailbox ID
 * @param[in]  input_mailbox_name     The name of the mailbox
 * @param[in]  input_mailbox_alias    The alias of the mailbox
 * @param[in]  input_eas_data         The EAS data
 * @param[in]  input_eas_data_length  The length of EAS data
 * @param[in]  input_on_server        The moving mailbox on server
 * @param[out] output_handle          The handle to manage tasks
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t, #email_mailbox_type_e
 */
EXPORT_API int email_rename_mailbox_ex(int input_mailbox_id, char *input_mailbox_name, char *input_mailbox_alias, void *input_eas_data, int input_eas_data_length, int input_on_server, int *output_handle);

/**
 * @brief Deletes a mailbox.
 * @details This function deletes the existing mailbox for the specified account based on the on_server option. \n
 *          If on_server is true, it deletes the mailbox from the server as well as locally.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @remarks See email-errors.h for exception.
 *
 * @param[in]  input_mailbox_id  The target mailbox ID
 * @param[in]  input_on_server   The deleting mailbox on server
 * @param[out] output_handle     The sending handle
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_delete_mailbox(int input_mailbox_id, int input_on_server, int *output_handle);

/**
 * @brief Deletes a mailbox.
 * @details This function deletes the existing mailbox for the specified account based on the option on_server option. \n
 *          If on_server is true, it deletes the mailbox from the server as well as locally.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @remarks See email-errors.h for exception.
 *
 * @param[in]  input_account_id        The account ID
 * @param[in]  input_mailbox_id_array  The mailbox array for deleting
 * @param[in]  input_mailbox_id_count  The count of mailbox for deleting
 * @param[in]  input_on_server         The deleting mailbox on server
 * @param[out] output_handle           The sending handle
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_delete_mailbox_ex(int input_account_id, int *input_mailbox_id_array, int input_mailbox_id_count, int input_on_server, int *output_handle);

/**
 * @brief Changes the mailbox type.
 * @details This function is invoked when a user wants to change the mailbox type.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @remarks See email-errors.h for exception.
 *
 * @param[in] input_mailbox_id    The mailbox ID
 * @param[in] input_mailbox_type  The mailbox type
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code(see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_type_e
 */
EXPORT_API int email_set_mailbox_type(int input_mailbox_id, email_mailbox_type_e input_mailbox_type);


/**
 * @brief Changes the 'local' attribute of #email_mailbox_t.
 * @details This function is invoked when a user wants to change the 'local' attribute.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @remarks See email-errors.h for exception.
 *
 * @param[in] input_mailbox_id        The mailbox ID
 * @param[in] input_is_local_mailbox  The value of the attribute 'local' of #email_mailbox_t
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_set_local_mailbox(int input_mailbox_id, int input_is_local_mailbox);

/**
 * @brief Gets all mailboxes from account.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id         The account ID
 * @param[in]  mailbox_sync_type  The sync type
 * @param[out] mailbox_list       The pointer of mailbox structure pointer (possibly @c NULL)
 * @param[out] count              The mailbox count (possibly @c 0)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 *
 * @code
 *      #include "email-api-mailbox.h"
 *      bool
 *      _api_sample_get_mailbox_list()
 *      {
 *          int account_id =0,count = 0;
 *          int mailbox_sync_type;
 *          int error_code = EMAIL_ERROR_NONE;
 *          email_mailbox_t *mailbox_list=NULL;
 *
 *          printf("\n > Enter account id: ");
 *          scanf("%d", &account_id);
 *          printf("\n > Enter mailbox_sync_type: ");
 *          scanf("%d", &mailbox_sync_type);
 *
 *          if((EMAIL_ERROR_NONE != email_get_mailbox_list(account_id, mailbox_sync_type, &mailbox_list, &count)))
 *          {
 *              printf(" Error\n");
 *          }
 *          else
 *          {
 *              printf("Success\n");
 *              email_free_mailbox(&mailbox_list,count);
 *          }
 *      }
 * @endcode
 */
EXPORT_API int email_get_mailbox_list(int account_id, int mailbox_sync_type, email_mailbox_t** mailbox_list, int* count);

/**
 * @brief Extends the email_get_mailbox_list_ex() function.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id         The account ID
 * @param[in]  mailbox_sync_type  The sync type
 * @param[in]  with_count         The count of mailbox
 * @param[out] mailbox_list       The pointer of mailbox structure pointer (possibly @c NULL)
 * @param[out] count              The mailbox count is saved here (possibly @c 0)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_get_mailbox_list_ex(int account_id, int mailbox_sync_type, int with_count, email_mailbox_t** mailbox_list, int* count);

/**
 * @brief Gets the mailbox list based on a keyword.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id    The account ID
 * @param[in]  keyword       The specified keyword for searching
 * @param[out] mailbox_list  The pointer of mailbox structure pointer (possibly @c NULL)
 * @param[out] count         The mailbox count is saved here (possibly @c 0)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_get_mailbox_list_by_keyword(int account_id, char *keyword, email_mailbox_t** mailbox_list, int* count);

/**
 * @brief Gets a mailbox by mailbox type.
 * @details This function is invoked when a user wants to know the mailbox information by @a mailbox_type for the given account.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id    The account ID
 * @param[in]  mailbox_type  The mailbox type
 * @param[out] mailbox       The pointer of mailbox structure pointer (possibly @c NULL)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type,  email_mailbox_t** mailbox);

/**
 * @brief Gets a mailbox by mailbox ID.
 * @details This function is invoked when a user wants to know the mailbox information by mailbox ID.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  input_mailbox_id  The mailbox ID
 * @param[out] output_mailbox    The pointer of mailbox structure pointer (possibly @c NULL)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_get_mailbox_by_mailbox_id(int input_mailbox_id, email_mailbox_t** output_mailbox);

/**
 * @brief Sets a mail slot size.
 * @details This function is invoked when a user wants to set the size of mail slot.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] input_account_id     The account ID
 * @param[in] input_mailbox_id     The mailbox ID
 * @param[in] input_new_slot_size  The mail slot size
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_set_mail_slot_size(int input_account_id, int input_mailbox_id, int input_new_slot_size);

/**
 * @brief Sets the sync time of a mailbox.
 * @details This function is invoked when a user wants to set the sync time of the mailbox.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] input_mailbox_id  The mailbox ID
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_stamp_sync_time_of_mailbox(int input_mailbox_id);


/**
 * @brief Frees the memory allocated for the mailbox information.
 *
 * @since_tizen 2.3
 * @privlevel N/P
 *
 * @param[in] mailbox_list  The pointer for searching mailbox structure pointer
 * @param[in] count         The count of mailboxes
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 *
 * @code
 *      #include "email-api-mailbox.h"
 *      bool
 *      _api_sample_free_mailbox_info()
 *      {
 *      email_mailbox_t *mailbox;
 *
 *      //fill the mailbox structure
 *      //count - number of mailbox structure user want to free
 *      if(EMAIL_ERROR_NONE == email_free_mailbox(&mailbox,count))
 *           //success
 *      else
 *           //failure
 *
 *      }
 * @endcode
 */

EXPORT_API int email_free_mailbox(email_mailbox_t** mailbox_list, int count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* __EMAIL_API_MAILBOX_H__ */


