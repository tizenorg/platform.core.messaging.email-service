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


#ifndef __EMAIL_API_MAIL_H__
#define __EMAIL_API_MAIL_H__

#include "email-types.h"

#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/**
* @defgroup EMAIL_SERVICE Email Service
* @{
*/


/**
* @ingroup EMAIL_SERVICE
* @defgroup EMAIL_API_MAIL Email Mail API
* @{
*/

/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with email-service.
 * @file		email-api-mail.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 		This file contains the data structures and interfaces of Messages provided by
 *			email-service .
 */



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @open
 * @fn email_add_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas)
 * @brief	Save a mail. This function is invoked when user wants to add a mail.
 * 		If the option from_eas is 1 then this will save the message on server as well as on locally.
 * 		If the incoming_server_type is EMAIL_SERVER_TYPE_POP3 then from_eas value will be 0
 * 		If the incoming_server_type is EMAIL_SERVER_TYPE_IMAP4 then from_eas value will be 1/0
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_mail_data	Specifies the saving mail.
 * @param[in] input_attachment_data_list	Specifies the mailbox structure for saving email.
 * @param[in] input_attachment_count	Specifies if the mail comes from composer.
 * @param[in] input_meeting_request	Specifies if the mail comes from composer.
 * @param[in] input_from_eas	Specifies if the mail will be saved on server.

 * @exception 	none
 * @see email_mail_data_t and email_mailbox_t
 * @remarks N/A
 * @code
 * #include "email-api-mail.h"
 * int _test_add_mail ()
 * {
 * 	int                    i = 0;
 * 	int                    account_id = 0;
 * 	int                    from_eas = 0;
 * 	int                    attachment_count = 0;
 * 	int                    err = EMAIL_ERROR_NONE;
 * 	char                   arg[50] = { 0 , };
 * 	char                  *body_file_path = "/opt/data/email/.emfdata/tmp/mail.txt";
 * 	email_mailbox_t         *mailbox_data = NULL;
 * 	email_mail_data_t       *test_mail_data = NULL;
 * 	email_attachment_data_t *attachment_data = NULL;
 * 	email_meeting_request_t *meeting_req = NULL;
 * 	FILE                  *body_file;
 *
 * 	printf("\n > Enter account id : ");
 * 	scanf("%d", &account_id);
 *
 *
 * 	memset(arg, 0x00, 50);
 * 	printf("\n > Enter mailbox name : ");
 * 	scanf("%s", arg);
 *
 * 	email_get_mailbox_by_name(account_id, arg, &mailbox_data);
 *
 * 	test_mail_data = malloc(sizeof(email_mail_data_t));
 * 	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));
 *
 * 	printf("\n From EAS? [0/1]> ");
 * 	scanf("%d", &from_eas);
 *
 * 	test_mail_data->account_id        = account_id;
 * 	test_mail_data->save_status       = 1;
 * 	test_mail_data->flags_seen_field  = 1;
 * 	test_mail_data->file_path_plain   = strdup(body_file_path);
 * 	test_mail_data->mailbox_id        = mailbox_data->mailbox_id;
 * 	test_mail_data->mailbox_type      = mailbox_data->mailbox_type;
 * 	test_mail_data->full_address_from = strdup("<test1@test.com>");
 * 	test_mail_data->full_address_to   = strdup("<test2@test.com>");
 * 	test_mail_data->full_address_cc   = strdup("<test3@test.com>");
 * 	test_mail_data->full_address_bcc  = strdup("<test4@test.com>");
 * 	test_mail_data->subject           = strdup("Meeting request mail");
 *
 * 	body_file = fopen(body_file_path, "w");
 *
 * 	for(i = 0; i < 500; i++)
 * 		fprintf(body_file, "X2 X2 X2 X2 X2 X2 X2");
 * 	fflush(body_file);
 *   fclose(body_file);
 *
 * 	printf(" > Attach file? [0/1] : ");
 * 	scanf("%d",&attachment_count);
 *
 * 	if ( attachment_count )  {
 * 		memset(arg, 0x00, 50);
 * 		printf("\n > Enter attachment name : ");
 * 		scanf("%s", arg);
 *
 * 		attachment_data = malloc(sizeof(email_attachment_data_t));
 *
 * 		attachment_data->attachment_name  = strdup(arg);
 *
 * 		memset(arg, 0x00, 50);
 * 		printf("\n > Enter attachment absolute path : ");
 * 		scanf("%s",arg);
 *
 * 		attachment_data->attachment_path  = strdup(arg);
 * 		attachment_data->save_status      = 1;
 * 		test_mail_data->attachment_count  = attachment_count;
 * 	}
 *
 * 	printf("\n > Meeting Request? [0: no, 1: yes (request from server), 2: yes (response from local)]");
 * 	scanf("%d", &(test_mail_data->meeting_request_status));
 *
 * 	if ( test_mail_data->meeting_request_status == 1
 * 		|| test_mail_data->meeting_request_status == 2 ) {
 * 		time_t current_time;
 * 		meeting_req = malloc(sizeof(email_meeting_request_t));
 * 		memset(meeting_req, 0x00, sizeof(email_meeting_request_t));
 *
 * 		meeting_req->meeting_response     = 1;
 * 		current_time = time(NULL);
 * 		gmtime_r(&current_time, &(meeting_req->start_time));
 * 		gmtime_r(&current_time, &(meeting_req->end_time));
 * 		meeting_req->location = malloc(strlen("Seoul") + 1);
 * 		memset(meeting_req->location, 0x00, strlen("Seoul") + 1);
 * 		strcpy(meeting_req->location, "Seoul");
 * 		strcpy(meeting_req->global_object_id, "abcdef12345");
 *
 * 		meeting_req->time_zone.offset_from_GMT = 9;
 * 		strcpy(meeting_req->time_zone.standard_name, "STANDARD_NAME");
 * 		gmtime_r(&current_time, &(meeting_req->time_zone.standard_time_start_date));
 * 		meeting_req->time_zone.standard_bias = 3;
 *
 * 		strcpy(meeting_req->time_zone.daylight_name, "DAYLIGHT_NAME");
 * 		gmtime_r(&current_time, &(meeting_req->time_zone.daylight_time_start_date));
 * 		meeting_req->time_zone.daylight_bias = 7;
 *
 * 	}
 *
 * 	if((err = email_add_mail(test_mail_data, attachment_data, attachment_count, meeting_req, from_eas)) != EMAIL_ERROR_NONE)
 * 		printf("email_add_mail failed. [%d]\n", err);
 * 	else
 * 		printf("email_add_mail success.\n");
 *
 * 	if(attachment_data)
 * 		email_free_attachment_data(&attachment_data, attachment_count);
 *
 * 	if(meeting_req)
 * 		email_free_meeting_request(&meeting_req, 1);
 *
 * 	email_free_mail_data(&test_mail_data, 1);
 * 	email_free_mailbox(&mailbox_data, 1);
 *
 * 	printf("saved mail id = [%d]\n", test_mail_data->mail_id);
 *
 * 	return 0;
 * }
 * @endcode
 */
