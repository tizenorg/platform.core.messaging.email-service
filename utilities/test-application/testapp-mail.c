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



/* common header */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

/* open header */
#include <glib.h>
#include <time.h>

#include "email-api-account.h"
#include "email-api-network.h"
#include "email-api-mail.h"
#include "email-api-mailbox.h"
#include "email-api-etc.h"

/* internal header */
#include "testapp-utility.h"
#include "testapp-mail.h"
#include "email-core-utils.h"
#include "email-core-mime.h"

#define MAIL_TEMP_BODY "/tmp/mail.txt"

/*
static void testapp_test_print_sorting_menu()
{
   	testapp_print("   EMAIL_SORT_DATETIME_HIGH = 0\n");
   	testapp_print("   EMAIL_SORT_DATETIME_LOW = 1\n");	
   	testapp_print("   EMAIL_SORT_SENDER_HIGH = 2\n");
   	testapp_print("   EMAIL_SORT_SENDER_LOW = 3\n");   
   	testapp_print("   EMAIL_SORT_RCPT_HIGH = 4\n");
   	testapp_print("   EMAIL_SORT_RCPT_LOW = 5\n");    
   	testapp_print("   EMAIL_SORT_SUBJECT_HIGH = 6\n");
   	testapp_print("   EMAIL_SORT_SUBJECT_LOW = 7\n");   
   	testapp_print("   EMAIL_SORT_PRIORITY_HIGH = 8\n");
   	testapp_print("   EMAIL_SORT_PRIORITY_LOW = 9\n");   
   	testapp_print("   EMAIL_SORT_ATTACHMENT_HIGH = 10\n");
   	testapp_print("   EMAIL_SORT_ATTACHMENT_LOW = 11\n");   
   	testapp_print("   EMAIL_SORT_FAVORITE_HIGH = 12\n");
   	testapp_print("   EMAIL_SORT_FAVORITE_LOW = 13\n");   
}

static void testapp_test_print_mail_list_item(email_mail_list_item_t *mail_list_item, int count)
{
	int i;
	
	testapp_print("\n>>>>> Print mail list items: count[%d]\n", count);
	for (i=0; i< count; i++) {
		testapp_print("\n[%d]\n", i);
		testapp_print(" >>> Mailbox Name [ %s ] \n", mail_list_item[i].mailbox_name);
		testapp_print(" >>> Mail ID [ %d ] \n", mail_list_item[i].mail_id);
		testapp_print(" >>> Account ID [ %d ] \n", mail_list_item[i].account_id);
		if (  mail_list_item[i].from!= NULL ) {
			testapp_print(" >>> From [ %s ] \n", mail_list_item[i].from);
		}
		if (  mail_list_item[i].from_email_address != NULL ) {
			testapp_print(" >>> from_email_address [ %s ] \n", mail_list_item[i].from_email_address);
		}
		if (  mail_list_item[i].recipients!= NULL ) {
			testapp_print(" >>> recipients [ %s ] \n", mail_list_item[i].recipients);
		}
		if (  mail_list_item[i].subject != NULL ) {
			testapp_print(" >>> subject [ %s ] \n", mail_list_item[i].subject);
		}
		testapp_print(" >>> text_download_yn [ %d ] \n", mail_list_item[i].is_text_downloaded);
		testapp_print(" >>> date_time [ %d ] \n", mail_list_item[i].date_time);
		testapp_print(" >>> flags_seen_field [ %d ] \n", mail_list_item[i].flags_seen_field);
		testapp_print(" >>> priority [ %d ] \n", mail_list_item[i].priority);
		testapp_print(" >>> save_status [ %d ] \n", mail_list_item[i].save_status);
		testapp_print(" >>> lock [ %d ] \n", mail_list_item[i].is_locked);
		testapp_print(" >>> is_report_mail [ %d ] \n", mail_list_item[i].is_report_mail);
		testapp_print(" >>> recipients_count [ %d ] \n", mail_list_item[i].recipients_count);
		testapp_print(" >>> has_attachment [ %d ] \n", mail_list_item[i].has_attachment);
		testapp_print(" >>> has_drm_attachment [ %d ] \n", mail_list_item[i].has_drm_attachment);

		if (  mail_list_item[i].previewBodyText != NULL ) {
			testapp_print(" >>> previewBodyText [ %s ] \n", mail_list_item[i].previewBodyText);
		}

		testapp_print(" >>> thread_id [ %d ] \n", mail_list_item[i].thread_id);
		testapp_print(" >>> thread_item_count [ %d ] \n", mail_list_item[i].thread_item_count);
	}
}
*/

static gboolean testapp_test_add_mail (int *result_mail_id)
{
	int                    result_from_scanf = 0;
	int                    i = 0;
	int                    account_id = 0;
	int                    mailbox_id = 0;
	int                    from_eas = 0;
	int                    attachment_count = 0;
	int                    err = EMAIL_ERROR_NONE;
	int                    smime_type = 0;
	char                   arg[50] = { 0 , };
	const char            *body_file_path = MAIL_TEMP_BODY;
	email_mailbox_t         *mailbox_data = NULL;
	email_mail_data_t       *test_mail_data = NULL;
	email_attachment_data_t *attachment_data = NULL;
	email_meeting_request_t *meeting_req = NULL;
	FILE                  *body_file;
 
	testapp_print("\n > Enter account id : ");
	result_from_scanf = scanf("%d", &account_id);

	
	memset(arg, 0x00, 50);
	testapp_print("\n > Enter mailbox id : ");
	result_from_scanf = scanf("%d", &mailbox_id);

	email_get_mailbox_by_mailbox_id(mailbox_id, &mailbox_data);

	test_mail_data = malloc(sizeof(email_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));
	
	testapp_print("\n Sync server? [0/1]> ");
	result_from_scanf = scanf("%d", &from_eas);

	test_mail_data->account_id           = account_id;
	test_mail_data->save_status          = 1;
	test_mail_data->body_download_status = 1;
	test_mail_data->flags_seen_field     = 1;
	test_mail_data->file_path_plain      = strdup(body_file_path);
	test_mail_data->mailbox_id           = mailbox_id;
	test_mail_data->mailbox_type         = mailbox_data->mailbox_type;
	test_mail_data->full_address_from    = strdup("<test1@test.com>");
	test_mail_data->full_address_to      = strdup("<test2@test.com>");
	test_mail_data->full_address_cc      = strdup("<test3@test.com>");
	test_mail_data->full_address_bcc     = strdup("<test4@test.com>");
	test_mail_data->subject              = strdup("Meeting request mail");

	body_file = fopen(body_file_path, "w");

	testapp_print("\n body_file [%p]\n", body_file);

	if(body_file == NULL) {
		testapp_print("\n fopen [%s]failed\n", body_file_path);
		return FALSE;
	}
/*	
	for(i = 0; i < 500; i++)
		fprintf(body_file, "X2 X2 X2 X2 X2 X2 X2");
*/
	fprintf(body_file, "Hello world");
	fflush(body_file);
	fclose(body_file);

	testapp_print(" > Select smime? [0: Normal, 1: sign, 2: Encrpyt, 3: sing + encrypt] : ");
	result_from_scanf = scanf("%d", &smime_type);
	test_mail_data->smime_type = smime_type;
	
	testapp_print(" > How many file attachment? [>=0] : ");
	result_from_scanf = scanf("%d",&attachment_count);
	
	test_mail_data->attachment_count  = attachment_count;
	if ( attachment_count > 0 )
		attachment_data = calloc(attachment_count, sizeof(email_attachment_data_t));


	for ( i = 0; i < attachment_count ; i++ ) {
		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment name : ");
		result_from_scanf = scanf("%s", arg);

		attachment_data[i].attachment_name  = strdup(arg);
		
		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment absolute path : ");
		result_from_scanf = scanf("%s",arg);
		
		attachment_data[i].attachment_path  = strdup(arg);
		attachment_data[i].save_status      = 1;
		attachment_data[i].mailbox_id       = test_mail_data->mailbox_id;
	}
	
	testapp_print("\n > Meeting Request? [0: no, 1: yes (request from server), 2: yes (response from local)]");
	result_from_scanf = scanf("%d", &(test_mail_data->meeting_request_status));
	
	if ( test_mail_data->meeting_request_status == 1 
		|| test_mail_data->meeting_request_status == 2 ) {
		time_t current_time;
		/*  dummy data for meeting request */
		meeting_req = malloc(sizeof(email_meeting_request_t));
		memset(meeting_req, 0x00, sizeof(email_meeting_request_t));
		
		meeting_req->meeting_response     = 1;
		current_time = time(NULL);
		gmtime_r(&current_time, &(meeting_req->start_time));
		gmtime_r(&current_time, &(meeting_req->end_time));
		meeting_req->location = strdup("Seoul");
		meeting_req->global_object_id = strdup("abcdef12345");

		meeting_req->time_zone.offset_from_GMT = 9;
		strcpy(meeting_req->time_zone.standard_name, "STANDARD_NAME");
		gmtime_r(&current_time, &(meeting_req->time_zone.standard_time_start_date));
		meeting_req->time_zone.standard_bias = 3;
		
		strcpy(meeting_req->time_zone.daylight_name, "DAYLIGHT_NAME");
		gmtime_r(&current_time, &(meeting_req->time_zone.daylight_time_start_date));
		meeting_req->time_zone.daylight_bias = 7;

	}
	
	if((err = email_add_mail(test_mail_data, attachment_data, attachment_count, meeting_req, from_eas)) != EMAIL_ERROR_NONE)
		testapp_print("email_add_mail failed. [%d]\n", err);
	else
		testapp_print("email_add_mail success.\n");

	testapp_print("saved mail id = [%d]\n", test_mail_data->mail_id);

	if(result_mail_id)
		*result_mail_id = test_mail_data->mail_id;

	if(attachment_data)
		email_free_attachment_data(&attachment_data, attachment_count);
		
	if(meeting_req)
		email_free_meeting_request(&meeting_req, 1);
		
	email_free_mail_data(&test_mail_data, 1);
	email_free_mailbox(&mailbox_data, 1);

	return FALSE;
}

