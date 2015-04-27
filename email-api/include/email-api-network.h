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


#ifndef __EMAIL_API_NETWORK_H__
#define __EMAIL_API_NETWORK_H__

#include "email-types.h"

/**
 * @internal
 * @file email-api-network.h
 * @brief This file contains the data structures and interfaces of Network related Functionality provided by
 *        email-service.
 *
 * @{
 * @code

 *  #include "email-api.h"
 *
 *  bool
 *  other_app_invoke_uniform_api_sample(int* error_code)
 *  {
 *
 *      // Sends a mail
 *      email_attachment_data_t attachment;
 *      int account_id = 1;
 *      int mailbox_id = 0;
 *      int attachment_id = 0;
 *      int err = EMAIL_ERROR_NONE;
 *      int mail_id = 0;
 *      int action = -1;
 *
 *      printf("Enter mail id\n");
 *      scanf("%d",&mail_id);
 *
 *      if(EMAIL_ERROR_NONE == email_send_mail(mail_id, &handle))
 *          //success
 *      else
 *          //failure
 *
 *      // Downloads header of new emails from mail server
 *      int handle = 0;
 *
 *      mailbox.account_id = account_id;
 *      printf("Enter mailbox id\n");
 *      scanf("%d",&mailbox_id);
 *      if(EMAIL_ERROR_NONE == email_sync_header (account_id, mailbox_id, &handle))
 *          //success
 *      else
 *          //failure
 *
 *      //Syncs mail header for all accounts
 *      if(EMAIL_ERROR_NONE == email_sync_header_for_all_account(&handle))
 *          //success
 *      else
 *          //failure
 *
 *      //Downloads email body from server
 *
 *      if(EMAIL_ERROR_NONE == email_download_body (mail_id,0,&handle))
 *          //success
 *      else
 *          //failure
 *
 *      //Downloads an email nth-attachment from server
 *      prinf("Enter attachment number\n");
 *      scanf("%d",&attachment_id);
 *      if(EMAIL_ERROR_NONE == email_download_attachment(mail_id, attachment_id, &handle))
 *          //success
 *      else
 *          //failure
 *
 *      //Cancel job
 *      if(EMAIL_ERROR_NONE == email_cancel_job(account_id,handle))//canceling download email nth attachment from server job.
 *                                  //so this handle contains the value return by the email_download_attachment()
 *          //success
 *      else
 *          //failure
 *      //Gets pending job list for an account
 *
 *      printf( " Enter Action \n SEND_MAIL = 0 \n SYNC_HEADER = 1 \n" \
 *              " DOWNLOAD_BODY,= 2 \n DOWNLOAD_ATTACHMENT = 3 \n" \
 *              " DELETE_MAIL = 4 \n SEARCH_MAIL = 5 \n SAVE_MAIL = 6 \n" \
 *              " NUM = 7 \n");
 *          scanf("%d",&action);
 *      if(EMAIL_ERROR_NONE == email_get_pending_job(action,account_id,mail_id,&status))
 *          //success
 *      else
 *          //error
 *
 *      //Gets Network status
 *      if(EMAIL_ERROR_NONE == email_get_network_status(&sending,&receiving))
 *          //success
 *      else
 *          //failure
 *
 *      //Sends read report
 *      if(EMAIL_ERROR_NONE == email_send_report(mail ,&handle))
 *          //success
 *      else
 *          //failure
 *      //Saves and sends
 *
 *
 *      if(EMAIL_ERROR_NONE  == email_add_mail(mail,NULL,0, NULL, 0))
 *      {
 *          if(EMAIL_ERROR_NONE == email_send_saved(account_id,&option,&handle))
 *              //success
 *          else
 *              //failure
 *      }
 *      //Get Imap mailbox list
 *      if(EMAIL_ERROR_NONE == email_sync_imap_mailbox_list(account_id,  &handle))
 *          //success
 *      else
 *          //failure
 *
 * }
 *
 * @endcode
 * @}
 */

/**
 * @internal
 * @ingroup EMAIL_SERVICE_FRAMEWORK
 * @defgroup EMAIL_SERVICE_NETWORK_MODULE Network API
 * @brief Network API is a set of operations to manage email send, receive and cancel related details.
 *
 * @section EMAIL_SERVICE_NETWORK_MODULE_HEADER Required Header
 *   \#include <email-api-network.h>
 *
 * @section EMAIL_SERVICE_NETWORK_MODULE_OVERVIEW Overview
 * Network API is a set of operations to manage email send, receive and cancel related details.
 */