EXPORT_API int email_add_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas);

 /**
  * @open
  * @fn email_add_read_receipt(int input_read_mail_id,  int *output_handle)
  * @brief	Add a read receipt mail. This function is invoked when user receives a mail with read report enable and wants to send a read report for the same.
  *
  * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
  * @param[in]  input_read_mail_id	Specifies the id of mail which has been read.
  * @param[out] output_receipt_mail_id	Specifies the receipt mail id .
  * @exception none
  * @see
  * @remarks N/A
  */
EXPORT_API int email_add_read_receipt(int input_read_mail_id, int *output_receipt_mail_id);

/**
 * @open
 * @fn email_update_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_composer)
 * @brief	Update a existing email information. This function is invoked when user wants to change some existing email information with new email information.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_mail_data		Specifies the mail ID.
 * @param[in] input_attachment_data_list	Specifies the pointer of attachment data.
 * @param[in] input_attachment_count	Specifies the number of attachment data.
 * @param[in] input_meeting_request	Specifies the meeting request data.
 * @param[in] input_from_eas Specifies whether sync server.

 * @exception 	none
 * @see 	email_mail_data_t
 * @code
 * #include "email-api-account.h"
 * int email_test_update_mail()
 * {
 * 	int                    mail_id = 0;
 * 	int                    err = EMAIL_ERROR_NONE;
 * 	int                    test_attachment_data_count = 0;
 * 	char                   arg[50];
 * 	email_mail_data_t       *test_mail_data = NULL;
 * 	email_attachment_data_t *test_attachment_data_list = NULL;
 * 	email_meeting_request_t *meeting_req = NULL;
 *
 * 	printf("\n > Enter mail id : ");
 * 	scanf("%d", &mail_id);
 *
 * 	email_get_mail_data(mail_id, &test_mail_data);
 *
 * 	printf("\n > Enter Subject: ");
 * 	scanf("%s", arg);
 *
 * 	test_mail_data->subject= strdup(arg);
 *
 *
 *
 * 	if (test_mail_data->attachment_count > 0) {
 * 		if ( (err = email_get_attachment_data_list(mail_id, &test_attachment_data_list, &test_attachment_data_count)) != EMAIL_ERROR_NONE ) {
 * 			printf("email_get_meeting_request() failed [%d]\n", err);
 * 			return -1;
 * 		}
 * 	}
 *
 * 	if ( test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_REQUEST
 * 		|| test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_RESPONSE
 * 		|| test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
 *
 * 		if ( (err = email_get_meeting_request(mail_id, &meeting_req)) != EMAIL_ERROR_NONE ) {
 * 			printf("email_get_meeting_request() failed [%d]\n", err);
 * 			return -1;
 * 		}
 *
 * 		printf("\n > Enter meeting response: ");
 * 		scanf("%d", (int*)&(meeting_req->meeting_response));
 * 	}
 *
 * 	if ( (err = email_update_mail(test_mail_data, test_attachment_data_list, test_attachment_data_count, meeting_req, 0)) != EMAIL_ERROR_NONE)
 * 			printf("email_update_mail failed.[%d]\n", err);
 * 		else
 * 			printf("email_update_mail success\n");
 *
 * 	if(test_mail_data)
 * 		email_free_mail_data(&test_mail_data, 1);
 *
 * 	if(test_attachment_data_list)
 * 		email_free_attachment_data(&test_attachment_data_list, test_attachment_data_count);
 *
 * 	if(meeting_req)
 * 		email_free_meeting_request(&meeting_req, 1);
 *
 * 	return 0;
 * }

 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_update_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas);

/**
 * @open
 * @fn email_count_mail(email_mailbox_t* mailbox, int* total, int* unseen)
 * @brief	Get mail count from mailbox.This function is invoked when user wants to know how many toatl mails and out of that
 * 		how many unseen mails are there in a given mailbox.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mailbox	Specifies the pointer of mailbox structure.
 * @param[out] total	Total email count is saved here.
 * @param[out] unseen	Unread email count is saved here.
 * @exception 	none
 * @see 	email_mailbox_t

 * @code
 * 	#include "email-api-account.h"
 *	bool
 *	_api_sample_count_mail()
 *	{
 *		int total = 0;
 *		int unseen = 0;
 * 		email_list_filter_t *filter_list = NULL;
 * 		int err = EMAIL_ERROR_NONE;
 * 		int i = 0;
 * 		filter_list = malloc(sizeof(email_list_filter_t) * 3);
 * 		memset(filter_list, 0 , sizeof(email_list_filter_t) * 3);
 *
 * 		filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * 		filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
 * 		filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * 		filter_list[0].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * 		filter_list[0].list_filter_item.rule.case_sensitivity              = false;
 *
 * 		filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
 *		filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;
 *
 *		filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 *		filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_TO;
 *		filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 *		filter_list[2].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 *		filter_list[2].list_filter_item.rule.case_sensitivity              = false;
 *
 *		if(EMAIL_ERROR_NONE == email_count_mail(filter_list, 3, &total, &unseen))
 *			printf("\n Total: %d, Unseen: %d \n", total, unseen);
 *	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_count_mail(email_list_filter_t *input_filter_list, int input_filter_count, int *output_total_mail_count, int *output_unseen_mail_count);

/**
 * @open
 * @fn email_get_max_mail_count(int *Count)
 * @brief	Gets the max count of mails which can be supported by email-service
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[out] Count 	max count of mails which can be supported by email-service
 * @exception 		none
 * @code
 *    	#include "email-api-account.h"
 *   		bool
 *  	 	_api_sample_get_max_mail_count()
 *   		{
 *   			int max_count = -1;
 *
 *   			if(EMAIL_ERROR_NONE == email_get_max_mail_count(&max_count))
 *   				printf("\n\t>>>>> email_get_max_mail_count() return [%d]\n\n", max_count);
 *    	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_get_max_mail_count(int *count);

/**
 * @open
 * @fn 	email_delete_mail(email_mailbox_t* mailbox, int *mail_ids, int num, int from_server)
 * @brief	Delete a mail or multiple mails.Based on from_server value this function will delte a mail or multiple mails from server or loaclly.
 * @param[in] mailbox			Reserved.
 * 		If the incoming_server_type is EMAIL_SERVER_TYPE_POP3 then from_server value will be 0
 * 		If the incoming_server_type is EMAIL_SERVER_TYPE_IMAP4 then from_server value will be 1/0
 *
 * @param[in] mail_ids[]				Specifies the arrary of mail id.
 * @param[in] num					Specifies the number of mail id.
 * @param[in] from_server	Specifies whether mails are deleted from server.
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @exception 		none
 * @see 		email_mailbox_t
 * @code
 * 	#include "email-api-account.h"
 *	bool
 *	_api_sample_delete_mail()
 *	{
 *		int count, i, mail_id=0, mailbox_id =0;
 *
 *		printf("\n > Enter Mail_id: ");
 *		scanf("%d",&mail_id);
 *		printf("\n > Enter Mailbox ID: ");
 *		scanf("%d",&mailbox_id);
 *		if(EMAIL_ERROR_NONE == email_delete_mail(mailbox_id, &mail_id, 1, 1))
 *			printf("\n email_delete_mail success");
 *		else
 *			printf("\n email_delete_mail failed");
 *	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_delete_mail(int input_mailbox_id, int *input_mail_ids, int input_num, int input_from_server);


/**
 * @open
 * @fn email_delete_all_mails_in_mailbox(int input_mailbox_id, int input_from_server)
 * @brief	Delete all mail from a mailbox.
 * 		If the incoming_server_type is EMAIL_SERVER_TYPE_POP3 then from_server value will be 0
 * 		If the incoming_server_type is EMAIL_SERVER_TYPE_IMAP4 then from_server value will be 1/0
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mailbox			Specifies the structure of mailbox.
 * @param[in] from_server		Specifies whether mails are also deleted from server.
 * @exception 		none
 * @see 		email_mailbox_t

 * @code
 * 	#include "email-api-account.h"
 *	bool
 *	_api_sample_delete_all_mails_in_mailbox()
 *	{
 *		int count, mailbox_id =0;
 *
 *		printf("\n > Enter mailbox_id: ");
 *		scanf("%d",&mailbox_id);
 *
 * 		if (EMAIL_ERROR_NONE != email_delete_all_mails_in_mailbox(mailbox_id, 0))
 * 			printf("email_delete_all_mails_in_mailbox failed");
 * 		else
 * 			printf("email_delete_all_mails_in_mailbox Success");
 *	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_delete_all_mails_in_mailbox(int input_mailbox_id, int input_from_server);

/**
 * @open
 *  @fn email_clear_mail_data()
 * @brief	delete email data from storage. This API will be used by the Settings Application
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @exception 		none
 * @see 		none

 * @code
 * 	#include "email-api-account.h"
 *	bool
 *	_api_sample_clear_mail_data()
 *	{
 *		if(EMAIL_ERROR_NONE == email_clear_mail_data())
 *			//success
 *		else
 *			//failure
 *
 *	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int  email_clear_mail_data();


/**
 * @open
 * @fn email_add_attachment(int mail_id, email_attachment_data_t* attachment)
 * @brief	Append a attachment to email.This function is invoked when user wants to add attachment to existing mail.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id		Specifies the mail ID.
 * @param[in] attachment	Specifies the structure of attachment.
 * @exception 		none
 * @see email_attachment_data_t

 * @code
 * 	#include "email-api-account.h"
 *	bool
 *	_api_sample_mail_add_attachment()
 *	{
 *		int mail_id = 0;
 *		email_attachment_data_t attachment;
 *
 *		printf("\n > Enter Mail Id: ");
 *		scanf("%d", &mail_id);
 *		memset(&attachment, 0x00, sizeof(email_attachment_data_t));
 *		printf("\n > Enter attachment name: ");
 *		attachment.name = strdup("Test");
 *		printf("\n > Enter attachment absolute path: ");
 *		attachment.savename = strdup("/tmp/test.txt");
 *		attachment.next = NULL;
 *		if(EMAIL_ERROR_NONE != email_add_attachment(mail_id, &attachment))
 *			printf("email_add_attachment failed\n");
 *		else
 *			printf(email_add_attachment success\n");
 *	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_add_attachment(int mail_id, email_attachment_data_t* attachment);


/**
 * @open
 * @fn email_delete_attachment(int mail_id, const char * attachment_id)
 * @brief	delete a attachment from email.This function is invoked when user wants to delete a attachment from existing mailbased on mail Id and attachment Id
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id			Specifies the mail ID.
 * @param[in] attachment_id	 	Specifies the attachment id.
 * @exception 		none
 * @see 		email_mailbox_t
 * @code
 * 	#include "email-api-account.h"
 *	bool
 *	_api_sample_mail_delete_attachment()
 *	{
 *
 *		if(EMAIL_ERROR_NONE != email_delete_attachment(1))
 *			//failure
 *		else
 *			//success
 *	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_delete_attachment(int attachment_id);

/**
 * @open
 * @fn	email_get_attachment_data(int mail_id, const char* attachment_id, email_attachment_data_t** attachment)
 * @brief	Get a mail attachment.This function is invoked when user wants to get the attachment information based on attachment id for the specified mail Id.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id		Specifies the mail ID.
 * @param[in] attachment_id	Specifies the buffer that a attachment ID been saved.
 * @param[out] attachment	The returned attachment is save here.
 * @exception 		none
 * @see 		email_mailbox_t and email_mail_attachment_info_t

 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_mail_get_attachment_info()
 *   	{
 *		email_mail_attachment_info_t *mail_attach_info = NULL;
 *		int mail_id = 0;
 *		char arg[10];
 *		int err = EMAIL_ERROR_NONE;
 *
 *
 *		printf("\n > Enter Mail Id: ");
 *		scanf("%d", &mail_id);
 *		printf("> attachment Id\n");
 *		scanf("%s",arg);
 *		if (EMAIL_ERROR_NONE != email_get_attachment_data(mail_id, &mail_attach_info))
 *			printf("email_get_attachment_data failed\n");
 *		else
 *		{
 *			printf("email_get_attachment_data SUCCESS\n");
 *			//do something
 *			email_free_attachment_data(&mail_attach_info,1);
 *		}
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_get_attachment_data(int attachment_id, email_attachment_data_t** attachment);

EXPORT_API int email_get_attachment_data_list(int input_mail_id, email_attachment_data_t **output_attachment_data, int *output_attachment_count);

/**
 * @open
 * @fn email_free_attachment_data(email_attachment_data_t **attachment_data_list, int attachment_data_count)
 * @brief	Free allocated memroy for email attachment.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] atch_info	Specifies the pointer of mail attachment structure pointer.
 * @exception 		none
 * @see 		email_mail_attachment_info_t

 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_mail_free_attachment_info()
 *   	{
 *		email_mailbox_t mailbox;
 *		email_mail_attachment_info_t *mail_attach_info = NULL;
 *		int mail_id = 0,account_id = 0;
 *		char arg[10];
 *		int err = EMAIL_ERROR_NONE;
 *
 *		memset(&mailbox, 0x00, sizeof(email_mailbox_t));
 *
 *		printf("\n > Enter Mail Id: ");
 *		scanf("%d", &mail_id);
 *		printf("\n > Enter account Id: ");
 *		scanf("%d", &account_id);
 *		printf("> attachment Id\n");
 *		scanf("%s",arg);
 *		mailbox.account_id = account_id;
 *		if (EMAIL_ERROR_NONE != email_get_attachment_data(&mailbox, mail_id, &mail_attach_info))
 *			printf("email_get_attachment_data failed\n");
 *		else
 *		{
 *			printf("email_get_attachment_data SUCCESS\n");
 *			//do something
 *			email_free_attachment_info(&mail_attach_info,1);
 *		}
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_free_attachment_data(email_attachment_data_t **attachment_data_list, int attachment_data_count);

/* -----------------------------------------------------------
					      Mail Information API
    -----------------------------------------------------------*/


