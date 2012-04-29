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



/**
 * This file defines all APIs of Email Framework.
 * @file	email-daemon.h
 * @author	Kyu-ho Jo(kyuho.jo@samsung.com)
 * @version	0.1
 * @brief	This file is the header file of email-engine library.
 */
#ifndef __EMAIL_DAEMON_H__
#define __EMAIL_DAEMON_H__

/**
* @defgroup EMAIL_SERVICE EmailFW
* @{
*/

/**
* @ingroup EMAIL_SERVICE
* @defgroup EMAIL_SERVICE Email Service
* @{
*/

#include "email-types.h"
#include "email-internal-types.h"

#include <time.h>

#ifdef __cplusplus
extern "C" 
{
#endif /* __cplusplus */

/*****************************************************************************/
/*  Initialization                                                           */
/*****************************************************************************/
/**
 * Initialize Email-engine.
 *
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_initialize(int* err_code);

/**
 * Finalize Email-engine.
 *
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_finalize(int* err_code);


/*****************************************************************************/
/*  Account                                                                  */
/*****************************************************************************/
/**
 * Create a new email account.
 *
 * @param[in] account	Specifies the structure pointer of account. 
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_create_account(emf_account_t* account, int* err_code);

/**
 * Delete a email account.
 *
 * @param[in] account_id	Specifies the account ID.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_delete_account(int account_id, int* err_code);

/**
 * Validate a email account.
 *
 * @param[in] account_id	Specifies the account ID.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_validate_account(int account_id, unsigned* handle, int* err_code);

/**
 * Change the information of a email account.
 *
 * @param[in] account_id	Specifies the orignal account ID.
 * @param[in] new_account	Specifies the information of new account.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_update_account(int account_id, emf_account_t* new_account, int* err_code);

/**
 * Change the information of a email account after validation
 *
 * @param[in] old_account_id	Specifies the orignal account ID.
 * @param[in] new_account_info	Specifies the information of new account.
 * @param[in] handle			Specifies the handle for stopping validation.
 * @param[out] err_code			Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_validate_account_and_update(int old_account_id, emf_account_t* new_account_info, unsigned* handle,int *err_code);

/**
 * Get a email account by ID.
 *
 * @param[in] account_id	Specifies the account ID.
 * @param[in] pulloption	Specifies the pulloption.
 * @param[out] acount		The returned account is saved here.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_get_account(int account_id, int pulloption, emf_account_t** acount, int* err_code);

/**
 * Get all emails.
 *
 * @param[out] acount_list	The returned accounts are saved here.(possibly NULL)
 * @param[out] count			The count of returned accounts is saved here.(possibly 0)
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_get_account_list(emf_account_t** acount_list, int* count, int* err_code);

/**
 * Free allocated memory.
 *
 * @param[in] account_list	Specifies the structure pointer of account.
 * @param[in] count			Specifies the count of accounts.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_free_account(emf_account_t** account_list, int count, int* err_code);

/**
 * Get a information of filtering.
 *
 * @param[in] filter_id			Specifies the filter ID.
 * @param[out] filtering_set	The returned information of filter are saved here.
 * @param[out] err_code			Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_get_filter(int filter_id, emf_rule_t** filtering_set, int* err_code);

/**
 * Get all filterings.
 *
 * @param[out] filtering_set		The returned filterings are saved here.(possibly NULL)
 * @param[out] count				The count of returned filters is saved here.(possibly 0)
 * @param[out] err_code			Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_get_filter_list(emf_rule_t** filtering_set, int* count, int* err_code);

/**
 * find a filter already exists.
 *
 * @param[in] filtering_set	Specifies the pointer of adding filter structure.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true if enable add filter, else returns false.
 */
INTERNAL_FUNC int emdaemon_find_filter(emf_rule_t* filter_info, int* err_code);

/**
 * Add a filter information.
 *
 * @param[in] filtering_set		Specifies the pointer of adding filter structure.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.(only EMF_FILTER_BLOCK supported.)
 */
INTERNAL_FUNC int emdaemon_add_filter(emf_rule_t* filtering_set, int* err_code);

/**
 * Change a filter information.
 *
 * @param[in] filter_id	Specifies the original filter ID.
 * @param[in] new_set	Specifies the information of new filter.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_update_filter(int filter_id, emf_rule_t* new_set, int* err_code);

/**
 * Delete a filter information.
 *
 * @param[in] filter_id	Specifies the filter ID.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_delete_filter(int filter_id, int* err_code);

/**
 * Free allocated memory.
 *
 * @param[in] filtering_set	Specifies the pointer of pointer of filter structure for memory free.
 * @param[in] count			Specifies the count of filter.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_free_filter (emf_rule_t** filtering_set, int count, int* err_code);


/*****************************************************************************/
/*  Mail                                                                     */
/*****************************************************************************/

/**
 * Send a mail.
 *
 * @param[in] mailbox	     		Specifies the mailbox to consist a sending email.
 * @param[in] mail_id	     			Specifies the mail ID.
 * @param[in] sending_option 	Specifies the sending option.
 * @param[in] callback	     		Specifies the callback function for retrieving sending information.
 * @param[in] handle	     			Specifies the handle for stopping sending.
 * @param[out] err_code	    	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_send_mail(emf_mailbox_t* mailbox, int mail_id, emf_option_t* sending_option,  unsigned* handle, int* err_code);

/**
 * Send all mails to been saved in Offline-mode.
 *
 * @param[in] account_id			Specifies the account ID.
 * @param[in] sending_option 	Specifies the sending option.
 * @param[in] callback				Specifies the callback function for retrieving sending information.
 * @param[in] handle					Specifies the handle for stopping sending.
 * @param[out] err_code			Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_send_mail_saved(int account_id, emf_option_t* sending_option,  unsigned* handle, int* err_code);

INTERNAL_FUNC int emdaemon_add_mail(emf_mail_data_t *input_mail_data, emf_attachment_data_t *input_attachment_data_list, int input_attachment_count, emf_meeting_request_t *input_meeting_request, int input_from_eas);

INTERNAL_FUNC int emdaemon_add_meeting_request(int account_id, char *mailbox_name, emf_meeting_request_t *meeting_req, int* err_code);

/**
 * Delete a mail or multiple mails.
 *
 * @param[in] account_id        Specifies the account id.
 * @param[in] mailbox			Specifies the mailbox.
 * @param[in] mail_id			Specifies the arrary of mail id.
 * @param[in] num				Specifies the number of mail id.
 * @param[in] from_server	    Specifies whether mails are deleted from server.
 * @param[in] callback			Specifies the callback function for retrieving deleting information.
 * @param[in] handle				Reserved.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */

INTERNAL_FUNC int emdaemon_delete_mail(int account_id, emf_mailbox_t* mailbox, int mail_id[], int num, int from_server,  unsigned* handle, int* err_code);

/**
 * Delete all mail from a mailbox.
 *
 * @param[in] mailbox			Specifies the structure of mailbox.
 * @param[in] with_server		Specifies whether mails are also deleted from server.
 * @param[in] callback			Specifies the callback function for delivering status during deleting.
 * @param[in] handle				Reserved.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_delete_mail_all(emf_mailbox_t* mailbox, int with_server,  unsigned* handle, int* err_code);

/**
 * Move a email to another mailbox.
 *
 * 
 * @param[in] mail_id		Specifies the mail ID.
 * @param[in] dst_mailbox	Specifies the mailbox structure for moving email.
 * @param[in] noti_param_1  Specifies first parameter of result notification.
 * @param[in] noti_param_2  Specifies second parameter of result notification.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_move_mail(int mail_ids[], int num, emf_mailbox_t* dst_mailbox, int noti_param_1, int noti_param_2, int* err_code);

/**
 * Move all email to another mailbox.
 *
 * 
 * @param[in] src_mailbox		Specifies the source mailbox structure for moving email.
 * @param[in] dst_mailbox		Specifies the destination mailbox structure for moving email.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_move_mail_all_mails(emf_mailbox_t* src_mailbox, emf_mailbox_t* dst_mailbox, int* err_code);


/**
 * Update a existing email information.
 *
 * @param[in] input_mail_data	Specifies the structure of mail data.
 * @param[in] input_attachment_data_list	Specifies the structure of mail data.
 * @param[in] input_attachment_count	Specifies the pointer of attachment structure.
 * @param[in] input_meeting_request	Specifies the number of attachment data.
 * @param[in] input_from_eas	Specifies whether the mail is from EAS.
 * @remarks N/A
 * @return This function returns EMF_ERROR_NONE on success or error code on failure.
 */
INTERNAL_FUNC int emdaemon_update_mail(emf_mail_data_t *input_mail_data, emf_attachment_data_t *input_attachment_data_list, int input_attachment_count, emf_meeting_request_t *input_meeting_request, int input_from_eas);


/**
 * Callback for mail resend
 *
 * @param[in] data		              Specifies the pointer to mail_id.
 * @remarks N/A
 * @return This function returns void.
 */
INTERNAL_FUNC void _OnMailSendRetryTimerCB( void* data );

/**
 * Download email body from server.
 *
 * @param[in] mailbox		Reserved.
 * @param[in] mail_id			Specifies the mail ID.
 * @param[in] callback		Specifies the callback function for retrieving download status.
 * @param[in] handle			Specifies the handle for stopping downloading.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_download_body(emf_mailbox_t* mailbox, int mail_id, int verbose, int with_attachment,  unsigned* handle, int* err_code);

/**
 * Get a mail attachment.
 *
 * @param[in] mailbox		Reserved.
 * @param[in] mail_id		Specifies the mail ID.
 * @param[in] attachment_id	Specifies the buffer that a attachment ID been saved.
 * @param[out] attachment	The returned attachment is save here.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_get_attachment(emf_mailbox_t* mailbox, int mail_id, char* attachment_id, emf_attachment_info_t** attachment, int* err_code);

/**
 * Download a email nth-attachment from server.
 *
 * @param[in] mailbox		Reserved.
 * @param[in] mail_id			Specifies the mail ID.
 * @param[in] nth				Specifies the buffer that a attachment number been saved. the minimum number is "1".
 * @param[in] callback		Specifies the callback function for retrieving download status.
 * @param[in] handle			Specifies the handle for stopping downloading.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_download_attachment(emf_mailbox_t* mailbox, int mail_id, char* nth,  unsigned* handle, int* err_code);


/**
 * Append a attachment to email.
 *
 * @param[in] mailbox		Reserved.
 * @param[in] mail_id		Specifies the mail ID.
 * @param[in] attachment	Specifies the structure of attachment.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_add_attachment(emf_mailbox_t* mailbox, int mail_id, emf_attachment_info_t* attachment, int* err_code);

/**
 * Delete a attachment from email.
 *
 * @param[in] mailbox		Reserved.
 * @param[in] mail_id		Specifies the mail ID.
 * @param[in] attachment_id	Specifies the buffer that a attachment ID been saved.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_delete_mail_attachment(emf_mailbox_t* mailbox, int mail_id, char* attachment_id, int* err_code);

/**
 * Free allocated memroy for email attachment.
 *
 * @param[in] atch_info	Specifies the pointer of mail attachment structure pointer.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_free_attachment_info(emf_attachment_info_t** atch_info, int* err_code);

/**
 * Change email flag.
 *
 * @param[in] mailbox	Reserved.
 * @param[in] mail_id	Specifies the mail ID.
 * @param[in] new_flag	Specifies the new email flag.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_modify_flag(int mail_id, emf_mail_flag_t new_flag, int onserver, int sticky_flag, int* err_code);

/**
 * Change email extra flag.
 *
 * @param[in] mailbox	Reserved.
 * @param[in] mail_id	Specifies the mail ID.
 * @param[in] new_flag	Specifies the new email extra flag.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_modify_extra_flag(int mail_id, emf_extra_flag_t new_flag, int* err_code);

/**
 * Change email read/unread flag.
 * @param[in] account_id  Specifies the account id.
 * @param[in] mail_ids		Specifies the array of mail ID.
 * @param[in] num			    Specifies the numbers of mail ID.
 * @param[in] field_type  Specifies the field type what you want to set. Refer emf_flags_field_type.
 * @param[in] value	      Specifies the value what you want to set. 
 * @param[in] onserver		Specifies the mail on server.
 * @param[out] err_code	  Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_set_flags_field(int account_id, int mail_ids[], int num, emf_flags_field_type field_type, int value, int onserver, int* err_code);

/*****************************************************************************/
/*  Mailbox                                                                  */
/*****************************************************************************/
INTERNAL_FUNC int emdaemon_get_imap_mailbox_list(int account_id, char* mailbox, unsigned* handle, int* err_code);

/**
 * Download header of new emails from mail server. 
 *
 * @param[in] mailbox		Specifies the structure of mailbox.
 * @param[in] callback		Specifies the callback function for retrieving download status.
 * @param[out] handle		Specifies the handle for stopping downloading.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_sync_header(emf_mailbox_t* mailbox, unsigned* handle, int* err_code);


/**
 * Get mail count from mailbox.
 *
 * @param[in] mailbox	Specifies the pointer of mailbox structure.
 * @param[out] total	Total email count is saved here.
 * @param[out] unseen	Unread email count is saved here.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_get_mail_count_of_mailbox(emf_mailbox_t* mailbox, int* total, int* unseen, int* err_code);

/**
 * Get all mailboxes from account.
 *
 * @param[in] account_id		Specifies the account ID.
 * @param[out] mailbox_list	Specifies the pointer of mailbox structure pointer.(possibly NULL)
 * @param[out] count			The mailbox count is saved here.(possibly 0)
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_get_mailbox_list(int account_id, emf_mailbox_t** mailbox_list, int* count, int* err_code);

/**
 * Create a new mailbox.
 *
 * @param[in] new_mailbox	Specifies the pointer of creating mailbox information.
 * @param[out] handle		Specifies the handle for stopping creating mailbox.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_add_mailbox(emf_mailbox_t* new_mailbox, int on_server, unsigned* handle, int* err_code);

/**
 * Delete a mailbox.
 *
 * @param[in] mailbox	Specifies the pointer of deleting mailbox information.
 * @param[out] handle		Specifies the handle for stopping creating mailbox.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_delete_mailbox(emf_mailbox_t* mailbox, int on_server, unsigned* handle, int* err_code);

/**
 * Delete all sub-mailboxes from a specific mailbox.
 *
 * @param[in] mailbox	Specifies the pointer of mailbox information.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_delete_mailbox_all(emf_mailbox_t* mailbox, int* err_code);

/**
 * Free allocated memory for mailbox information.
 *
 * @param[in] mailbox_list	Specifies the pointer for searching mailbox structure pointer.
 * @param[in] count			Specifies the count of mailboxes.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_free_mailbox(emf_mailbox_t** mailbox_list, int count, int* err_code);

/*****************************************************************************/
/*  Etc                                                                      */
/*****************************************************************************/


/**
 * Register a callback for event processing.
 *
 * @param[in] action			Kind of event callback.
 * @param[in] callback		Function which will be called during processing event.
 * @param[in] event_data	Event data which will be passed to the callback.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_register_event_callback(emf_action_t action, emf_event_callback callback, void* event_data);

/**
 * Unregister a callback for event processing.
 *
 * @param[in] action			Kind of event callback.
 * @param[in] callback		Function which will be called during processing event.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_unregister_event_callback(emf_action_t action, emf_event_callback callback);

/**
 * Get current event queue status.
 *
 * @param[out] on_sending		True if sending is in progress.
 * @param[out] on_receiving		True if receiving is in progress.
 * @remarks N/A
 */
INTERNAL_FUNC void emdaemon_get_event_queue_status(int* on_sending, int* on_receiving);

/**
 * Get the handle of a pending job.
 *
 * @param[in] action			Specifies kind of the job.
 * @param[in] account_id	Specifies the account ID.
 * @param[in] mail_id			Specifies the mail ID.
 * @remarks N/A
 * @return This function return its handle if a pending job exists, otherwise -1.
 */
INTERNAL_FUNC int emdaemon_get_pending_job(emf_action_t action, int account_id, int mail_id, emf_event_status_type_t* status);

/**
 * Cancel a progressive work.
 *
 * @param[in] account_id		Specifies the account ID.
 * @param[in] handle				Specifies the ID of cancelling work.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_cancel_job(int account_id, int handle, int* err_code);



/**
 * Cancel a progressive send mail job.
 *
 * @param[in] account_id		Specifies the account ID.
 * @param[in] mail_id				Specifies the Mail ID of cancelling send mail.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_cancel_sending_mail_job(int account_id, int mail_id, int* err_code);

/**
 * set email options.
 *
 * @param[in] option			Specifies the email options.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_set_option(emf_option_t* option, int* err_code);

/**
 * get email options.
 *
 * @param[out] option			Specifies the email options.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_get_option(emf_option_t* option, int* err_code);

/**
 * Sync the Local activity 
 *
 * 
 * @param[in] account_id		Specifies the Account ID.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emdaemon_sync_local_activity(int account_id, int *err_code);


#ifdef __FEATURE_AUTO_POLLING__
INTERNAL_FUNC int emdaemon_start_auto_polling(int* err_code);
#endif /* __FEATURE_AUTO_POLLING__ */

INTERNAL_FUNC int emdaemon_insert_accountinfo_to_contact(emf_account_t* account);

INTERNAL_FUNC int emdaemon_update_accountinfo_to_contact(emf_account_t* old_account, emf_account_t* new_account);

INTERNAL_FUNC int emdaemon_update_mailbox(emf_mailbox_t* old_mailbox, emf_mailbox_t* new_mailbox, int on_server, unsigned* handle, int* err_code);

INTERNAL_FUNC int emdaemon_search_mail_on_server(int input_account_id, const char *input_mailbox_name, email_search_filter_t *input_search_filter, int input_search_filter_count, unsigned int *output_handle);

INTERNAL_FUNC int emdaemon_clear_all_mail_data(int* err_code);

INTERNAL_FUNC int emdaemon_send_mail_retry( int mail_id,  int timeout_in_sec, int* err_code);

INTERNAL_FUNC int emdaemon_validate_account_and_create(emf_account_t* new_account, unsigned* handle, int* err_code);

INTERNAL_FUNC int emdaemon_set_mail_slot_size_of_mailbox(int account_id, char* mailbox_name, int new_slot_size, unsigned* handle, int *err_code);

INTERNAL_FUNC int emdaemon_move_mail_thread_to_mailbox(int thread_id, char *target_mailbox_name, int move_always_flag, int *err_code);

INTERNAL_FUNC int emdaemon_delete_mail_thread(int thread_id, int delete_always_flag, unsigned* handle, int *err_code);

INTERNAL_FUNC int emdaemon_modify_seen_flag_of_thread(int thread_id, int seen_flag, int on_server, unsigned* handle, int *err_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @} @}
*/

#endif /* __EMAIL_DAEMON_H__ */