/**
 * @internal
* @addtogroup EMAIL_SERVICE_NETWORK_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Sends a mail.
 * @details This function is invoked when a user wants to send a composed mail.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]   mail_id  The mail ID
 * @param[out]  handle   The sending handle
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t and #email_option_t
 */
EXPORT_API int email_send_mail(int mail_id,	int *handle);

/**
 * @brief Sends a mail.
 * @details This function is invoked when a user wants to send the mail, not been downloaded the attachment.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]   input_mail_id  The mail ID
 * @param[out]  handle         The sending handle
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t and #email_option_t
 */
EXPORT_API int email_send_mail_with_downloading_attachment_of_original_mail(int input_mail_id, int *output_handle);

/**
 * @brief Sends a mail.
 * @details This function is invoked when a user wants to send the scheduled mail.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]   input_mail_id  The mail ID
 * @param[out]  input_time     The scheduled time
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (refer to #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t and #email_option_t
 */
EXPORT_API int email_schedule_sending_mail(int input_mail_id, time_t input_time);


/**
 * @brief Downloads headers of new emails from the mail server.
 * @details This function is invoked when a user wants to download only the headers of new mails.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  input_account_id  The account ID
 * @param[in]  input_mailbox_id  The mailbox ID
 * @param[out] handle            The handle for stopping downloading
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_sync_header(int input_account_id, int input_mailbox_id, int *handle);


/**
 * @brief Downloads headers of new emails from the mail server for all emails.
 * @details This function is invoked when a user wants to download headers of new mails for all accounts.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[out]  handle  The handle for stopping downloading
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_sync_header_for_all_account(int *handle);


/**
 * @brief Downloads an email body from the server.
 * @details This function is invoked when a user wants to download email body with/without attachment based on the @a with_attachment option
 *          from the server.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]   mail_id          The mail ID
 * @param[in]   with_attachment  The flag indicating whether there is an attachment
 * @param[out]  handle           The handle for stopping downloading
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_download_body(int mail_id, int with_attachment, int *handle);


/**
 * @brief Downloads an email nth-attachment from the server.
 * @details This function is invoked if a user wants to download only specific attachment of a mail whose body is already downloaded.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  mail_id  The mail ID
 * @param[in]  nth      The attachment number been saved (The minimum number is "1")
 * @param[out] handle   The handle for stopping downloading
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 */
EXPORT_API int email_download_attachment(int mail_id, int nth, int *handle);


/**
 * @brief Cancels the ongoing job.
 * @details This function is invoked if a user wants to cancel any ongoing job of a specified account.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] input_account_id   The account ID
 * @param[in] input_handle       The handle for stopping the operation
 * @param[in] input_cancel_type  The type of cancellation
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_cancel_job(int input_account_id, int input_handle, email_cancelation_type input_cancel_type);

EXPORT_API int email_get_pending_job(email_action_t action, int account_id, int mail_id, email_event_status_type_t * status) DEPRECATED;

EXPORT_API int email_get_network_status(int* on_sending, int* on_receiving) DEPRECATED;

/**
 * @brief Gives the current job information.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[out]  output_task_information        The array of job information
 * @param[out]  output_task_information_count  The count of job information
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_get_task_information(email_task_information_t **output_task_information, int *output_task_information_count);

/**
 * @brief Sends all mails to be saved in offline-mode.
 * @details This function is invoked when a user wants to send an email and saving it afterwards.
 *          This will save the email in draft mailbox and then send it.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id  The account ID
 * @param[out] handle      The handle for stopping sending
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_option_t
 */
EXPORT_API int email_send_saved(int account_id, int *handle);

/**
 * @brief Fetches all mailbox names from the server and stores the non-existing mailboxes in the DB.
 * @details This function is invoked when a user wants to download all server mailboxes from IMAP server.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  account_id  The account ID
 * @param[out] handle      The handle for stopping Network operation
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_sync_imap_mailbox_list(int account_id, int *handle);

/**
 * @brief Queries the maximum mail size limit from the SMTP server.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] account_id  The Account ID
 * @param[out] handle	   The handle for stopping Network operation
 *
 * @return #EMAIL_ERROR_NONE on success,
 *         otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_query_smtp_mail_size_limit(int account_id, int *handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */


#endif /* __EMAIL_API_NETWORK_H__ */