static gboolean testapp_test_update_mail()
{
	int                    result_from_scanf = 0;
	int                    mail_id = 0;
	int                    err = EMAIL_ERROR_NONE;
	int                    test_attachment_data_count = 0;
	char                   arg[50];
	email_mail_data_t       *test_mail_data = NULL;
	email_attachment_data_t *test_attachment_data_list = NULL;
	email_meeting_request_t *meeting_req = NULL;
	
	testapp_print("\n > Enter mail id : ");
	result_from_scanf = scanf("%d", &mail_id);

	email_get_mail_data(mail_id, &test_mail_data);

	testapp_print("\n > Enter Subject: ");
	result_from_scanf = scanf("%s", arg);

	test_mail_data->subject= strdup(arg);

	if (test_mail_data->attachment_count > 0) {
		if ( (err = email_get_attachment_data_list(mail_id, &test_attachment_data_list, &test_attachment_data_count)) != EMAIL_ERROR_NONE ) {
			testapp_print("email_get_meeting_request() failed [%d]\n", err);
			return FALSE;
		}
	}	

	if ( test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_REQUEST 
		|| test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_RESPONSE 
		|| test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		
		if ( (err = email_get_meeting_request(mail_id, &meeting_req)) != EMAIL_ERROR_NONE ) {
			testapp_print("email_get_meeting_request() failed [%d]\n", err);
			return FALSE;
		}
	
		testapp_print("\n > Enter meeting response: ");
		result_from_scanf = scanf("%d", (int*)&(meeting_req->meeting_response));
	}
	
	if ( (err = email_update_mail(test_mail_data, test_attachment_data_list, test_attachment_data_count, meeting_req, 0)) != EMAIL_ERROR_NONE) 
			testapp_print("email_update_mail failed.[%d]\n", err);
		else
			testapp_print("email_update_mail success\n");
		
	if(test_mail_data)
		email_free_mail_data(&test_mail_data, 1);
		
	if(test_attachment_data_list) 
		email_free_attachment_data(&test_attachment_data_list, test_attachment_data_count);
		
	if(meeting_req)
		email_free_meeting_request(&meeting_req, 1);

	return TRUE;
}

static gboolean testapp_test_get_mails()
{
	testapp_print("\n >>> testapp_test_get_mails : Entered \n");
	email_mail_data_t *mails = NULL, **mails_pointer = NULL;
	int mailbox_id = 0;
	int count = 0, i = 0;
	int account_id = 0;
	int start_index =0;
	int limit_count = 0;
	int sorting = 0;
	int err_code = EMAIL_ERROR_NONE;
	int to_get_count = 0;
	int is_for_thread_view = 0;
	int list_type;
	int result_from_scanf = 0;
	struct tm *temp_time_info;

	testapp_print("\n > Enter Account_id (0 = all accounts) : ");
	result_from_scanf = scanf("%d",&account_id);

	testapp_print("\n > Enter Mailbox id (0 = all mailboxes) :");
	result_from_scanf = scanf("%d", &mailbox_id);

	testapp_print("\n > Enter Sorting : ");
	result_from_scanf = scanf("%d",&sorting);

	testapp_print("\n > Enter Start index (starting at 0): ");
	result_from_scanf = scanf("%d",&start_index);

	testapp_print("\n > Enter max_count : ");
	result_from_scanf = scanf("%d",&limit_count);

	testapp_print("\n > For thread view : ");
	result_from_scanf = scanf("%d",&is_for_thread_view);

	testapp_print("\n > Mail count only (0:list 1:count): ");
	result_from_scanf = scanf("%d",&to_get_count);

	if(to_get_count)
		mails_pointer = NULL;
	else
		mails_pointer = &mails;

	if(is_for_thread_view == -2) {
		list_type = EMAIL_LIST_TYPE_LOCAL;
	}
	else if(is_for_thread_view == -1)
		list_type = EMAIL_LIST_TYPE_THREAD;
	else
		list_type = EMAIL_LIST_TYPE_NORMAL;

	/* Get mail list */
	if(mailbox_id == 0) {
		testapp_print("Calling email_get_mail_list for all mailbox.\n");
		err_code = email_get_mails(account_id, 0, list_type, start_index, limit_count, sorting, mails_pointer, &count);
		if ( err_code < 0)
			testapp_print("email_get_mails failed - err[%d]\n", err_code);
	}
	else {
		testapp_print("Calling email_get_mail_list for %d mailbox_id.\n", mailbox_id);
		err_code = email_get_mails(account_id, mailbox_id, list_type, start_index, limit_count, sorting,  mails_pointer, &count);
		if ( err_code < 0)
			testapp_print("email_get_mails failed - err[%d]\n", err_code);
	}
	testapp_print("email_get_mails >>>>>>count - %d\n",count);

	if (mails) {
		for (i=0; i< count; i++) {
			testapp_print("\n[%d]\n", i);
			testapp_print(" >>> mailbox_id [ %d ] \n", mails[i].mailbox_id);
			testapp_print(" >>> mail_id [ %d ] \n", mails[i].mail_id);
			testapp_print(" >>> account_id [ %d ] \n", mails[i].account_id);
			if (  mails[i].full_address_from != NULL )
				testapp_print(" >>> full_address_from [ %s ] \n", mails[i].full_address_from);
			if (  mails[i].full_address_to != NULL )
				testapp_print(" >>> recipients [ %s ] \n", mails[i].full_address_to);
			if (  mails[i].subject != NULL )
				testapp_print(" >>> subject [ %s ] \n", mails[i].subject);
			testapp_print(" >>> body_download_status [ %d ] \n", mails[i].body_download_status);
			temp_time_info = localtime(&mails[i].date_time);
			testapp_print(" >>> date_time [ %d/%d/%d %d:%d:%d] \n",
									temp_time_info->tm_year + 1900,
									temp_time_info->tm_mon+1,
									temp_time_info->tm_mday,
									temp_time_info->tm_hour,
									temp_time_info->tm_min,
									temp_time_info->tm_sec);
			testapp_print(" >>> flags_seen_field [ %d ] \n", mails[i].flags_seen_field);
			testapp_print(" >>> priority [ %d ] \n", mails[i].priority);
			testapp_print(" >>> save_status [ %d ] \n", mails[i].save_status);
			testapp_print(" >>> lock_status [ %d ] \n", mails[i].lock_status);
			testapp_print(" >>> attachment_count [ %d ] \n", mails[i].attachment_count);
			if (  mails[i].preview_text != NULL )
				testapp_print(" >>> previewBodyText [ %s ] \n", mails[i].preview_text);
		}
		free(mails);
	}

	testapp_print(" >>> testapp_test_get_mails : End \n");
	return 0;
}