/**
 * @open
 * @fn email_query_mails(char *conditional_clause_string, email_mail_data_t** mail_list,  int *result_count)
 * @brief                          Query the mail list information from DB based on the mailbox name.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] conditional_clause_string  Specifies the where clause string.
 * @param[in/out] mail_list        Specifies the pointer to the structure email_mail_data_t.
 * @param[in/out] result_count     Specifies the number of mails returned.
 * @exception                      None
 * @see                            email_mail_list_item_t

 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_query_mail()
 *   	{
 *		email_mail_data_t *mail_list = NULL;
 *		char conditional_clause_string[500];
 *		int result_count = 0;
 *
 *		memset(conditional_clause_string, 0x00, 10);
 *		printf("\n > Enter where clause: ");
 *		scanf("%s", conditional_clause_string);
 *
 *
 *		if (EMAIL_ERROR_NONE != email_query_mails(conditional_clause_string, &mail_list, &result_count))
 *			printf("email_query_mails failed \n");
 *		else {
 *			printf("Success\n");
 *			//do something
 *			free(mail_list);
 *		}
 *
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_query_mails(char *conditional_clause_string, email_mail_data_t** mail_list,  int *result_count);

/**
 * @open
 * @fn email_query_mail_list(char *input_conditional_clause_string, email_mail_list_item_t** output_mail_list,  int *output_result_count)
 * @brief Query the mail list information from DB.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_conditional_clause_string	Specifies the where clause string.
 * @param[in/out] output_mail_list				Specifies the pointer to the structure email_mail_list_item_t.
 * @param[in/out] output_result_count			Specifies the number of mails returned.
 * @exception	None
 * @see			email_mail_list_item_t

 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_query_mail_list()
 *   	{
 *		email_mail_list_item_t *mail_list = NULL;
 *		char conditional_clause_string[500];
 *		int result_count = 0;
 *
 *		memset(conditional_clause_string, 0x00, 10);
 *		printf("\n > Enter where clause: ");
 *		scanf("%s", conditional_clause_string);
 *
 *
 *		if (EMAIL_ERROR_NONE != email_query_mail_list(conditional_clause_string, &mail_list, &result_count))
 *			printf("email_query_mail_list failed \n");
 *		else {
 *			printf("Success\n");
 *			//do something
 *			free(mail_list);
 *		}
 *
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_query_mail_list(char *input_conditional_clause_string, email_mail_list_item_t** output_mail_list,  int *output_result_count);

/**
 * @open
 * @fn email_get_mail_data(int input_mail_id, email_mail_data_t **output_mail_data)
 * @brief	Get a mail by its mail id. This function is invoked when user wants to get a mail based on mail id existing in DB.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_mail_id	specifies the mail id.
 * @param[out] output_mail_data		The returned mail is save here.
 * @exception 		none
 * @see	email_mail_data_t

 * @code
 *    #include "email-api-account.h"
 *   	bool
 *   	_api_sample_get_mail_data()
 *   	{
 *		email_mail_data_t *mail = NULL;
 *		int mail_id = 0 ;
 *		int err = EMAIL_ERROR_NONE;
 *
 *		printf("\n > Enter mail id: ");
 *		scanf("%d", &mail_id);
 *
 *		if (EMAIL_ERROR_NONE != email_get_mail_data(mail_id, &mail))
 *			printf("email_get_mail_data failed\n");
 *		else
 *		{
 *			printf("email_get_mail_data SUCCESS\n");
 *			//do something
 *			email_free_mail_data(&mail,1);
 *		}
 *   	}
 * @endcode
 * @remarks N/A
 */

