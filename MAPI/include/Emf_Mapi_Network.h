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


#ifndef __EMF_MAPI_NETWORK_H__
#define __EMF_MAPI_NETWORK_H__

#include "emf-types.h"

/**
* @defgroup EMAIL_FRAMEWORK Email Service
* @{
*/

/**
* @ingroup EMAIL_FRAMEWORK
* @defgroup EMAIL_MAPI_NETWORK Email Network API
* @{
*/

/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with Email Engine.
 * @file		Emf_Mapi_Network.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 		This file contains the data structures and interfaces of Network related Functionality provided by 
 *			Email Engine . 
 *
 * @{
  
 * @code
 
 * 	#include "Emf_Mapi_Network.h"
 *
 *	bool
 *	other_app_invoke_uniform_api_sample(int* error_code)
 *	{
 *
 *		// Send a mail 
 *		emf_mailbox_t mbox;
 *		emf_mail_t *mail = NULL;
 *		emf_mail_head_t *head =NULL;
 *		emf_mail_body_t *body =NULL;
 *		emf_attachment_info_t attachment;
 *		emf_option_t option;
 *		int account_id = 1;
 *		int err = EMF_ERROR_NONE;
 *		int mail_id = 1;
 *		char arg[50]; //Input attachment number need to be download
 *		emf_event_status_type_t  status;
 *		int action = -1;
 *		int on_sending = 0;
 *	       	int on_receiving = 0;
 * 
 *		mail =( emf_mail_t *)malloc(sizeof(emf_mail_t));
 *		head =( emf_mail_head_t *)malloc(sizeof(emf_mail_head_t));
 *		body =( emf_mail_body_t *)malloc(sizeof(emf_mail_body_t));
 *		mail->info =( emf_mail_info_t*) malloc(sizeof(emf_mail_info_t));
 *		memset(mail->info, 0x00, sizeof(emf_mail_info_t));
 *
 *		memset(&mbox, 0x00, sizeof(emf_mailbox_t));
 *		memset(&option, 0x00, sizeof(emf_option_t));
 *		memset(mail, 0x00, sizeof(emf_mail_t));
 *		memset(head, 0x00, sizeof(emf_mail_head_t));
 *		memset(body, 0x00, sizeof(emf_mail_body_t));
 *		memset(&attachment, 0x00, sizeof(emf_attachment_info_t));
 *
 *		option.keep_local_copy = 1;
 *		
 *		mbox.account_id = account_id;
 *		mbox.name = strdup("OUTBOX");
 *		head->to=strdup("test@test.com");
 *		head->subject =strdup("test");
 *		body->plain = strdup("/tmp/mail.txt");
 *		
 *		mail->info->account_id = account_id;
 *		mail->info->flags.draft = 1;
 *		mail->body = body;
 *		mail->head = head;
 *				
 *		attachment.name = strdup("attach");
 *		attachment.savename = strdup("/tmp/mail.txt");
 *		attachment.next = NULL;
 *		mail->body->attachment = &attachment;
 *		mail->body->attachment_num = 1;
 *		if(EMF_ERROR_NONE  == email_add_message(mail,&mbox,1))
 *		{
 *	 		if(EMF_ERROR_NONE == email_send_mail(&mbox, mail->info->uid,&option,&handle))
 *				//success
 *			else
 *				//failure
 *		}
 *
 *		// Download header of new emails from mail server
 *		unsigned handle = 0;
 *
 *		memset(&mbox, 0x00, sizeof(emf_mailbox_t));
 *
 *		mbox.account_id = account_id;
 *		mbox.name = strdup("INBOX");
 *		if(EMF_ERROR_NONE == email_sync_header (&mbox,&handle))
 *			//success
 *		else
 *			//failure
 *
 *		//Sync mail header for all accounts
 *		if(EMF_ERROR_NONE == email_sync_header_for_all_account(&handle))
 *			//success
 *		else
 *			//failure
 *		
 *		//Download email body from server
 *
 *		memset(&mbox, 0x00, sizeof(emf_mailbox_t));
 *		mbox.account_id = account_id;
 *		mbox.name = strdup("INBOX");
 *		if(EMF_ERROR_NONE == email_download_body (&mbox,mail_id,0,&handle))
 *			//success
 *		else
 *			//failure
 *		
 *		//Download a email nth-attachment from server
 *		prinf("Enter attachment number\n");
 *		scanf("%s",arg);
 *		memset(&mbox, 0x00, sizeof(emf_mailbox_t));
 *		mbox.name = strdup("INBOX");
 *		mbox.account_id = account_id;
 *		if(EMF_ERROR_NONE == email_download_attachment(&mailbox,mail_id,arg,&handle))
 *			//success
 *		else
 *			//failure
 *
 *		//Cancel job
 *		if(EMF_ERROR_NONE == email_cancel_job(account_id,handle))//canceling download email nth attachment from server job.
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
 *		if(EMF_ERROR_NONE == email_get_pending_job(action,account_id,mail_id,&status))
 *			//success
 *		else
 *			//error
 *
 *		//Get Network status
 *		if(EMF_ERROR_NONE == email_get_network_status(&sending,&receiving))
 *			//success
 *		else
 *			//failure
 *
 *		//Send read report
 *		if(EMF_ERROR_NONE == email_send_report(mail ,&handle))
 *			//success
 *		else
 *			//failure
 *		//Save and send
 *
 *		mbox.account_id = account_id;
 *		mbox.name = strdup("DRAFTBOX");
 *		
 *		if(EMF_ERROR_NONE  == email_add_message(mail,&mbox,1))
 *		{
 *	 		if(EMF_ERROR_NONE == email_send_saved(account_id,&option,&handle))
 *				//success
 *			else
 *				//failure
 *		}
 *		//Get Imap mailbox list
 *		printf("\n > Enter server name:\n");
 *		scanf("%s",arg);
 *		if(EMF_ERROR_NONE == email_get_imap_mailbox_list(account_id , arg , &handle))
 *			//success
 *		else
 *			//failure
 *
 *		//sync local activity
 *		if(EMF_ERROR_NONE == email_sync_local_activity(account_id))
 *			//success
 *		else
 *			//failure
 * }
 *
 * @endcode
 * @}

 */
 


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @open
 * @fn email_send_mail( emf_mailbox_t* mailbox, int mail_id, emf_option_t* sending_option, unsigned* handle)
 * @brief	Send a mail.This function is invoked when user wants to send a composed mail.
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mailbox	     		Specifies the mailbox to consist a sending email.
 * @param[in] mail_id	     		Specifies the mail ID.
 * @param[in] sending_option 		Specifies the sending option.
 * @param[out] handle 		Specifies the sending handle.
 * @exception 	none
 * @see 	emf_mailbox_t and emf_option_t
 * @remarks N/A
 */