#define TEMP_ARGUMENT_SIZE 4096

static gboolean testapp_test_mail_send (int *result_mail_id)
{
	int                    added_mail_id = 0;
	int                    err = EMAIL_ERROR_NONE;
	unsigned               handle = 0;
	email_option_t           option = { 0 };
	email_mailbox_t          mailbox_data = { 0 };
	email_mail_data_t       *result_mail_data = NULL;

	testapp_test_add_mail(&added_mail_id);
	
	if(result_mail_id) {
		email_get_mail_data(added_mail_id, &result_mail_data);

		memset(&mailbox_data, 0x00, sizeof(email_mailbox_t));
		mailbox_data.account_id   = result_mail_data->account_id;
		mailbox_data.mailbox_id   = result_mail_data->mailbox_id;
		option.keep_local_copy    = 1;

		testapp_print("Calling email_send_mail...\n");

		if( email_send_mail(added_mail_id, &option, &handle) < 0) {
			testapp_print("Sending failed[%d]\n", err);
		}
		else  {
			testapp_print("Start sending. handle[%d]\n", handle);
		}

		email_free_mail_data(&result_mail_data, 1);
	}

	if(result_mail_id)
		*result_mail_id = added_mail_id;

	return FALSE;
}

static gboolean testapp_test_get_mail_list()
{
	email_list_filter_t *filter_list = NULL;
	email_list_sorting_rule_t *sorting_rule_list = NULL;
	email_mail_list_item_t *result_mail_list = NULL;
	int result_mail_count = 0;
	int err = EMAIL_ERROR_NONE;
	int i = 0;

	filter_list = malloc(sizeof(email_list_filter_t) * 9);
	memset(filter_list, 0 , sizeof(email_list_filter_t) * 9);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
	filter_list[0].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
	filter_list[0].list_filter_item.rule.case_sensitivity              = false;

	filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;

	filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_TO;
	filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
	filter_list[2].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
	filter_list[2].list_filter_item.rule.case_sensitivity              = false;

	filter_list[3].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[3].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;

	filter_list[4].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[4].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_CC;
	filter_list[4].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
	filter_list[4].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
	filter_list[4].list_filter_item.rule.case_sensitivity              = false;

	filter_list[5].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[5].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;

	filter_list[6].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[6].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_BCC;
	filter_list[6].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
	filter_list[6].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
	filter_list[6].list_filter_item.rule.case_sensitivity              = false;

	filter_list[7].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[7].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;

	filter_list[8].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[8].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_FROM;
	filter_list[8].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
	filter_list[8].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
	filter_list[8].list_filter_item.rule.case_sensitivity              = false;

	/*
	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value  = 1;

	filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_AND;

	filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME;
	filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[2].list_filter_item.rule.key_value.string_type_value   = strdup("INBOX");
	filter_list[2].list_filter_item.rule.case_sensitivity              = true;
	*/

	sorting_rule_list = malloc(sizeof(email_list_sorting_rule_t) * 2);
	memset(sorting_rule_list, 0 , sizeof(email_list_sorting_rule_t) * 2);

	sorting_rule_list[0].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_ATTACHMENT_COUNT;
	sorting_rule_list[0].sort_order                                    = EMAIL_SORT_ORDER_ASCEND;
	sorting_rule_list[0].force_boolean_check                           = true;

	sorting_rule_list[1].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_DATE_TIME;
	sorting_rule_list[1].sort_order                                    = EMAIL_SORT_ORDER_ASCEND;

	err = email_get_mail_list_ex(filter_list, 9, sorting_rule_list, 2, -1, -1, &result_mail_list, &result_mail_count);

	if(err == EMAIL_ERROR_NONE) {
		testapp_print("email_get_mail_list_ex succeed.\n");

		for(i = 0; i < result_mail_count; i++) {
			testapp_print("mail_id [%d], subject [%s], from [%s]\n", result_mail_list[i].mail_id, result_mail_list[i].subject, result_mail_list[i].from);
		}
	}
	else {
		testapp_print("email_get_mail_list_ex failed.\n");
	}

	email_free_list_filter(&filter_list, 9);

	return FALSE;
}

static gboolean testapp_test_send_cancel ()
{
	int num = 0;
	int Y = 0;
	int i = 0;
	int j = 0;
	int *mailIdList = NULL;
	int mail_id = 0;
	int result_from_scanf = 0;

	testapp_print("\n > Enter total Number of mail  want to send: ");
	result_from_scanf = scanf("%d", &num);
	mailIdList = (int *)malloc(sizeof(int)*num);
	if(!mailIdList)
		return false ;
	
	for(i = 1;i <=num ; i++) {
		testapp_test_mail_send(&mail_id);
		
		testapp_print("mail_id[%d]",mail_id);

		mailIdList[i] = mail_id;
		testapp_print("mailIdList[%d][%d]",i,mailIdList[i]);

		mail_id = 0;
		testapp_print("\n > Do you want to cancel the send mail job '1' or '0': ");
		result_from_scanf = scanf("%d", &Y);
		if(Y == 1) {
			testapp_print("\n >Enter mail-id[1-%d] ",i);
				result_from_scanf = scanf("%d", &j);
			testapp_print("\n mailIdList[%d] ",mailIdList[j]);	
			if(email_cancel_sending_mail( mailIdList[j]) < 0)
				testapp_print("email_cancel_sending_mail failed..!");
			else
				testapp_print("email_cancel_sending_mail success..!");
		}
	}
	return FALSE;
}

static gboolean testapp_test_delete()
{
	int mail_id=0, account_id =0;
	int mailbox_id = 0;
	int err = EMAIL_ERROR_NONE;
	int from_server = 0;
	int result_from_scanf = 0;

	testapp_print("\n > Enter Account_id: ");
	result_from_scanf = scanf("%d", &account_id);

	testapp_print("\n > Enter Mail_id: ");
	result_from_scanf = scanf("%d", &mail_id);
	
	testapp_print("\n > Enter Mailbox id: ");
	result_from_scanf = scanf("%d", &mailbox_id);
	
	testapp_print("\n > Enter from_server: ");
	result_from_scanf = scanf("%d", &from_server);

	/* delete message */
	if( (err = email_delete_mail(mailbox_id, &mail_id, 1, from_server)) < 0)
		testapp_print("\n email_delete_mail failed[%d]\n", err);
	else
		testapp_print("\n email_delete_mail success\n");

	return FALSE;
}



static gboolean testapp_test_move()
{
	int mail_id[3];
	int i = 0;
	int mailbox_id = 0;
	int result_from_scanf = 0;
	
	for(i = 0; i< 3; i++) {
		testapp_print("\n > Enter mail_id: ");
		result_from_scanf = scanf("%d",&mail_id[i]);
	}
	
	testapp_print("\n > Enter mailbox_id: ");
	result_from_scanf = scanf("%d", &mailbox_id);

	/* move mail */
	email_move_mail_to_mailbox(mail_id, 3, mailbox_id);
	return FALSE;
}