EXPORT_API int email_get_mail_data(int input_mail_id, email_mail_data_t **output_mail_data);


/**
 * @open
 * @fn email_free_mail_data(email_mail_data_t** mail_list, int count)
 * @brief	Free allocated memroy for emails.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_list	Specifies the pointer of mail structure pointer.
 * @param[in] count		Specifies the count of mails.
 * @exception 		none
 * @see                 email_mail_data_t

 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_free_mail()
 *   	{
 *		email_mail_data_t *mail;
 *
 *		//fill the mail structure
 *		//count - number of mail structure user want to free
 *		 if(EMAIL_ERROR_NONE == email_free_mail_data(&mail,count))
 *		 	//success
 *		 else
 *		 	//failure
 *
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_free_mail_data(email_mail_data_t** mail_list, int count);

/* -----------------------------------------------------------
					      Mail Flag API
    -----------------------------------------------------------*/

/**
 * @open
 * @fn email_set_flags_field(int *mail_ids, int num, email_flags_field_type field_type, int value, int onserver)
 * @brief	Change email flags field.
 *      	If the incoming_server_type is EMAIL_SERVER_TYPE_POP3 then from_server value will be 0
 *     		If the incoming_server_type is EMAIL_SERVER_TYPE_IMAP4 then from_server value will be 1/0
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] account_id  Specifies account ID.
 * @param[in] mail_ids		Specifies the array of mail ID.
 * @param[in] num 		Specifies the number of mail ID.
 * @param[in] field_type  Specifies the field type what you want to set. Refer email_flags_field_type.
 * @param[in] value	      Specifies the value what you want to set.
 * @param[in] onserver		Specifies whether mail Flag updation in server
 * @exception 		none
 * @see 		none
 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_set_flags_field()
 *   	{
 *   		int account_id = 0;
 *			int mail_id = 0;
 *			int err = EMAIL_ERROR_NONE;
 *
 *			printf("\n > Enter account id: ");
 *			scanf("%d", &account_id);
 *
 *			printf("\n > Enter mail id: ");
 *			scanf("%d", &mail_id);
 *			if (EMAIL_ERROR_NONE != email_set_flags_field(&account_id, &mail_id, EMAIL_FLAGS_SEEN_FIELD, 1, 0))
 *				printf("email_set_flags_field failed\n");
 *			else
 *			{
 *				printf("email_set_flags_field succeed\n");
 *				//do something
 *			}
 *   	}
 * @endcode
 * @remarks N/A
 */