EXPORT_API int email_send_mail( emf_mailbox_t* mailbox, 
				int mail_id, 
				emf_option_t* sending_option, 
				unsigned* handle);


/**
 * @open
 * @fn email_sync_header(emf_mailbox_t* mailbox, unsigned* handle)
 * @brief	Download header of new emails from mail server.This function is invoked when user wants to download only header of new mails. 
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mailbox		Specifies the structure of mailbox.
 * @param[out] handle		Specifies the handle for stopping downloading.
 * @exception 	none
 * @see 	emf_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_sync_header(emf_mailbox_t* mailbox, unsigned* handle);


/**
 * @open
 * @fn email_sync_header_for_all_account(unsigned* handle)
 * @brief	Download header of new emails from mail server for all emails.This function is invoked when user wants to download header of new mails for all accounts.
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[out] handle		Specifies the handle for stopping downloading.
 * @exception none
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_sync_header_for_all_account(unsigned* handle);


/**
 * @open
 * @fn email_download_body(emf_mailbox_t* mailbox, int mail_id, int with_attachment, unsigned* handle)
 * @brief	Download email body from server.This function is invoked when user wants to download email body with/without attachment based on the option with_attachment
 * 		from the server.
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mailbox			Specifies the structure of mailbox.
 * @param[in] mail_id			Specifies the mail ID.
 * @param[in] with_attachment	Specifies the whether attachment is there or not.
 * @param[out] handle		Specifies the handle for stopping downloading.
 * @exception none
 * @see emf_mailbox_t
 * @remarks N/A
 */
EXPORT_API int email_download_body(emf_mailbox_t* mailbox, int mail_id, int with_attachment, unsigned* handle);






/**
 * @open
 * @fn email_download_attachment(emf_mailbox_t* mailbox, int mail_id, char* nth, unsigned* handle);
 * @brief	Download a email nth-attachment from server.This function is invoked if user wants to download only specific attachment of a mail whose body is already downloaded.
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mailbox		Reserved.
 * @param[in] mail_id		Specifies the mail ID.
 * @param[in] nth			Specifies the buffer that a attachment number been saved. the minimum number is "1".
 * @param[out] handle		Specifies the handle for stopping downloading.
 * @exception none
 * @see 	emf_mailbox_t  
 * @remarks N/A
 */
EXPORT_API int email_download_attachment(emf_mailbox_t* mailbox, 
							int mail_id,
							const char* nth,  
							unsigned* handle);