static gboolean testapp_test_delete_all()
{
	int mailbox_id =0;
	int err = EMAIL_ERROR_NONE;
	int result_from_scanf = 0;

	testapp_print("\n > Enter mailbox_id: ");
	result_from_scanf = scanf("%d",&mailbox_id);
						
	/* delete all message */
	if ( (err = email_delete_all_mails_in_mailbox(mailbox_id, 0)) < 0)
		testapp_print("email_delete_all_mails_in_mailbox failed [%d]\n", err);
	else
		testapp_print("email_delete_all_mails_in_mailbox Success\n");	
															
	return FALSE;
}


static gboolean testapp_test_add_attachment()
{	
	int mail_id = 0;
	int result_from_scanf = 0;
	char arg[100];
	email_attachment_data_t attachment;
	
	testapp_print("\n > Enter Mail Id: ");
	result_from_scanf = scanf("%d", &mail_id);

	memset(&attachment, 0x00, sizeof(email_attachment_data_t));
	memset(arg, 0x00, 100);
	testapp_print("\n > Enter attachment name: ");
	result_from_scanf = scanf("%s",arg);
	
	attachment.attachment_name = strdup(arg);
	
	memset(arg, 0x00, 100);
	testapp_print("\n > Enter attachment absolute path: ");
	result_from_scanf = scanf("%s",arg);

	attachment.save_status = true;
	attachment.attachment_path = strdup(arg);
	if(email_add_attachment(mail_id, &attachment) < 0)
		testapp_print("email_add_attachment failed\n");	
	else
		testapp_print("email_add_attachment success\n");


	return FALSE;

}

static gboolean testapp_test_set_deleted_flag()
{
	int index = 0;
	int account_id = 0;
	int mail_ids[100] = { 0, };
	int temp_mail_id = 0;
	int err_code = EMAIL_ERROR_NONE;
	int result_from_scanf = 0;

	testapp_print("\n >>> Input target account id: ");
	result_from_scanf = scanf("%d", &account_id);

	do {
		testapp_print("\n >>> Input target mail id ( Input 0 to terminate ) [MAX = 100]: ");
		result_from_scanf = scanf("%d", &temp_mail_id);
		mail_ids[index++] = temp_mail_id;
	} while (temp_mail_id != 0);

	err_code = email_set_flags_field(account_id, mail_ids, index, EMAIL_FLAGS_DELETED_FIELD, 1, true);
	testapp_print("email_set_flags_field returns - err[%d]\n", err_code);

	return 0;
}

static gboolean testapp_test_expunge_mails_deleted_flagged()
{
	int mailbox_id = 0;
	int on_server = 0;
	int err_code = EMAIL_ERROR_NONE;
	unsigned handle = 0;
	int result_from_scanf = 0;

	testapp_print("\n >>> Input target mailbox id: ");
	result_from_scanf = scanf("%d", &mailbox_id);

	testapp_print("\n >>> Expunge on server?: ");
	result_from_scanf = scanf("%d", &on_server);

	err_code = email_expunge_mails_deleted_flagged(mailbox_id, on_server, &handle);

	testapp_print("email_expunge_mails_deleted_flagged returns - err[%d]\n", err_code);

	return 0;
}

static gboolean testapp_test_get_mail_list_ex()
{
	testapp_print("\n >>> testapp_test_get_mail_list_ex : Entered \n");
	email_mail_list_item_t *mail_list = NULL, **mail_list_pointer = NULL;
	int mailbox_id = 0;
	int count = 0, i = 0;
	int account_id = 0;
	int start_index =0;
	int limit_count = 0;
	int sorting = 0;
	int err_code = EMAIL_ERROR_NONE;
	int to_get_count = 0;
	int is_for_thread_view = 0;
	int list_type;
	struct tm *temp_time_info;
	int result_from_scanf = 0;

	testapp_print("\n > Enter Account_id (0 = all accounts) : ");
	result_from_scanf = scanf("%d",&account_id);

	testapp_print("\n > Enter Mailbox id (0 = all mailboxes) :");
	result_from_scanf = scanf("%d", &mailbox_id);

	testapp_print("\n > Enter Sorting : ");
	result_from_scanf = scanf("%d",&sorting);
	
	testapp_print("\n > Enter Start index (starting at 0): ");
	result_from_scanf = scanf("%d",&start_index);

	testapp_print("\n > Enter max_count : ");
	result_from_scanf = scanf("%d",&limit_count);

	testapp_print("\n > For thread view : ");
	result_from_scanf = scanf("%d",&is_for_thread_view);

	testapp_print("\n > Count mails?(1: YES):");
	result_from_scanf = scanf("%d",&to_get_count);

	if(to_get_count)
		mail_list_pointer = NULL;
	else
		mail_list_pointer = &mail_list;
	
	if(is_for_thread_view == -2) {
		list_type = EMAIL_LIST_TYPE_LOCAL;
 	}
	else if(is_for_thread_view == -1)
		list_type = EMAIL_LIST_TYPE_THREAD;
	else
		list_type = EMAIL_LIST_TYPE_NORMAL;
	
	/* Get mail list */
	if( mailbox_id == 0) {
		testapp_print("Calling email_get_mail_list for all mailbox.\n");
		err_code = email_get_mail_list(account_id, 0, list_type, start_index, limit_count, sorting,  mail_list_pointer, &count);
		if ( err_code < 0) 
			testapp_print("email_get_mail_list failed - err[%d]\n", err_code);
	}
	else {
		testapp_print("Calling email_get_mail_list for %d mailbox_id.\n", mailbox_id);
		err_code = email_get_mail_list(account_id, mailbox_id, list_type, start_index, limit_count, sorting,  mail_list_pointer, &count);
		if ( err_code < 0) 
			testapp_print("email_get_mail_list failed - err[%d]\n", err_code);
	}
	testapp_print("email_get_mail_list >>>>>>count - %d\n",count);

	if (mail_list) {
		for (i=0; i< count; i++) {
			testapp_print("\n[%d]\n", i);
			testapp_print(" >>> mailbox_id [ %d ] \n", mail_list[i].mailbox_id);
			testapp_print(" >>> mail_id [ %d ] \n", mail_list[i].mail_id);
			testapp_print(" >>> account_id [ %d ] \n", mail_list[i].account_id);
			if (  mail_list[i].from != NULL )
				testapp_print(" >>> From [ %s ] \n", mail_list[i].from);
			if (  mail_list[i].recipients != NULL )
				testapp_print(" >>> recipients [ %s ] \n", mail_list[i].recipients);
			if (  mail_list[i].subject != NULL )
				testapp_print(" >>> subject [ %s ] \n", mail_list[i].subject);
			testapp_print(" >>> is_text_downloaded [ %d ] \n", mail_list[i].is_text_downloaded);
			temp_time_info = localtime(&mail_list[i].date_time);
			testapp_print(" >>> date_time [ %d/%d/%d %d:%d:%d] \n",
							  	temp_time_info->tm_year + 1900,
							  	temp_time_info->tm_mon+1,
							  	temp_time_info->tm_mday,
							  	temp_time_info->tm_hour,
							  	temp_time_info->tm_min,
							  	temp_time_info->tm_sec);
			testapp_print(" >>> flags_seen_field [ %d ] \n", mail_list[i].flags_seen_field);
			testapp_print(" >>> priority [ %d ] \n", mail_list[i].priority);
			testapp_print(" >>> save_status [ %d ] \n", mail_list[i].save_status);
			testapp_print(" >>> is_locked [ %d ] \n", mail_list[i].is_locked);
			testapp_print(" >>> has_attachment [ %d ] \n", mail_list[i].has_attachment);
			if (  mail_list[i].previewBodyText != NULL )
				testapp_print(" >>> previewBodyText [ %s ] \n", mail_list[i].previewBodyText);
		}
		free(mail_list);
	}
	
	testapp_print("\n >>> email_get_mail_list : End \n");
	return 0;
}