EXPORT_API int email_set_flags_field(int account_id, int *mail_ids, int num, email_flags_field_type field_type, int value, int onserver);

/* -----------------------------------------------------------
					      Mail Move API
    -----------------------------------------------------------*/

/**
 * @open
 * @fn email_move_mail_to_mailbox(int *mail_ids, int num, int input_target_mailbox_id)
 * @brief	Move a email to another mailbox.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id					Specifies the array of mail ID.
 * @param[in] num						Specifies the count of mail IDs.
 * @param[in] input_target_mailbox_id	Specifies the mailbox ID for moving email.
 * @exception 		none
 * @see 		email_mailbox_t

 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_move_mail_to_mailbox()
 *   	{
 *		int mail_id = 0;
 *      int mailbox_id = 0;
 *		int err = EMAIL_ERROR_NONE;

 *		printf("\n > Enter mail_id: ");
 *		scanf("%d",&mail_id);
 *
 *		printf("\n > Enter target mailbox_id: ");
 *		scanf("%d",&mailbox_id);
 *
 *		if(EMAIL_ERROR_NONE == email_move_mail_to_mailbox(&mail_id,	1, mailbox_id))
 *			printf("Success\n");
 *
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_move_mail_to_mailbox(int *mail_ids, int num, int input_target_mailbox_id);


/**
 * @open
 * @fn email_move_all_mails_to_mailbox(int input_source_mailbox_id, int input_target_mailbox_id)
 * @brief	Move all email to another mailbox.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_source_mailbox_id	Specifies the source mailbox ID for moving email.
 * @param[in] input_target_mailbox_id	Specifies the destination mailbox ID for moving email.
 * @exception 		none
 * @see 		email_mailbox_t

 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_move_all_mails_to_mailbox()
 *   	{
 *   	int src_mailbox_id;
 *   	int dst_mailbox_id;
 *		int total_count;
 *		int err = EMAIL_ERROR_NONE;
 *		char temp[128];
 *
 *
 *		// input mailbox information : need  account_id and name (src & dest)
 *		printf("src mail maibox id> ");
 *		scanf("%d", &src_mailbox_id);
 *
 *		// Destination mailbox
 *		printf("dest mailbox id> ");
 *		scanf("%d", &dst_mailbox_id);
 *
 *		if( EMAIL_ERROR_NONE == email_move_all_mails_to_mailbox(src_mailbox_id, dst_mailbox_id))
 *		{
 *			printf("  fail email_move_all_mails_to_mailbox: \n");
 *		}
 *		else
 *			//success
 * }
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_move_all_mails_to_mailbox(int input_source_mailbox_id, int input_target_mailbox_id);


/**
 * @open
 * @fn email_get_mail_list_ex(email_list_filter_t *input_filter_list, int input_filter_count, email_list_sorting_rule_t *input_sorting_rule_list, int input_sorting_rule_count, int input_start_index, int input_limit_count, email_mail_list_item_t** output_mail_list, int *output_result_count)
 * @brief	Get the Mail List information from DB.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] input_filter_list			Specifies the filter list
 * @param[in] input_filter_count		Specifies the number of filter
 * @param[in] input_sorting_rule_list	Specifies the sorting rule list.
 * @param[in] input_sorting_rule_count	Specifies the number of sorting rule
 * @param[in] input_start_index			Specifies start index for LIMIT clause of SQL query.
 * @param[in] input_limit_count			Specifies limit count for LIMIT clause of SQL query.
 * @param[in/out] output_mail_list		Specifies the pointer to the structure email_mail_list_item_t.
 * @param[in/out] output_result_count	Specifies the number of mails returned.
 * @exception 		none
 * @see             email_mail_list_item_t
 * @code
 * email_list_filter_t *filter_list = NULL;
 * email_list_sorting_rule_t *sorting_rule_list = NULL;
 * email_mail_list_item_t *result_mail_list = NULL;
 * int result_mail_count = 0;
 * int err = EMAIL_ERROR_NONE;
 * int i = 0;
 * filter_list = malloc(sizeof(email_list_filter_t) * 9);
 * memset(filter_list, 0 , sizeof(email_list_filter_t) * 9);
 *
 * filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
 * filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * filter_list[0].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * filter_list[0].list_filter_item.rule.case_sensitivity              = false;
 *
 * filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
 * filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;
 *
 * filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_TO;
 * filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * filter_list[2].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * filter_list[2].list_filter_item.rule.case_sensitivity              = false;
 *
 * filter_list[3].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
 * filter_list[3].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;
 *
 * filter_list[4].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * filter_list[4].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_CC;
 * filter_list[4].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * filter_list[4].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * filter_list[4].list_filter_item.rule.case_sensitivity              = false;
 *
 * filter_list[5].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
 * filter_list[5].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;
 *
 * filter_list[6].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * filter_list[6].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_BCC;
 * filter_list[6].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * filter_list[6].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * filter_list[6].list_filter_item.rule.case_sensitivity              = false;
 *
 * filter_list[7].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
 * filter_list[7].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;
 *
 * filter_list[8].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * filter_list[8].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_FROM;
 * filter_list[8].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * filter_list[8].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * filter_list[8].list_filter_item.rule.case_sensitivity              = false;
 *
 * sorting_rule_list = malloc(sizeof(email_list_sorting_rule_t) * 2);
 * memset(sorting_rule_list, 0 , sizeof(email_list_sorting_rule_t) * 2);
 *
 * sorting_rule_list[0].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD;
 * sorting_rule_list[0].sort_order                                    = EMAIL_SORT_ORDER_ASCEND;
 *
 * sorting_rule_list[1].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_DATE_TIME;
 * sorting_rule_list[1].sort_order                                    = EMAIL_SORT_ORDER_ASCEND;
 *
 * err = email_get_mail_list_ex(filter_list, 9, sorting_rule_list, 2, -1, -1, &result_mail_list, &result_mail_count);
 *
 * if(err == EMAIL_ERROR_NONE) {
 * printf("email_get_mail_list_ex succeed.\n");
 *
 * for(i = 0; i < result_mail_count; i++) {
 * 	printf("mail_id [%d], subject [%s], from [%s]\n", result_mail_list[i].mail_id, result_mail_list[i].subject, result_mail_list[i].from);
 * 	}
 * }
 * else {
 * 	printf("email_get_mail_list_ex failed.\n");
 * }
 *
 * email_free_list_filter(&filter_list, 9);
 *
 * return FALSE;
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_get_mail_list_ex(email_list_filter_t *input_filter_list, int input_filter_count, email_list_sorting_rule_t *input_sorting_rule_list, int input_sorting_rule_count, int input_start_index, int input_limit_count, email_mail_list_item_t** output_mail_list, int *output_result_count);

EXPORT_API int email_free_list_filter(email_list_filter_t **input_filter_list, int input_filter_count);

/**
 * @open
 * @fn email_get_mails(int account_id , int mailbox_id, int thread_id, int start_index, int limit_count, email_sort_type_t sorting, email_mail_data_t** mail_list,  int* result_count)
 * @brief	Get the Mail List information from DB based on the mailbox name.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] account_id		Specifies the account ID
 * @param[in] mailbox_id		Specifies the mailbox id
 * @param[in] thread_id   		Specifies the thread id. It can be EMAIL_LIST_TYPE_THREAD, EMAIL_LIST_TYPE_NORMAL or thread id.
 * @param[in] start_index   	Specifies start index for LIMIT clause of SQL query.
 * @param[in] limit_count   	Specifies limit count for LIMIT clause of SQL query.
 * @param[in] sorting       	Specifies the sorting type.
 * @param[in/out] mail_list		Specifies the pointer to the structure email_mail_data_t.
 * @param[in/out] result_count	Specifies the number of mails returned.
 * @exception 		none
 * @see                 email_mail_data_t

 * @code
 *    #include "email-api-account.h"
 *   	bool
 *   	_api_sample_get_mail()
 *   	{
 *		email_mail_data_t *mail_list = NULL;
 *		int mailbox_id;
 *		int result_count = 0;
 *		int account_id = 0;
 *		int err_code = EMAIL_ERROR_NONE;
 *
 *		printf("\n > Enter Mailbox id: ");
 *		scanf("%d", mailbox_id);
 *
 *		printf("\n > Enter Account_id: ");
 *		scanf("%d",&account_id);
 *
 *		if (EMAIL_ERROR_NONE != email_get_mails(account_id, mailbox_id, EMAIL_LIST_TYPE_NORMAL, 0, 100, EMAIL_SORT_DATETIME_HIGH,  &mail_list, &result_count)) {
 *			printf("email_get_mails failed \n");
 *		}
 *		else {
 *			printf("Success\n");
 *			//do something
 *			free(mail_list);
 *		}
 *
 *   	}
 * @endcode
 * @remarks N/A
 */