/**
 * @open
 * @fn email_cancel_job(int account_id, int handle);
 * @brief	cancel the ongoing job.This function is invoked if user wants to cancel any ongoing job of a specified account.
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id	Specifies the account ID.
 * @param[in] handle		Specifies the handle for stopping the operation.
 * @exception	 none
 * @see 	none  
 * @remarks N/A
 */

EXPORT_API int email_cancel_job(int account_id, int handle);


/**
 * @open
 * @fn email_get_pending_job(emf_action_t action, int account_id, int mail_id, emf_event_status_type_t * status);
 * @brief	get pending job list.This function is invoked if user wants to get the pending job list with status information .
 * 		Based on action item of a mail is for a specific account this will give all pending job list.
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id	Specifies the action of the job.
 * @param[in] account_id	Specifies the account ID.
 * @param[in] mail_id		Specifies the mail ID.
 * @param[out]status		Specifies the status of the job.
 * @exception	 none
 * @see 	emf_action_t and emf_event_status_type_t  
 * @remarks N/A
 */
EXPORT_API int email_get_pending_job(emf_action_t action, int account_id, int mail_id, emf_event_status_type_t * status);


/**
 * @open
 * @fn email_get_network_status(int* on_sending, int* on_receiving)
 * @brief	This function gives the  current network status.This gives the information to the user whether sending operation is in progress or receiving operation.
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[out] on_sending		True if sending is in progress.
 * @param[out] on_receiving		True if receivng is in progress.
 * @exception 	none
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_get_network_status(int* on_sending, int* on_receiving);


/**
 * @open
 * @fn email_send_report(emf_mail_t* mail, unsigned* handle)
 * @brief	Send a read receipt mail.This function is invoked when user receives a mail with read report enable and wants to send a read report for the same.
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mail		    		Specifies the mail to been read.
 * @param[out] handle	     		Specifies the handle for stopping sending.
 * @exception none
 * @see  	emf_mail_t
 * @remarks N/A
 */
EXPORT_API int email_send_report(emf_mail_t* mail,  unsigned* handle);


/**
 * @open
 * @fn email_send_saved(int account_id, emf_option_t* sending_option, unsigned* handle)
 * @brief	Send all mails to been saved in Offline-mode.This function is invoked when user wants to send an email and after saving it.
 * 		This will save the email in draft mailbox and then sends. 
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id			Specifies the account ID.
 * @param[in] sending_option 		Specifies the sending option.
 * @param[out] handle				Specifies the handle for stopping sending.
 * @exception none
 * @see emf_option_t
 * @remarks N/A
 */
EXPORT_API int email_send_saved(int account_id, emf_option_t* sending_option, unsigned* handle);


EXPORT_API int email_download_mail_from_server(emf_mailbox_t * mailbox, const char * server_mail_id, unsigned* handle) DEPRECATED;


/**
 * @open
 * @fn email_get_imap_mailbox_list(int account_id, char* mailbox, unsigned* handle)
 *  @brief	fetch all the mailbox names from server and store the non-existing mailboxes in DB.This function is invoked when user wants to download all server mailboxes from IMAP server
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id			Specifies the account ID.
 * @param[in] mailbox 			Specifies the mailbox name.
 * @param[out] handle			Specifies the handle for stopping Network operation.
 * @exception 	none
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_get_imap_mailbox_list(int account_id, const char* mailbox, unsigned* handle);



/**
 * @open
 * @fn email_sync_local_activity(int account_id)
 *  @brief	sync local activity
 *
 * @return 	This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id			Specifies the account ID.
 * @exception 	none
 * @see 	none
 * @remarks N/A
 */
EXPORT_API int email_sync_local_activity(int account_id);


/**
 * @open
 * @fn email_find_mail_on_server(int account_id, const char *mailbox_name, int search_type, char *search_value)
 * @brief	Search the mails on server.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] account_id	Specifies the Account ID
 * @param[in] mailbox_name	Specifies the Mailbox Name
 * @param[in] search_type	Specifies the searching type(EMF_SEARCH_FILTER_SUBJECT,  EMF_SEARCH_FILTER_SENDER, EMF_SEARCH_FILTER_RECIPIENT, EMF_SEARCH_FILTER_ALL)
 * @param[in] search_value	Specifies the value to use for searching. (ex : Subject, email address, display name)
 * @exception 		none
 * @see
 * @code
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_find_mail_on_server(int input_account_id, const char *input_mailbox_name, int input_search_type, char *input_search_value, unsigned *output_handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @}
*/


#endif /* __EMF_MAPI_NETWORK_H__ */