static gboolean testapp_test_get_mail_list_for_thread_view()
{
	testapp_print(" >>> testapp_test_get_mail_list_for_thread_view : Entered \n");
	email_mail_list_item_t *mail_list = NULL;
	int count = 0, i = 0;
	int account_id = 0;
	int err_code = EMAIL_ERROR_NONE;
	struct tm *time_info;

	/* Get mail list */
	if ( email_get_mail_list(account_id, 0 , EMAIL_LIST_TYPE_THREAD, 0, 500, EMAIL_SORT_DATETIME_HIGH, &mail_list, &count) < 0)  {
		testapp_print("email_get_mail_list failed : %d\n",err_code);
		return FALSE;
	}
	testapp_print("email_get_mail_list >>>>>>count - %d\n",count);
	if (mail_list) {
		for (i=0; i< count; i++) {
			testapp_print(" i [%d]\n", i);
			testapp_print(" >>> Mail ID [ %d ] \n", mail_list[i].mail_id);
			testapp_print(" >>> Account ID [ %d ] \n", mail_list[i].account_id);
			testapp_print(" >>> Mailbox id [ %d ] \n", mail_list[i].mailbox_id);
			testapp_print(" >>> From [ %s ] \n", mail_list[i].from);
			testapp_print(" >>> recipients [ %s ] \n", mail_list[i].recipients);
			testapp_print(" >>> subject [ %s ] \n", mail_list[i].subject);
			testapp_print(" >>> is_text_downloaded [ %d ] \n", mail_list[i].is_text_downloaded);
			time_info = localtime(&mail_list[i].date_time);
			testapp_print(" >>> date_time [ %s ] \n", asctime(time_info));
			testapp_print(" >>> flags_seen_field [ %d ] \n", mail_list[i].flags_seen_field);
			testapp_print(" >>> priority [ %d ] \n", mail_list[i].priority);
			testapp_print(" >>> save_status [ %d ] \n", mail_list[i].save_status);
			testapp_print(" >>> lock [ %d ] \n", mail_list[i].is_locked);
			testapp_print(" >>> has_attachment [ %d ] \n", mail_list[i].has_attachment);
			testapp_print(" >>> previewBodyText [ %s ] \n", mail_list[i].previewBodyText);
			testapp_print(" >>> thread_id [ %d ] \n", mail_list[i].thread_id);
			testapp_print(" >>> thread__item_count [ %d ] \n", mail_list[i].thread_item_count);
		}
		free(mail_list);
	}	
	testapp_print(" >>> testapp_test_get_mail_list_for_thread_view : End \n");
	return 0;
}

static gboolean testapp_test_count ()
{
	int total = 0;
	int unseen = 0;
	email_list_filter_t *filter_list = NULL;

	filter_list = malloc(sizeof(email_list_filter_t) * 3);
	memset(filter_list, 0 , sizeof(email_list_filter_t) * 3);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
	filter_list[0].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
	filter_list[0].list_filter_item.rule.case_sensitivity              = false;

	filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;

	filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_TO;
	filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
	filter_list[2].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
	filter_list[2].list_filter_item.rule.case_sensitivity              = false;

	if(EMAIL_ERROR_NONE == email_count_mail(filter_list, 3, &total, &unseen))
		printf("\n Total: %d, Unseen: %d \n", total, unseen);
	return 0;
}

static gboolean	testapp_test_set_flags_field ()
{
	int account_id = 0;
	int mail_id = 0;
	int result_from_scanf = 0;

	testapp_print("\n > Enter Account ID: ");
	result_from_scanf = scanf("%d", &account_id);
	
	testapp_print("\n > Enter Mail ID: ");
	result_from_scanf = scanf("%d", &mail_id);

	if(email_set_flags_field(account_id, &mail_id, 1, EMAIL_FLAGS_FLAGGED_FIELD, 1, 1) < 0)
		testapp_print("email_set_flags_field failed");
	else
		testapp_print("email_set_flags_field success");
		
	return TRUE;
}

static gboolean testapp_test_download_body ()
{
	int mail_id = 0;
	unsigned handle = 0, err;
	int result_from_scanf = 0;

	testapp_print("\n > Enter Mail Id: ");
	result_from_scanf = scanf("%d", &mail_id);
	err = email_download_body(mail_id, 0, &handle);
	if(err  < 0)
		testapp_print("email_download_body failed");
	else {
		testapp_print("email_download_body success");
		testapp_print("handle[%d]\n", handle);
	}
	return TRUE;
}


static gboolean testapp_test_cancel_download_body ()
{
	int mail_id = 0;
	int account_id = 0;
	int yes = 0;
	unsigned handle = 0;
	int result_from_scanf = 0;
	
	email_mailbox_t mailbox;
	memset(&mailbox, 0x00, sizeof(email_mailbox_t));

	testapp_print("\n > Enter account_id: ");
	result_from_scanf = scanf("%d", &account_id);

	testapp_print("\n > Enter mail id: ");
	result_from_scanf = scanf("%d", &mail_id);
	
	if( email_download_body(mail_id, 0, &handle) < 0)
		testapp_print("email_download_body failed");
	else {
		testapp_print("email_download_body success\n");
		testapp_print("Do u want to cancel download job>>>>>1/0\n");
		result_from_scanf = scanf("%d",&yes);
		if(1 == yes) {
			if(email_cancel_job(account_id, handle , EMAIL_CANCELED_BY_USER) < 0)
				testapp_print("email_cancel_job failed..!");
			else {
				testapp_print("email_cancel_job success..!");
				testapp_print("handle[%d]\n", handle);
			}
		}
			
	}
	return TRUE;
}
static gboolean testapp_test_download_attachment ()
{
	int mail_id = 0;
	int attachment_no = 0;
	unsigned handle = 0;
	int result_from_scanf = 0;
	
	testapp_print("\n > Enter Mail Id: ");
	result_from_scanf = scanf("%d", &mail_id);

	testapp_print("\n > Enter attachment number: ");
	result_from_scanf = scanf("%d", &attachment_no);
	
	if( email_download_attachment(mail_id, attachment_no ,&handle) < 0)
		testapp_print("email_download_attachment failed");
	else {
		testapp_print("email_download_attachment success");
		testapp_print("handle[%d]\n", handle);
	}
	return TRUE;

}

static gboolean testapp_test_retry_send()
{
	int mail_id = 0;
	int timeout = 0;
	int result_from_scanf = 0;
	
	email_mailbox_t mailbox;
	memset(&mailbox, 0x00, sizeof(email_mailbox_t));

	testapp_print("\n > Enter Mail Id: ");
	result_from_scanf = scanf("%d", &mail_id);
	
	testapp_print("\n > Enter timeout in seconds: ");
	result_from_scanf = scanf("%d", &timeout);

	if( email_retry_sending_mail(mail_id, timeout) < 0)
		testapp_print("email_retry_sending_mail failed");		
	return TRUE;		
}

static gboolean testapp_test_move_all_mails_to_mailbox()
{
	int err = EMAIL_ERROR_NONE;
	int result_from_scanf = 0;
	int src_mailbox_id;
	int dest_mailbox_id;

	testapp_print("src mailbox id> ");
	result_from_scanf = scanf("%d", &src_mailbox_id);

	testapp_print("dest mailbox id> ");
	result_from_scanf = scanf("%d", &dest_mailbox_id);
	
	err = email_move_all_mails_to_mailbox(src_mailbox_id, dest_mailbox_id);
	if ( err < 0 ) {
		testapp_print("email_move_all_mails_to_mailbox failed: err[%d]\n", err);
	}
	else {
		testapp_print("email_move_all_mails_to_mailbox suceeded\n");
	}

	return false;
}

static gboolean testapp_test_get_totaldiskusage()
{
	unsigned long total_size = 0;
	int err_code = EMAIL_ERROR_NONE ;
	
	err_code = email_get_disk_space_usage(&total_size);
	if ( err_code < 0)
		testapp_print("email_get_disk_space_usage failed err : %d\n",err_code);
	else
		testapp_print("email_get_disk_space_usage SUCCESS, total disk usage in KB : %ld \n", total_size);		

	return FALSE;
}