EXPORT_API int email_get_mails(int account_id , int mailbox_id, int thread_id, int start_index, int limit_count, email_sort_type_t sorting, email_mail_data_t** mail_list,  int* result_count);

/**
 * @open
 * @fn email_get_mail_list(int account_id, int mailbox_id, int thread_id, int start_index, int limit_count, email_sort_type_t sorting, email_mail_list_item_t** mail_list,  int* result_count)
 * @brief	Get the Mail List information from DB based on the mailbox name.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] account_id		Specifies the account id
 * @param[in] mailbox_id		Specifies the mailbox id
 * @param[in] thread_id   		Specifies the thread id. It can be EMAIL_LIST_TYPE_THREAD, EMAIL_LIST_TYPE_NORMAL or thread id.
 * @param[in] start_index   	Specifies start index for LIMIT clause of SQL query.
 * @param[in] limit_count   	Specifies limit count for LIMIT clause of SQL query.
 * @param[in] sorting       	Specifies the sorting type.
 * @param[in/out] mail_list		Specifies the pointer to the structure email_mail_list_item_t.
 * @param[in/out] result_count	Specifies the number of mails returned.
 * @exception 		none
 * @see                 email_mail_list_item_t

 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_get_mail()
 *   	{
 *		email_mail_list_item_t *mail_list = NULL;
 *		int mailbox_id;
 *		int result_count = 0;
 *		int account_id = 0;
 *		int err_code = EMAIL_ERROR_NONE;
 *
 *		printf("\n > Enter Mailbox id: ");
 *		scanf("%d", mailbox_id);
 *
 *		printf("\n > Enter Account_id: ");
 *		scanf("%d",&account_id);
 *
 *		if (EMAIL_ERROR_NONE != email_get_mail_list(account_id, mailbox_id, EMAIL_LIST_TYPE_NORMAL, 0, 100, EMAIL_SORT_DATETIME_HIGH,  &mail_list, &result_count))
 *		{
 *			printf("email_get_mail_list_ex failed \n");
 *		}
 *		else
 *		{
 *			printf("Success\n");
 *			//do something
 *			free(mail_list);
 *		}
 *
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_get_mail_list(int account_id, int mailbox_id, int thread_id, int start_index, int limit_count, email_sort_type_t sorting, email_mail_list_item_t** mail_list,  int* result_count);

/**
 * @open
 * @fn 	email_get_mail_by_address(int account_id, int mailbox_id, email_email_address_list_t* addr_list,
									int start_index, int limit_count, email_sort_type_t sorting, email_mail_list_item_t** mail_list,  int* result_count)
 * @brief	Get the Mail List information from the DB based on the mailbox name account_id and sender address.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] account_id	Specifies the Account ID
 * @param[in] mailbox_id	Specifies the Mailbox ID
 * @param[in] addr_list		Specifies the addresses of senders. see email_email_address_list_t
 * @param[in] start_index		Specifies the first mail index of searched mail. This function will return mails whose index in the result list are from start_index to start_index + limit_count
 * @param[in] limit_count		Specifies the max number of returned mails.
 * @param[in] search_type   Specifies the search type.
 * @param[in] search_value  Specifies the search value.
 * @param[in] sorting                   Specifies the sorting order. see email_sort_type_t
 * @param[in/out] mail_list	Specifies the Pointer to the structure email_mail_list_item_t.
 * @param[in/out] result_count		Specifies the Number of searched Mails
 * @exception 		none
 * @see                 email_email_address_list_t, email_sort_type_t, email_mail_list_item_t
 * @code
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_get_mail_by_address(int account_id , int mailbox_id, email_email_address_list_t* addr_list,
									int start_index, int limit_count, int search_type, const char *search_value, email_sort_type_t sorting, email_mail_list_item_t** mail_list,  int* result_count);

/**
 * @open
 * @fn email_get_thread_information_by_thread_id(int thread_id, email_mail_data_t** thread_info)
 * @brief	Get the thread information for specific thread from DB based on the mailbox name.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] thread_id   	    Specifies the thread_id
 * @param[in/out] thread_info	Specifies the Pointer to the structure email_mail_data_t.
 * @exception 		none
 * @see             email_mail_data_t
 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_get_thread_information()
 *   	{
 *		email_mail_data_t *thread_info = NULL;
 *		int thread_id = 0;
 *		int err_code = EMAIL_ERROR_NONE;
 *
 *		printf("\n > Enter thread_id: ");
 *		scanf("%d",&thread_id);
 *
 *		if ( EMAIL_ERROR_NONE != email_get_thread_information_by_thread_id(thread_id, &thread_info))
 *		{
 *			printf("email_get_thread_information_by_thread_id failed :\n"); *
 *		}
 *		else
 *		{
 *			printf("Success\n");
 *			//do something
 *		}
 *
 *   	}
 * @endcode
 * @remarks N/A
 */


