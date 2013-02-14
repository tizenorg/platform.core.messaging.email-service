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
* @defgroup EMAIL_SERVICE Email Service
* @{
*/

/**
* @ingroup EMAIL_SERVICE
* @defgroup EMAIL_API_NETWORK Email Network API
* @{
*/

/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-network.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 		This file contains the data structures and interfaces of Network related Functionality provided by
 *			email-service .
 *
 * @{

 * @code

 * 	#include "email-api.h"
 *
 *	bool
 *	other_app_invoke_uniform_api_sample(int* error_code)
 *	{
 *
 *		// Send a mail
 *		email_attachment_data_t attachment;
 *		int account_id = 1;
 *		int mailbox_id = 0;
 *		int attachment_id = 0;
 *		int err = EMAIL_ERROR_NONE;
 *		int mail_id = 0;
 *		int action = -1;
 *
 *		printf("Enter mail id\n");
 *		scanf("%d",&mail_id);
 *
 *	 	if(EMAIL_ERROR_NONE == email_send_mail(mail_id, &handle))
 *			//success
 *		else
 *			//failure
 *
 *		// Download header of new emails from mail server
 *		int handle = 0;
 *
 *		mailbox.account_id = account_id;
 *		printf("Enter mailbox id\n");
 *		scanf("%d",&mailbox_id);
 *		if(EMAIL_ERROR_NONE == email_sync_header (account_id, mailbox_id, &handle))
 *			//success
 *		else
 *			//failure
 *
 *		//Sync mail header for all accounts
 *		if(EMAIL_ERROR_NONE == email_sync_header_for_all_account(&handle))
 *			//success
 *		else
 *			//failure
 *
 *		//Download email body from server
 *
 *		if(EMAIL_ERROR_NONE == email_download_body (mail_id,0,&handle))
 *			//success
 *		else
 *			//failure
 *
 *		//Download a email nth-attachment from server
 *		prinf("Enter attachment number\n");
 *		scanf("%d",&attachment_id);
 *		if(EMAIL_ERROR_NONE == email_download_attachment(mail_id, attachment_id, &handle))
 *			//success
 *		else
 *			//failure
 *
 *		//Cancel job
 *		if(EMAIL_ERROR_NONE == email_cancel_job(account_id,handle))//canceling download email nth attachment from server job.
 *									//so this handle contains the value return by the email_download_attachment()
 *			//success
 *		else
 *			//failure
 * 		//Get pending job listfor an account
 *
 *		printf( " Enter Action \n SEND_MAIL = 0 \n SYNC_HEADER = 1 \n" \
 *			    " DOWNLOAD_BODY,= 2 \n DOWNLOAD_ATTACHMENT = 3 \n" \
 *			    " DELETE_MAIL = 4 \n SEARCH_MAIL = 5 \n SAVE_MAIL = 6 \n" \
 *			    " NUM = 7 \n");
 *	    	scanf("%d",&action);
 *		if(EMAIL_ERROR_NONE == email_get_pending_job(action,account_id,mail_id,&status))
 *			//success
 *		else
 *			//error
 *
 *		//Get Network status
 *		if(EMAIL_ERROR_NONE == email_get_network_status(&sending,&receiving))
 *			//success
 *		else
 *			//failure
 *
 *		//Send read report
 *		if(EMAIL_ERROR_NONE == email_send_report(mail ,&handle))
 *			//success
 *		else
 *			//failure
 *		//Save and send
 *
 *
 *		if(EMAIL_ERROR_NONE  == email_add_mail(mail,NULL,0, NULL, 0))
 *		{
 *	 		if(EMAIL_ERROR_NONE == email_send_saved(account_id,&option,&handle))
 *				//success
 *			else
 *				//failure
 *		}
 *		//Get Imap mailbox list
 *		if(EMAIL_ERROR_NONE == email_sync_imap_mailbox_list(account_id,  &handle))
 *			//success
 *		else
 *			//failure
 *
 * }
 *
 * @endcode
 * @}

 */



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**

 * @fn email_send_mail(int mail_id,	int *handle)
 * @brief	Send a mail.This function is invoked when user wants to send a composed mail.
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id	     		Specifies the mail ID.
 * @param[out] handle 		Specifies the sending handle.
 * @exception 	none
 * @see 	email_mailbox_t and email_option_t
 * @remarks N/A
 */
EXPORT_API int email_send_mail(int mail_id,	int *handle);

EXPORT_API int email_send_mail_with_downloading_attachment_of_original_mail(int input_mail_id, int *output_handle);


/**

 * @fn email_sync_header(int input_account_id, int input_mailbox_id, int *handle)
 * @brief	Download header of new emails from mail server.This function is invoked when user wants to download only header of new mails.
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_account_id		Specifies the account ID.
 * @param[in] input_mailbox_id		Specifies the mailbox ID.
 * @param[out] handle		Specifies the handle for stopping downloading.
 * @exception 	none
 * @see 	email_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_sync_header(int input_account_id, int input_mailbox_id, int *handle);


/**

 * @fn email_sync_header_for_all_account(int *handle)
 * @brief	Download header of new emails from mail server for all emails.This function is invoked when user wants to download header of new mails for all accounts.
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[out] handle		Specifies the handle for stopping downloading.
 * @exception none
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_sync_header_for_all_account(int *handle);


/**

 * @fn email_download_body(int mail_id, int with_attachment, int *handle)
 * @brief	Download email body from server.This function is invoked when user wants to download email body with/without attachment based on the option with_attachment
 * 		from the server.
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id			Specifies the mail ID.
 * @param[in] with_attachment	Specifies the whether attachment is there or not.
 * @param[out] handle		Specifies the handle for stopping downloading.
 * @exception none
 * @see email_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_download_body(int mail_id, int with_attachment, int *handle);






/**

 * @fn email_download_attachment(int mail_id, const char* nth, int *handle);
 * @brief	Download a email nth-attachment from server.This function is invoked if user wants to download only specific attachment of a mail whose body is already downloaded.
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id		Specifies the mail ID.
 * @param[in] nth			Specifies the attachment number been saved. the minimum number is "1".
 * @param[out] handle		Specifies the handle for stopping downloading.
 * @exception none
 * @see 	email_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_download_attachment(int mail_id, int nth, int *handle);


/**

 * @fn email_cancel_job(int account_id, int handle);
 * @brief	cancel the ongoing job.This function is invoked if user wants to cancel any ongoing job of a specified account.
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_account_id	Specifies the account ID.
 * @param[in] input_handle		Specifies the handle for stopping the operation.
 * @param[in] input_cancel_type	Specifies the type of cancellation.
 * @exception	 none
 * @see 	none
 * @remarks N/A
 */

EXPORT_API int email_cancel_job(int input_account_id, int input_handle, email_cancelation_type input_cancel_type);


/**

 * @fn email_get_pending_job(email_action_t action, int account_id, int mail_id, email_event_status_type_t * status);
 * @brief	get pending job list.This function is invoked if user wants to get the pending job list with status information .
 * 		Based on action item of a mail is for a specific account this will give all pending job list.
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] account_id	Specifies the action of the job.
 * @param[in] account_id	Specifies the account ID.
 * @param[in] mail_id		Specifies the mail ID.
 * @param[out]status		Specifies the status of the job.
 * @exception	 none
 * @see 	email_action_t and email_event_status_type_t
 * @remarks N/A
 */
EXPORT_API int email_get_pending_job(email_action_t action, int account_id, int mail_id, email_event_status_type_t * status);


/**

 * @fn email_get_network_status(int* on_sending, int* on_receiving)
 * @brief	This function gives the  current network status.This gives the information to the user whether sending operation is in progress or receiving operation.
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[out] on_sending		True if sending is in progress.
 * @param[out] on_receiving		True if receivng is in progress.
 * @exception 	none
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_get_network_status(int* on_sending, int* on_receiving);

/**

 * @fn email_send_saved(int account_id, int *handle)
 * @brief	Send all mails to been saved in Offline-mode.This function is invoked when user wants to send an email and after saving it.
 * 		This will save the email in draft mailbox and then sends.
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] account_id			Specifies the account ID.
 * @param[out] handle				Specifies the handle for stopping sending.
 * @exception none
 * @see email_option_t
 * @remarks N/A
 */
EXPORT_API int email_send_saved(int account_id, int *handle);

/**

 * @fn email_sync_imap_mailbox_list(int account_id, int *handle)
 *  @brief	fetch all the mailbox names from server and store the non-existing mailboxes in DB.This function is invoked when user wants to download all server mailboxes from IMAP server
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] account_id			Specifies the account ID.
 * @param[out] handle			Specifies the handle for stopping Network operation.
 * @exception 	none
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_sync_imap_mailbox_list(int account_id, int *handle);

/**

 * @fn email_search_mail_on_server(int input_account_id, int input_mailbox_id, email_search_filter_t *input_search_filter_list, int input_search_filter_count, int *output_handle)
 * @brief	Search the mails on server.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] account_id	Specifies the Account ID
 * @param[in] mailbox_id	Specifies the Mailbox ID
 * @param[in] search_type	Specifies the searching type(EMAIL_SEARCH_FILTER_SUBJECT,  EMAIL_SEARCH_FILTER_SENDER, EMAIL_SEARCH_FILTER_RECIPIENT, EMAIL_SEARCH_FILTER_ALL)
 * @param[in] search_value	Specifies the value to use for searching. (ex : Subject, email address, display name)
 * @exception 		none
 * @see email_search_filter_t,
 * @code
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_search_mail_on_server(int input_account_id, int input_mailbox_id, email_search_filter_t *input_search_filter_list, int input_search_filter_count, int *output_handle);

EXPORT_API int email_clear_result_of_search_mail_on_server(int input_account_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @}
*/


#endif /* __EMAIL_API_NETWORK_H__ */