static gboolean testapp_test_db_test()
{
	int err = EMAIL_ERROR_NONE;
	int result_from_scanf = 0;
	int mail_id;
	int account_id;
	char *to = NULL;
	char *cc = NULL;
	char *bcc = NULL;

	to = (char *) malloc(500000);
	cc = (char *) malloc(500000);
	bcc = (char *) malloc(500000);
	
	memset(to, 0x00, sizeof(to));
	memset(cc, 0x00, sizeof(to));
	memset(bcc, 0x00, sizeof(to));

	testapp_print("Input Mail id:\n");
	result_from_scanf = scanf("%d", &mail_id);
	testapp_print("Input Account id:\n");
	result_from_scanf = scanf("%d", &account_id);
	testapp_print("Input TO field:\n");
	result_from_scanf = scanf("%s", to);
	testapp_print("Input CC field:\n");
	result_from_scanf = scanf("%s", cc);
	testapp_print("Input BCC field:\n");
	result_from_scanf = scanf("%s", bcc);
	
	if ( emstorage_test(mail_id, account_id, to, cc, bcc, &err) == true ) {
		testapp_print(">> Saving Succeeded\n");
	}
	else {
		testapp_print(">> Saving Failed[%d]\n", err);
	}
	
	free(to);
	free(cc);
	free(bcc);
	
	return false;
}

static gboolean testapp_test_address_format_check_test()
{
	/*
	int i;
	int type;
	int index;
	int check_yn;
	int err = EMAIL_ERROR_NONE;
	char *pAddress = NULL;
	char input_address[8096];
	char *address_list[] = {
		"tom@gmail.com",
		"tom@gmail,com",
		"tom@gmail.com@gmail.com",
		"[tom@gmail.com]",
		"\"Tom\"<tom@gmail.com>", 
		"\"Tom\"[tom@gmail.com]",
		"\"Tom\"<tom,@gmail.com>",
		"<tom@gmail.com>",
		"<tom@gmail,com>",
		"<tom@gmail.>",
		"<tom@gmail.com>,tom@gmail.com",
		"<tom@gmail.com>;tom@gmail.com",
		"tom @gmail.com",
		"tom@ gmail.com",
		"tom@gmail..com",
		"tom@",
		"tom@gmail",
		"tom@gmail.",
		"tom@gmail.c",
		"tom@.",
		"\"\"Tom\"<tom,@gmail.com>",
		"tom@gmail.com,tom@gmail.com",
		"tom@gmail.com.",
		"<tom@gmail.com>,tom@.gmail.com",
		"&& tom@live.com ----",
		"madhu <tom@gmail.com>",
		"tom@gmail.com, && tom@live.com",
		"tom@gmail.com , && tom@live.com",
		"< tom@gmail.com    > , <           tom@.gmail.com     >",
		"tom@gmail.com ,           tom@gmail.com",
		"<tom@gmail.com          >",
		"<to     m@gmail.com          >",
		"<tom@gmail.com>;tom@gmail.com",	
	       "Tom< tom@gmail.com > ,       Tom <tom@gmail.com>",	
		" \"Tom\"  < tom@gmail.com>  ,  \"Tom\"   < tom@gmail.com> ,  Tom  < tom@gmail.com> ; Tom  < tom@gmail.com>,\"tomtom\" <   tom@aol.com>,\"tomtom\" <   tom@aol.com>    ,\"tomtom\" <tom@aol.com>;<tom@live.com>,  <tom@live.com>; \"tomtomtomtom\" <tom@live.com>  ; \"tomtomtomtom\" <tom@live.com>,\"Tom\"  < tom@gmail.com>",
		"<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>,<tom@gmail.com>",
		"             tom@gmail.com         ;   <  tom@gmail.com   > ",
		"\"Tom\" <        tom@gmail.com >  ;  mama <  tom@gmail.com    >  ,   abcd@gmail.com     ,   abc@gmail.com ,\"tomtom\" <tom@aol.com>,\"tomtom\"  < tom@aol.com> ;\"tomtom\" <   tom@aol.com   >    ,\"tomtom\" <tom@aol.com> , \"tomtomtomtom\" <tom@live.com>  ",
		"             \"tomtom\" <tom@aol.com>    ;\"tomtom\" <tom@aol.com> ;          ",
		"  tom@gmail.com  ,   tom@gmail.com ,   tom@gmail.com; Tom  < tom@gmail.com   >  ,<   tom@aol.com>, <tom@aol.com>    ,<tom@aol.com>;<tom@live.com>,  tom@live.com;tom@live.com; \"tomtomtomtom\" <tom@live.com>,\"Tom\"  < tom@gmail.com> ;    ",
		"<tom@aol.com>    , \"tomtomtomtom\" <tom@live.com>,\"Tom\"  < tom@gmail.com> ;    ",
		" Tom <tom@gmail.com>,Tom <tom@gmail.com>,Tom <tom@gmail.com>,Tom <tom@gmail.com>,Tom <   tom@gmail.com>  ,<tom@gmail.com>,Tom <tom@gmail.com       >,Tom< tom@gmail.com > ,       Tom <tom@gmail.com>,Tom <tom@gmail.com>,Tom <tom@gmail.com>,Tom <tom@gmail.com>,Tom <tom@gmail.com>,Tom <tom@gmail.com>,Tom <tom@gmail.com>,Tom <tom@gmail.com>",
		NULL
	};
	int address_count = 0;
	
	testapp_print("Select input method:\n");
	testapp_print("1. Test data\n");
	testapp_print("2. Keyboard\n");
	result_from_scanf = scanf("%d", &type);

	switch ( type ) {
		case 1:
			do  {
				for ( i = 0; address_list[i] != NULL; i++ ) {
					testapp_print("[%d] addr[%s]\n", i, address_list[i]);
					address_count++;
				}
				testapp_print("Choose address to be tested:[99:quit]\n");
				result_from_scanf = scanf("%d", &index);
				if ( index == 99 )
					break;

				testapp_print(">> [%d] Checking? (1:Yes, Other:No) [%s]\n", index, address_list[index]);
				result_from_scanf = scanf("%d", &check_yn);
				if ( check_yn == 1 ) {
					pAddress = strdup(address_list[index]);
					if ( em_verify_email_address(pAddress, false, &err ) == true ) {
						testapp_print("<< [%d] Checking Succeeded : addr[%s]\n", index, address_list[index]);
					}
					else {
						testapp_print("<< [%d] Checking Failed    : addr[%s], err[%d]\n", index, address_list[index], err);
					}			
					if(pAddress) {
						free(pAddress);
						pAddress = NULL;
					}
				}
				else {
					testapp_print("<< [%d]  Skipped            : addr[%s]\n", index, address_list[index]);
				}
			} 
			while (1);
			break;
		case 2:
			testapp_print("Input address: \n");
			result_from_scanf = scanf("%s", input_address);
			if ( em_verify_email_address(input_address, false, &err ) == true ) {
				testapp_print(">> Saving Succeeded : addr[%s]\n", input_address);
			}
			else {
				testapp_print(">> Saving Failed : addr[%s], err[%d]\n", input_address, err);
			}
			break;
		default:
			testapp_print("wrong nuber... [%d]\n", type);
			break;
	}
	*/
	return FALSE;
}

static gboolean testapp_test_get_max_mail_count()
{
	int max_count = -1;
	int err = EMAIL_ERROR_NONE;

	err = email_get_max_mail_count(&max_count);
	testapp_print("\n\t>>>>> email_get_max_mail_count() return [%d]\n\n", max_count);
	
	return false;
}