EXPORT_API int email_get_thread_information_by_thread_id(int thread_id, email_mail_data_t** thread_info);

/**
 * @open
 * @fn email_get_thread_information_ex(int thread_id, email_mail_list_item_t** thread_info)
 * @brief	Get the Mail List information for specific thread from DB based on the mailbox name.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] thread_id   	    Specifies the thread_id
 * @param[in/out] thread_info	Specifies the Pointer to the structure email_mail_list_item_t.
 * @exception 		none
 * @see             email_mail_list_item_t
 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_get_thread_information_ex()
 *   	{
 *		email_mail_list_item_t *thread_info = NULL;
 *		int thread_id = 0;
 *		int err_code = EMAIL_ERROR_NONE;
 *
 *		printf("\n > Enter thread_id: ");
 *		scanf("%d",&thread_id);
 *
 *		if ( EMAIL_ERROR_NONE != email_get_thread_information_ex(thread_id, &thread_info))
 *		{
 *			printf("email_get_mail_list_of_thread failed :\n"); *
 *		}
 *		else
 *		{
 *			printf("Success\n");
 *			//do something
 *		}
 *
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_get_thread_information_ex(int thread_id, email_mail_list_item_t** thread_info);



/**
 * @open
 * @fn email_retry_sending_mail( int mail_id, int timeout_in_sec)
 * @brief	Retry mail send
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id			Specifies the mail id for the mail to resend
 * @param[in] timeout_in_sec	Specifies the timeout value in seconds
 * @exception 		none
 * @see                 none

 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_retry_send_mail()
 *   	{
 *		int mail_id = 1;
 *		int timeout_in_sec = 2;
 *
 *		 if(EMAIL_ERROR_NONE == email_retry_sending_mail(mail_id,timeout_in_sec))
 *		 	//success
 *		 else
 *		 	//failure
 *
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_retry_sending_mail(int mail_id, int timeout_in_sec);

EXPORT_API int email_create_db_full();

/**
 * @open
 * @fn email_cancel_sending_mail( int mail_id)
 * @brief	Callback function for cm popup. We set the status as EMAIL_MAIL_STATUS_SEND_CANCELED
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id		Specifies the mailID
 * @exception 		none
 * @see                 none
 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_cancel_send_mail()
 *   	{
 *   		int mail_id = 10;
 *
 *
 *		 if(EMAIL_ERROR_NONE == email_cancel_sending_mail(mail_id,))
 *		 	//success
 *		 else
 *		 	//failure
 *
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_cancel_sending_mail(int mail_id) ;

 /**
 * @open
 * @fn email_get_disk_space_usage(unsigned long *total_size)
 * @brief	Gets the total disk usage of emails in KB.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[out] total_size 	total disk usage of emails
 * @exception 		none
 * @code
 *    	#include "email-api-account.h"
 *   		bool
 *   		_api_sample_get_disk_space_usage()
 *   		{
 *   			unsigned long total_size = 0;
 *
 *   			if ( EMAIL_ERROR_NONE  != email_get_disk_space_usage(&total_size))
 *   				printf("email_get_disk_space_usage failed err : %d\n",err_code);
 *   			else
 *	   			printf("email_get_disk_space_usage SUCCESS, total disk usage in KB : %ld \n", total_size);
 * 		}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_get_disk_space_usage(unsigned long *total_size);

/**
 * @open
 * @fn 	email_get_address_info_list(int mail_id, email_address_info_list_t** address_info_list)
 * @brief	Get the address info list. The address info list contains from, to, cc, bcc addresses and their display name, contact id and etc. (see email_address_info_list_t)
 *			Each GList (from, to, cc, bcc) is the list of email_address_info_t data.
 *			"data" variable of GList structure contains email_address_info_t data. <br>
 *			To get email_address_info_t data from GList, Use type casting from GList node.
 *
 * @return 	This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id   	Specifies the mail id to get the list.
 * @param[out] address_info_list	Specifies the Pointer to the structure email_address_info_list_t. Memory for a new address info list will be allocated to this. Call email_free_address_info_list() to free the memory allocated to this.
 * @exception 		none
 * @see                 email_address_info_list_t, email_address_info_t, email_free_address_info_list()
 * @code
 *    	#include "email-api-account.h"
 *   	bool
 *   	_api_sample_get_address_info_list()
 *   	{
 *		email_address_info_list_t *p_address_info_list= NULL;
 *		email_address_info_t *p_address_info = NULL;
 *		int mai_id;
 *		int err_code = EMAIL_ERROR_NONE;
 *
 *		mail_id = 1;
 *
 *		if ( EMAIL_ERROR_NONE != (err_code = email_get_address_info_list(mail_id, &p_address_info_list)) )
 *		{
 *			printf("email_get_address_info_list failed :\n"); *
 * 			return false;
 *		}
 *		else
 *		{
 *			printf("Success\n");
 *			//do something with p_address_info_list
 *			GList *list = p_address_info_list->from;
 *			GList *node = g_list_first(list);
 *			while ( node != NULL )
 *			{
 *				p_address_info = (email_address_info_t*)node->data;
 *				printf("%d,  %s, %s, %d\n", p_address_info->address_type, p_address_info->address, p_address_info->display_name, p_address_info->contact_id);
 *
 *				node = g_list_next(node);
 *			}
 *		}
 *
 *		// Free sender list
 *		if ( p_address_info_list )
 * 		{
 *			email_free_address_info_list(&p_address_info_list);
 * 		}
 *   	}
 * @endcode
 * @remarks N/A
 */