#include "email-storage.h"
static gboolean testapp_test_test_get_thread_information()
{
	int error_code, thread_id= 38;
	email_mail_list_item_t *result_mail;
	
	if( (error_code = email_get_thread_information_ex(thread_id, &result_mail)) == EMAIL_ERROR_NONE) {
		testapp_print("email_get_thread_information returns = %d\n", error_code);
		testapp_print("subject = %s\n", result_mail->subject);
		testapp_print("mail_id = %d\n", result_mail->mail_id);
		testapp_print("from = %s\n", result_mail->from);
		testapp_print("thrad_id = %d\n", result_mail->thread_id);
		testapp_print("count = %d\n", result_mail->thread_item_count);
	}

	return TRUE;
}

static gboolean testapp_test_get_address_info_list()
{
	testapp_print(" >>> testapp_test_get_address_info_list : Entered \n");

	char buf[1024];
	int i = 0;
	int mail_id = 0;
	int result_from_scanf = 0;
	email_address_info_list_t *address_info_list= NULL;
	email_address_info_t *p_address_info = NULL;
	GList *list = NULL;
	GList *node = NULL;

	memset(buf, 0x00, sizeof(buf));
	testapp_print("\n > Enter mail_id: ");
	result_from_scanf = scanf("%d",&mail_id);

	email_get_address_info_list(mail_id, &address_info_list);

	testapp_print("===================================================\n");
	testapp_print("\t\t\t address info list\n");
	testapp_print("===================================================\n");
	testapp_print("address_type\t address \t\tdisplay_name\t contact id\n");
	testapp_print("===================================================\n");

	for ( i = EMAIL_ADDRESS_TYPE_FROM; i <= EMAIL_ADDRESS_TYPE_BCC; i++ ) {
		switch ( i ) {
			case EMAIL_ADDRESS_TYPE_FROM:
				list = address_info_list->from;
				break;
			case EMAIL_ADDRESS_TYPE_TO:
				list = address_info_list->to;
				break;
			case EMAIL_ADDRESS_TYPE_CC:
				list = address_info_list->cc;
				break;
			case EMAIL_ADDRESS_TYPE_BCC:
				list = address_info_list->bcc;
				break;
		}

		/*  delete dynamic-allocated memory for each item */
		node = g_list_first(list);
		while ( node != NULL ) {
			p_address_info = (email_address_info_t*)node->data;
			testapp_print("%d\t\t%s\t%s\t%d\n", p_address_info->address_type, p_address_info->address, p_address_info->display_name, p_address_info->contact_id);

			node = g_list_next(node);
		}
		testapp_print("\n");
	}
	testapp_print("===================================================\n");

	if(address_info_list)
		email_free_address_info_list(&address_info_list);

	testapp_print(" >>> testapp_test_get_address_info_list : END \n");
	return TRUE;
}

static gboolean testapp_test_search_mail_on_server()
{
	testapp_print(" >>> testapp_test_search_mail_on_server : Entered \n");

	int err_code = EMAIL_ERROR_NONE;
	int account_id = 0;
	int mailbox_id = 0;
	int search_key_value_integer = 0;
	int result_from_scanf = 0;
	email_search_filter_type search_filter_type = 0;
	email_search_filter_t search_filter;
	unsigned int handle = 0;
	char search_key_value_string[MAX_EMAIL_ADDRESS_LENGTH];

	testapp_print("input account id : ");
	result_from_scanf = scanf("%d",&account_id);

	testapp_print("input mailbox id : ");
	result_from_scanf = scanf("%d", &mailbox_id);

	testapp_print(
		"	EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       =  1,  ( integet type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_UID              =  2,  ( integet type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_BCC              =  7,  ( string type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_CC               =  9,  ( string type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_FROM             = 10,  ( string type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_KEYWORD          = 11,  ( string type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_SUBJECT          = 13,  ( string type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_TO               = 15,  ( string type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      = 16,  ( integet type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     = 17,  ( integet type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE = 20,  ( time type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     = 21,  ( time type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  = 22,  ( time type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   = 26,  ( integet type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    = 28,  ( integet type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      = 30,  ( integet type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     = 32,  ( integet type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     = 34,  ( integet type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       = 36,  ( integet type ) \n"
		"	EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       = 43,  ( string type ) \n");
	testapp_print("input search filter type : ");
	result_from_scanf = scanf("%d", (int*)&search_filter_type);

	search_filter.search_filter_type = search_filter_type;

	switch(search_filter_type) {
		case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       :
		case EMAIL_SEARCH_FILTER_TYPE_UID              :
		case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      :
		case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     :
		case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       :
			testapp_print("input search filter key value : ");
			result_from_scanf = scanf("%d", &search_key_value_integer);
			search_filter.search_filter_key_value.integer_type_key_value = search_key_value_integer;
			break;

		case EMAIL_SEARCH_FILTER_TYPE_BCC              :
		case EMAIL_SEARCH_FILTER_TYPE_CC               :
		case EMAIL_SEARCH_FILTER_TYPE_FROM             :
		case EMAIL_SEARCH_FILTER_TYPE_KEYWORD          :
		case EMAIL_SEARCH_FILTER_TYPE_SUBJECT          :
		case EMAIL_SEARCH_FILTER_TYPE_TO               :
		case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       :
			testapp_print("input search filter key value : ");
			result_from_scanf = scanf("%s", search_key_value_string);
			search_filter.search_filter_key_value.string_type_key_value = search_key_value_string;
			break;

		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE :
		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     :
		case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  :
			testapp_print("input search filter key value (format = YYYYMMDDHHMMSS) : ");
			result_from_scanf = scanf("%s", search_key_value_string);
			/* TODO : write codes for converting string to time */
			/* search_filter.search_filter_key_value.time_type_key_value = search_key_value_string; */
			break;
		default :
			testapp_print("Invalid filter type [%d]", search_filter_type);
			return FALSE;
	}

	if( (err_code = email_search_mail_on_server(account_id, mailbox_id, &search_filter,1, &handle)) != EMAIL_ERROR_NONE) {
		testapp_print("email_search_mail_on_server failed [%d]", err_code);
	}

	testapp_print(" >>> testapp_test_search_mail_on_server : END \n");
	return TRUE;
}