EXPORT_API int email_get_address_info_list(int mail_id, email_address_info_list_t** address_info_list);

/**
 * @open
 * @fn	email_free_address_info_list(email_address_info_list_t **address_info_list)
 * @brief	Free the address info list allocated by email_get_address_info_list(). This function will free the memory which is allocated to address_info_list itself.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] address_info_list	Specifies the Pointer to the structure email_address_info_list_t to be freed.
 * @exception 		none
 * @see                 email_address_info_list_t, email_get_address_info_list()
 */
EXPORT_API int email_free_address_info_list(email_address_info_list_t **address_info_list);

/**
 * @open
 * @fn	email_get_structure(const char*encoded_string, void **struct_var, email_convert_struct_type_e type)
 * @brief	This function returns the structure of the type which is indicated by 'type' variable. This function will allocate new memory to 'struct_var' for structure.<br>
 *			Some notifications such as NOTI_DOWNLOAD_NEW_MAIL are published with string parameter. The string contains various values that might be divided by delimiter.<br>
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] encoded_string		Specifies the Pointer to the string from notification.
 * @param[out] struct_var			Specifies the Pointer to the structure to be returned. If success, new memory for structure will be allocated.
 * @param[in] type				Specifies the converting type. see email_convert_struct_type_e
 * @exception 		none
 * @see                 email_convert_struct_type_e
 */
EXPORT_API int email_get_structure(const char*encoded_string, void **struct_var, email_convert_struct_type_e type);

/**
 * @open
 * @fn email_get_meeting_request(int mail_id, email_meeting_request_t **meeting_req)
 * @brief	Get the information of meeting request.  The information of meeting request is based on Mail Id. <br>
 *			The information of meeting request is corresponding to only one mail.
 *			For this reason, the information of meeting request can be added by using email_add_message_with_meeting_request() with a matched mail information.
 *
 * @return This function returns EMAIL_ERROR_NONE on success. This function returns EMAIL_ERROR_DATA_NOT_FOUND if there isn't a matched mail. Otherwise it returns error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] mail_id		Specifies the Mail id for which  meeting request details need to be fetched
 * @param[in/out] meeting_req	Specifies the Pointer to the structure email_meeting_request_t.
 * @exception 		none
 * @see                 email_meeting_request_t
 */
EXPORT_API int email_get_meeting_request(int mail_id, email_meeting_request_t **meeting_req);


/**
 * @open
 * @fn	email_free_meeting_request(email_meeting_request_t** meeting_req, int count)
 * @brief	Free the meeting request allocated by email_get_meeting_request() or alloacted to add. This function will free the memory which is allocated to meeting_req (= *meeting_req) itself.
 *
 * @return This function returns EMAIL_ERROR_NONE on success or error code (refer to EMAIL_ERROR_XXX) on failure.
 * @param[in] email_meeting_request_t	Specifies the Pointer to the structure email_meeting_request_t to be freed.
 * @param[in] count	Specifies the number of elements in meeting_req array. this is usually 1.
 * @exception 		none
 * @see                 email_meeting_request_t, email_get_meeting_request()
 */
EXPORT_API int email_free_meeting_request(email_meeting_request_t** meeting_req, int count);

EXPORT_API int email_move_thread_to_mailbox(int thread_id, int target_mailbox_id, int move_always_flag);

EXPORT_API int email_delete_thread(int thread_id, int delete_always_flag);

EXPORT_API int email_modify_seen_flag_of_thread(int thread_id, int seen_flag, int on_server);

EXPORT_API int email_expunge_mails_deleted_flagged(int input_mailbox_id, int input_on_server, int *output_handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @} @}
*/


#endif /* __EMAIL_API_MAIL_H__ */