static gboolean testapp_test_add_mail_to_search_result_box()
{
	int                    i = 0;
	int                    account_id = 0;
	int                    from_eas = 0;
	int                    attachment_count = 0;
	int                    err = EMAIL_ERROR_NONE;
	int                    result_from_scanf = 0;
	char                   arg[50] = { 0 , };
	char                  *body_file_path = MAIL_TEMP_BODY;
	email_mailbox_t         *mailbox_data = NULL;
	email_mail_data_t       *test_mail_data = NULL;
	email_attachment_data_t *attachment_data = NULL;
	email_meeting_request_t *meeting_req = NULL;
	FILE                  *body_file = NULL;

	testapp_print("\n > Enter account id : ");
	result_from_scanf = scanf("%d", &account_id);

	email_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_SEARCH_RESULT, &mailbox_data);

	test_mail_data = malloc(sizeof(email_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));

	testapp_print("\n From EAS? [0/1]> ");
	result_from_scanf = scanf("%d", &from_eas);

	test_mail_data->account_id           = account_id;
	test_mail_data->save_status          = 1;
	test_mail_data->body_download_status = 1;
	test_mail_data->flags_seen_field     = 1;
	test_mail_data->file_path_plain      = strdup(body_file_path);
	test_mail_data->mailbox_id           = mailbox_data->mailbox_id;
	test_mail_data->mailbox_type         = mailbox_data->mailbox_type;
	test_mail_data->full_address_from    = strdup("<test1@test.com>");
	test_mail_data->full_address_to      = strdup("<test2@test.com>");
	test_mail_data->full_address_cc      = strdup("<test3@test.com>");
	test_mail_data->full_address_bcc     = strdup("<test4@test.com>");
	test_mail_data->subject              = strdup("Into search result mailbox");

	body_file = fopen(body_file_path, "w");

	for(i = 0; i < 500; i++)
		fprintf(body_file, "X2 X2 X2 X2 X2 X2 X2");
	fflush(body_file);
	fclose(body_file);

	testapp_print(" > Attach file? [0/1] : ");
	result_from_scanf = scanf("%d",&attachment_count);

	if ( attachment_count )  {
		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment name : ");
		result_from_scanf = scanf("%s", arg);

		attachment_data = malloc(sizeof(email_attachment_data_t));

		attachment_data->attachment_name  = strdup(arg);

		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment absolute path : ");
		result_from_scanf = scanf("%s",arg);

		attachment_data->attachment_path  = strdup(arg);
		attachment_data->save_status      = 1;
		test_mail_data->attachment_count  = attachment_count;
	}

	testapp_print("\n > Meeting Request? [0: no, 1: yes (request from server), 2: yes (response from local)]");
	result_from_scanf = scanf("%d", &(test_mail_data->meeting_request_status));

	if ( test_mail_data->meeting_request_status == 1
		|| test_mail_data->meeting_request_status == 2 ) {
		time_t current_time;
		/*  dummy data for meeting request */
		meeting_req = malloc(sizeof(email_meeting_request_t));
		memset(meeting_req, 0x00, sizeof(email_meeting_request_t));

		meeting_req->meeting_response     = 1;
		current_time = time(NULL);
		gmtime_r(&current_time, &(meeting_req->start_time));
		gmtime_r(&current_time, &(meeting_req->end_time));
		meeting_req->location = malloc(strlen("Seoul") + 1);
		memset(meeting_req->location, 0x00, strlen("Seoul") + 1);
		strcpy(meeting_req->location, "Seoul");
		strcpy(meeting_req->global_object_id, "abcdef12345");

		meeting_req->time_zone.offset_from_GMT = 9;
		strcpy(meeting_req->time_zone.standard_name, "STANDARD_NAME");
		gmtime_r(&current_time, &(meeting_req->time_zone.standard_time_start_date));
		meeting_req->time_zone.standard_bias = 3;

		strcpy(meeting_req->time_zone.daylight_name, "DAYLIGHT_NAME");
		gmtime_r(&current_time, &(meeting_req->time_zone.daylight_time_start_date));
		meeting_req->time_zone.daylight_bias = 7;

	}

	if((err = email_add_mail(test_mail_data, attachment_data, attachment_count, meeting_req, from_eas)) != EMAIL_ERROR_NONE)
		testapp_print("email_add_mail failed. [%d]\n", err);
	else
		testapp_print("email_add_mail success.\n");

	testapp_print("saved mail id = [%d]\n", test_mail_data->mail_id);

	if(attachment_data)
		email_free_attachment_data(&attachment_data, attachment_count);

	if(meeting_req)
		email_free_meeting_request(&meeting_req, 1);

	email_free_mail_data(&test_mail_data, 1);
	email_free_mailbox(&mailbox_data, 1);

	return FALSE;
}

static gboolean testapp_test_email_load_eml_file()
{
	email_mail_data_t *mail_data = NULL;
	email_attachment_data_t *mail_attachment_data = NULL;
	int i = 0;
	int attachment_count = 0;
	int err = EMAIL_ERROR_NONE;
	int result_from_scanf = 0;
	char eml_file_path[255] = {0, };

	testapp_print("Input eml file path : ");
	result_from_scanf = scanf("%s", eml_file_path);

	if ((err = email_open_eml_file(eml_file_path, &mail_data, &mail_attachment_data, &attachment_count)) != EMAIL_ERROR_NONE)
	{
		testapp_print("email_open_eml_file failed : [%d]\n", err);
		return false;	
	}
	
	testapp_print("load success\n");
	testapp_print("Return-Path: %s\n", mail_data->full_address_return);
	testapp_print("To: %s\n", mail_data->full_address_to);
	testapp_print("Subject: %s\n", mail_data->subject);
	testapp_print("From: %s\n", mail_data->full_address_from);
	testapp_print("Reply-To: %s\n", mail_data->full_address_reply);
	testapp_print("Sender: %s\n", mail_data->email_address_sender);
	testapp_print("Message-ID: %s\n", mail_data->message_id);
	testapp_print("attachment_count: %d\n", mail_data->attachment_count);
	testapp_print("inline content count : %d\n", mail_data->inline_content_count);

	for (i = 0;i < mail_data->attachment_count ; i++) {
		testapp_print("attachment_id: %d\n", mail_attachment_data[i].attachment_id);
		testapp_print("inline_attachment_status: %d\n", mail_attachment_data[i].inline_content_status);
		testapp_print("attachment_name: %s\n", mail_attachment_data[i].attachment_name);
		testapp_print("attachment_path: %s\n", mail_attachment_data[i].attachment_path);
		testapp_print("mailbox_id: %d\n", mail_attachment_data[i].mailbox_id);
	}

	testapp_print("Success : Open eml file\n");
	
	if ((err = email_delete_eml_data(mail_data)) != EMAIL_ERROR_NONE) {
		testapp_print("email_delete_eml_data failed : [%d]\n", err);
		return false;
	}

	testapp_print("Success : email_delete_eml_data\n");

	if (mail_data)
		email_free_mail_data(&mail_data, 1);
	
	testapp_print("Success : email_free_mail_data\n");

	if (mail_attachment_data)
		email_free_attachment_data(&mail_attachment_data, attachment_count);

	testapp_print("Success : email_free_attachment_data\n");

	return true;

}

/* internal functions */
static gboolean testapp_test_interpret_command (int menu_number)
{
	gboolean go_to_loop = TRUE;
	
	switch (menu_number) {
		case 1:
			testapp_test_get_mails();
			break;
		case 2:
			testapp_test_mail_send(NULL);
			break;
		case 3:
			testapp_test_get_mail_list();
			break;
		case 4:
			testapp_test_add_attachment();
			break;			
		case 5:
			testapp_test_set_deleted_flag();
			break;
		case 6:
			testapp_test_expunge_mails_deleted_flagged();
			break;
		case 9:
			testapp_test_count();
			break;
		case 14:
			testapp_test_delete();
			break;
 		case 16:
			testapp_test_download_body();
			break;
		case 17:
			testapp_test_download_attachment();
			break;		
		case 20:
			testapp_test_delete_all();
			break;
		case 21:
			testapp_test_move();
			break;
 		case 23:
			testapp_test_retry_send();
			break;
		case 27:
			testapp_test_move_all_mails_to_mailbox();
			break;
		case 38:
			testapp_test_get_totaldiskusage();
			break;
		case 40:
			testapp_test_address_format_check_test();
			break;
		case 41:
			testapp_test_get_max_mail_count();
			break;
		case 42:
			testapp_test_db_test();
			break;
		case 43:
			testapp_test_send_cancel();
			break;
		case 44:
			testapp_test_cancel_download_body();
			break;
		case 46:
			 testapp_test_get_mail_list_for_thread_view();
			break;
 		case 48:
			testapp_test_test_get_thread_information();
			break;
 		case 51:
			testapp_test_get_mail_list_ex();
			break;
		case 52:
			testapp_test_get_address_info_list();
			break;
		case 55:
			testapp_test_set_flags_field();
			break;
		case 56:
			testapp_test_add_mail(NULL);
			break;
		case 57:
			testapp_test_update_mail();
			break;
		case 58:
			testapp_test_search_mail_on_server();
			break;
		case 59:
			testapp_test_add_mail_to_search_result_box();
			break;
		case 60:
			testapp_test_email_load_eml_file();
			break;
		case 0:
			go_to_loop = FALSE;
			break;
		default:
			break;
	}

	return go_to_loop;
}

void testapp_mail_main()
{
	gboolean go_to_loop = TRUE;
	int menu_number = 0;
	int result_from_scanf = 0;
	
	while (go_to_loop) {
		testapp_show_menu(EMAIL_MAIL_MENU);
		testapp_show_prompt(EMAIL_MAIL_MENU);
			
		result_from_scanf = scanf("%d", &menu_number);

		go_to_loop = testapp_test_interpret_command (menu_number);
	}
}

