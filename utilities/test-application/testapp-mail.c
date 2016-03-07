/*
*  email-service
*
* Copyright(c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Kyuho Jo <kyuho.jo@samsung.com>, Sunghyun Kwon <sh0701.kwon@samsung.com>
*
* Licensed under the Apache License, Version 2.0(the "License");
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
#include <wait.h>

/* open header */
#include <glib.h>
#include <time.h>

#include "email-api-account.h"
#include "email-api-network.h"
#include "email-api-mail.h"
#include "email-api-mailbox.h"
#include "email-api-etc.h"
#include "email-api-smime.h"

/* internal header */
#include "testapp-utility.h"
#include "testapp-mail.h"
#include "email-core-utils.h"
#include "email-core-mime.h"

#define MAIL_TEMP_BODY "/tmp/utf8"
#define HTML_TEMP_BODY "/tmp/utf8.htm"

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
		if (mail_list_item[i].from!= NULL) {
			testapp_print(" >>> From [ %s ] \n", mail_list_item[i].from);
		}
		if (mail_list_item[i].from_email_address != NULL) {
			testapp_print(" >>> from_email_address [ %s ] \n", mail_list_item[i].from_email_address);
		}
		if (mail_list_item[i].recipients!= NULL) {
			testapp_print(" >>> recipients [ %s ] \n", mail_list_item[i].recipients);
		}
		if (mail_list_item[i].subject != NULL) {
			testapp_print(" >>> subject [ %s ] \n", mail_list_item[i].subject);
		}
		testapp_print(" >>> text_download_yn [ %d ] \n", mail_list_item[i].body_download_status);
		testapp_print(" >>> date_time [ %d ] \n", mail_list_item[i].date_time);
		testapp_print(" >>> flags_seen_field [ %d ] \n", mail_list_item[i].flags_seen_field);
		testapp_print(" >>> priority [ %d ] \n", mail_list_item[i].priority);
		testapp_print(" >>> save_status [ %d ] \n", mail_list_item[i].save_status);
		testapp_print(" >>> lock [ %d ] \n", mail_list_item[i].lock_status);
		testapp_print(" >>> report_status [ %d ] \n", mail_list_item[i].report_status);
		testapp_print(" >>> recipients_count [ %d ] \n", mail_list_item[i].recipients_count);
		testapp_print(" >>> attachment_count [ %d ] \n", mail_list_item[i].attachment_count);
		testapp_print(" >>> DRM_status [ %d ] \n", mail_list_item[i].DRM_status);

		if (mail_list_item[i].preview_text != NULL) {
			testapp_print(" >>> preview_text [ %s ] \n", mail_list_item[i].preview_text);
		}

		testapp_print(" >>> thread_id [ %d ] \n", mail_list_item[i].thread_id);
		testapp_print(" >>> thread_item_count [ %d ] \n", mail_list_item[i].thread_item_count);
	}
}
*/

static gboolean testapp_add_mail_for_sending(int *result_mail_id)
{
	int                    i = 0;
	int                    account_id = 0;
	int                    err = EMAIL_ERROR_NONE;
	int                    smime_type = 0;
	char                   recipient_address[300] = { 0 , };
	char                   from_address[300] = { 0 , };
	char                   passpharse[300] = {0, };
	const char            *body_file_path = MAIL_TEMP_BODY;

	email_account_t       *account_data = NULL;
	email_mailbox_t       *mailbox_data = NULL;
	email_mail_data_t     *test_mail_data = NULL;
	FILE                  *body_file;

	testapp_print("\n > Enter account id : ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter recipient address : ");
	if (0 >= scanf("%s", recipient_address))
		testapp_print("Invalid input. ");

	email_get_account(account_id, GET_FULL_DATA_WITHOUT_PASSWORD, &account_data);

	email_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &mailbox_data);

	test_mail_data = malloc(sizeof(email_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));

	SNPRINTF(from_address, 300, "<%s>", account_data->user_email_address);

	test_mail_data->account_id             = account_id;
	test_mail_data->save_status            = EMAIL_MAIL_STATUS_SEND_DELAYED;
	test_mail_data->body_download_status   = 1;
	test_mail_data->flags_seen_field       = 1;
	test_mail_data->file_path_plain        = strdup(body_file_path);
	test_mail_data->mailbox_id             = mailbox_data->mailbox_id;
	test_mail_data->mailbox_type           = mailbox_data->mailbox_type;
	test_mail_data->full_address_from      = strdup(from_address);
	test_mail_data->full_address_to        = strdup(recipient_address);
	test_mail_data->subject                = strdup("Read receipt request from TIZEN");
	test_mail_data->remaining_resend_times = 3;
	test_mail_data->report_status          = EMAIL_MAIL_REQUEST_DSN | EMAIL_MAIL_REQUEST_MDN;

	body_file = fopen(body_file_path, "w");

	testapp_print("\n body_file [%p]\n", body_file);

	if (body_file == NULL) {
		testapp_print("\n fopen [%s]failed\n", body_file_path);
		return FALSE;
	}

	for (i = 0; i < 100; i++)
		fprintf(body_file, "Mail sending Test. [%d]\n", i);

	fflush(body_file);
	fclose(body_file);

	testapp_print(" > Select smime? [0: Normal, 1: sign, 2: Encrpyt, 3: sing + encrypt, 4: pgp sign, 5: pgp encrypted, 6: pgp sign + encrypt] : ");
	if (0 >= scanf("%d", &smime_type))
		testapp_print("Invalid input. ");
	test_mail_data->smime_type = smime_type;

	if (smime_type >= EMAIL_PGP_SIGNED) {
		testapp_print(" > passpharse : ");
		if (0 >= scanf("%s", passpharse))
			testapp_print("Invalid input. ");
		test_mail_data->pgp_password = strdup(passpharse);
	}

	if ((err = email_add_mail(test_mail_data, NULL, 0, NULL, 0)) != EMAIL_ERROR_NONE)
		testapp_print("email_add_mail failed. [%d]\n", err);
	else
		testapp_print("email_add_mail success.\n");

	testapp_print("saved mail id = [%d]\n", test_mail_data->mail_id);

	if (result_mail_id)
		*result_mail_id = test_mail_data->mail_id;

	email_free_mail_data(&test_mail_data, 1);
	email_free_mailbox(&mailbox_data, 1);
	email_free_account(&account_data, 1);

	return FALSE;
}

static gboolean testapp_test_add_mail(int *result_mail_id)
{
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
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	memset(arg, 0x00, 50);
	testapp_print("\n > Enter mailbox id : ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input. ");

	email_get_mailbox_by_mailbox_id(mailbox_id, &mailbox_data);

	test_mail_data = malloc(sizeof(email_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));

	testapp_print("\n From EAS? [0/1]> ");
	if (0 >= scanf("%d", &from_eas))
		testapp_print("Invalid input. ");

	test_mail_data->account_id             = account_id;
	test_mail_data->save_status            = 1;
	test_mail_data->body_download_status   = 1;
	test_mail_data->flags_seen_field       = 1;
	test_mail_data->file_path_plain        = strdup(body_file_path);
	test_mail_data->mailbox_id             = mailbox_id;
	test_mail_data->mailbox_type           = mailbox_data->mailbox_type;
	test_mail_data->full_address_from      = strdup("<test1@test.com>");
	test_mail_data->full_address_to        = strdup("<test2@test.com>");
	test_mail_data->full_address_cc        = strdup("<test3@test.com>");
	test_mail_data->full_address_bcc       = strdup("<test4@test.com>");
	test_mail_data->subject                = strdup("Meeting request mail");
	test_mail_data->remaining_resend_times = 3;
	test_mail_data->eas_data               = strdup("EAS DATA TEST");
	test_mail_data->eas_data_length        = strlen(test_mail_data->eas_data) + 1;

	body_file = fopen(body_file_path, "w");

	testapp_print("\n body_file [%p]\n", body_file);

	if (body_file == NULL) {
		testapp_print("\n fopen [%s]failed\n", body_file_path);
		return FALSE;
	}
/*
	for (i = 0; i < 500; i++)
		fprintf(body_file, "X2 X2 X2 X2 X2 X2 X2");
*/
	fprintf(body_file, "Hello world");
	fflush(body_file);
	fclose(body_file);

	testapp_print(" > Select smime? [0: Normal, 1: sign, 2: Encrypt, 3: sing + encrypt] : ");
	if (0 >= scanf("%d", &smime_type))
		testapp_print("Invalid input. ");
	test_mail_data->smime_type = smime_type;

	testapp_print(" > How many file attachment? [>=0] : ");
	if (0 >= scanf("%d", &attachment_count))
		testapp_print("Invalid input. ");

	test_mail_data->attachment_count  = attachment_count;
	if (attachment_count > 0)
		attachment_data = calloc(attachment_count, sizeof(email_attachment_data_t));


	for (i = 0; i < attachment_count ; i++) {
		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment name : ");
		if (0 >= scanf("%s", arg))
			testapp_print("Invalid input. ");

		attachment_data[i].attachment_name  = strdup(arg);

		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment absolute path : ");
		if (0 >= scanf("%s", arg))
			testapp_print("Invalid input. ");

		attachment_data[i].attachment_path  = strdup(arg);
		attachment_data[i].save_status      = 1;
		attachment_data[i].mailbox_id       = test_mail_data->mailbox_id;
	}

	testapp_print("\n > Meeting Request? [0: no, 1: yes(request from server), 2: yes(response from local)]");
	if (0 >= scanf("%d", (int *)&(test_mail_data->meeting_request_status)))
		testapp_print("Invalid input. ");

	if (test_mail_data->meeting_request_status == 1
		|| test_mail_data->meeting_request_status == 2) {
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

	if ((err = email_add_mail(test_mail_data, attachment_data, attachment_count, meeting_req, from_eas)) != EMAIL_ERROR_NONE)
		testapp_print("email_add_mail failed. [%d]\n", err);
	else
		testapp_print("email_add_mail success.\n");

	testapp_print("saved mail id = [%d]\n", test_mail_data->mail_id);
	testapp_print("saved mailbox id = [%d]\n", test_mail_data->mailbox_id);

	if (result_mail_id)
		*result_mail_id = test_mail_data->mail_id;

	if (attachment_data)
		email_free_attachment_data(&attachment_data, attachment_count);

	if (meeting_req)
		email_free_meeting_request(&meeting_req, 1);

	email_free_mail_data(&test_mail_data, 1);
	email_free_mailbox(&mailbox_data, 1);

	return FALSE;
}

static gboolean testapp_test_update_mail()
{
	int                    mail_id = 0;
	int                    err = EMAIL_ERROR_NONE;
	int                    test_attachment_data_count = 0;
	int                    ret = 0;
	char                   arg[50];
	email_mail_data_t       *test_mail_data = NULL;
	email_attachment_data_t *test_attachment_data_list = NULL;
	email_meeting_request_t *meeting_req = NULL;

	testapp_print("\n > Enter mail id : ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	email_get_mail_data(mail_id, &test_mail_data);

	if (!test_mail_data) {
		testapp_print("email_get_mail_data() failed\n");
		return FALSE;
	}

	testapp_print("\n > Enter Subject: ");
	if (0 >= scanf("%s", arg))
		testapp_print("Invalid input. ");

	test_mail_data->subject = strdup(arg);

	if (test_mail_data->attachment_count > 0) {
		if ((err = email_get_attachment_data_list(mail_id, &test_attachment_data_list, &test_attachment_data_count)) != EMAIL_ERROR_NONE) {
			testapp_print("email_get_attachment_data_list() failed [%d]\n", err);
			goto FINISH_OFF;
		}
	}

	if (test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_REQUEST
		|| test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_RESPONSE
		|| test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {

		if ((err = email_get_meeting_request(mail_id, &meeting_req)) != EMAIL_ERROR_NONE) {
			testapp_print("email_get_meeting_request() failed [%d]\n", err);
			goto FINISH_OFF;
		}

		testapp_print("\n > Enter meeting response: ");
		if (0 >= scanf("%d", (int *)&(meeting_req->meeting_response)))
			testapp_print("Invalid input. ");
	}

	if ((err = email_update_mail(test_mail_data, test_attachment_data_list, test_attachment_data_count, meeting_req, 0)) != EMAIL_ERROR_NONE)
			testapp_print("email_update_mail failed.[%d]\n", err);
		else
			testapp_print("email_update_mail success\n");

	ret = 1;

FINISH_OFF:

	if (test_mail_data)
		email_free_mail_data(&test_mail_data, 1);

	if (test_attachment_data_list)
		email_free_attachment_data(&test_attachment_data_list, test_attachment_data_count);

	if (meeting_req)
		email_free_meeting_request(&meeting_req, 1);

	if (!ret)
		return FALSE;

	return TRUE;
}

static gboolean testapp_test_get_mails()
{
	testapp_print("\n >>> testapp_test_get_mails : Entered \n");
	email_mail_data_t *mails = NULL, **mails_pointer = NULL;
	int mailbox_id = 0;
	int count = 0, i = 0;
	int account_id = 0;
	int start_index = 0;
	int limit_count = 0;
	int sorting = 0;
	int err_code = EMAIL_ERROR_NONE;
	int to_get_count = 0;
	int is_for_thread_view = 0;
	int list_type;
	struct tm *temp_time_info;

	testapp_print("\n > Enter Account_id(0 = all accounts) : ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");
	testapp_print("\n > Enter Mailbox id(0 = all mailboxes) :");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter Sorting : ");
	if (0 >= scanf("%d", &sorting))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter Start index(starting at 0): ");
	if (0 >= scanf("%d", &start_index))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter max_count : ");
	if (0 >= scanf("%d", &limit_count))
		testapp_print("Invalid input. ");

	testapp_print("\n > For thread view : ");
	if (0 >= scanf("%d", &is_for_thread_view))
		testapp_print("Invalid input. ");

	testapp_print("\n > Mail count only(0:list 1:count): ");
	if (0 >= scanf("%d", &to_get_count))
		testapp_print("Invalid input. ");

	if (to_get_count)
		mails_pointer = NULL;
	else
		mails_pointer = &mails;

	if (is_for_thread_view == -2)
		list_type = EMAIL_LIST_TYPE_LOCAL;
	else if (is_for_thread_view == -1)
		list_type = EMAIL_LIST_TYPE_THREAD;
	else
		list_type = EMAIL_LIST_TYPE_NORMAL;

	/* Get mail list */
	if (mailbox_id == 0) {
		testapp_print("Calling email_get_mails for all mailbox.\n");
		err_code = email_get_mails(account_id, 0, list_type, start_index, limit_count, sorting, mails_pointer, &count);
		if (err_code < 0)
			testapp_print("email_get_mails failed - err[%d]\n", err_code);
	} else {
		testapp_print("Calling email_get_mails for %d mailbox_id.\n", mailbox_id);
		err_code = email_get_mails(account_id, mailbox_id, list_type, start_index, limit_count, sorting,  mails_pointer, &count);
		if (err_code < 0)
			testapp_print("email_get_mails failed - err[%d]\n", err_code);
	}
	testapp_print("email_get_mails >>>>>>count - %d\n", count);

	if (mails) {
		for (i = 0; i < count; i++) {
			testapp_print("\n[%d]\n", i);
			testapp_print(" >>> mailbox_id [ %d ] \n", mails[i].mailbox_id);
			testapp_print(" >>> mail_id [ %d ] \n", mails[i].mail_id);
			testapp_print(" >>> account_id [ %d ] \n", mails[i].account_id);
			if (mails[i].full_address_from != NULL)
				testapp_print(" >>> full_address_from [ %s ] \n", mails[i].full_address_from);
			if (mails[i].full_address_to != NULL)
				testapp_print(" >>> recipients [ %s ] \n", mails[i].full_address_to);
			if (mails[i].subject != NULL)
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
			if (mails[i].preview_text != NULL)
				testapp_print(" >>> preview_text [ %s ] \n", mails[i].preview_text);
		}
		free(mails);
	}

	testapp_print(" >>> testapp_test_get_mails : End \n");
	return 0;
}

#define TEMP_ARGUMENT_SIZE 4096

static gboolean testapp_test_mail_send(int *result_mail_id)
{
	int                    added_mail_id = 0;
	int                    err = EMAIL_ERROR_NONE;
	int                    handle = 0;
	email_mail_data_t     *result_mail_data = NULL;

	testapp_add_mail_for_sending(&added_mail_id);

	if (added_mail_id) {
		email_get_mail_data(added_mail_id, &result_mail_data);

		testapp_print("Calling email_send_mail...\n");

		if (email_send_mail(added_mail_id, &handle) < 0) {
			testapp_print("Sending failed[%d]\n", err);
		} else {
			testapp_print("Start sending. handle[%d]\n", handle);
		}

		email_free_mail_data(&result_mail_data, 1);
	}

	if (result_mail_id)
		*result_mail_id = added_mail_id;

	return FALSE;
}

static gboolean testapp_test_get_mail_list_ex()
{
	email_list_filter_t *filter_list = NULL;
	email_list_sorting_rule_t *sorting_rule_list = NULL;
	email_mail_list_item_t *result_mail_list = NULL;
	int filter_rule_count = 0;
	int sorting_rule_count = 0;
	int result_mail_count = 0;
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int sort_order = 0;

	filter_rule_count = 3;

	filter_list = malloc(sizeof(email_list_filter_t) * filter_rule_count);
	memset(filter_list, 0 , sizeof(email_list_filter_t) * filter_rule_count);

	filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
	filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[0].list_filter_item.rule.key_value.integer_type_value  = 1;
	filter_list[0].list_filter_item.rule.case_sensitivity              = false;

	filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_AND;

	filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[2].list_filter_item.rule.key_value.integer_type_value  = EMAIL_MAILBOX_TYPE_INBOX;
	filter_list[2].list_filter_item.rule.case_sensitivity              = false;

	/*filter_list[0].list_filter_item_type                                     = EMAIL_LIST_FILTER_ITEM_RULE_ATTACH;
	filter_list[0].list_filter_item.rule_attach.target_attribute             = EMAIL_MAIL_ATTACH_ATTRIBUTE_ATTACHMENT_NAME;
	filter_list[0].list_filter_item.rule_attach.rule_type                    = EMAIL_LIST_FILTER_RULE_INCLUDE;
	filter_list[0].list_filter_item.rule_attach.key_value.string_type_value  = strdup("test");
	filter_list[0].list_filter_item.rule_attach.case_sensitivity             = false;*/

	/*
	filter_list[0].list_filter_item_type                                  = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[0].list_filter_item.rule_fts.target_attribute             = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
	filter_list[0].list_filter_item.rule_fts.rule_type                    = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[0].list_filter_item.rule_fts.key_value.integer_type_value = 1;

	filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;


	filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
	filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
	filter_list[2].list_filter_item.rule.key_value.string_type_value   = strdup("2013");
	filter_list[2].list_filter_item.rule.case_sensitivity              = false;


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
	*/

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
	sorting_rule_count = 2;

	sorting_rule_list = malloc(sizeof(email_list_sorting_rule_t) * sorting_rule_count);
	memset(sorting_rule_list, 0 , sizeof(email_list_sorting_rule_t) * sorting_rule_count);
/*
	sorting_rule_list[0].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_RECIPIENT_ADDRESS;
	sorting_rule_list[0].key_value.string_type_value                   = strdup("minsoo.kimn@gmail.com");
	sorting_rule_list[0].sort_order                                    = EMAIL_SORT_ORDER_TO_CCBCC;
*/
	testapp_print("\n Enter the sort_order :");
	if (0 >= scanf("%d", &sort_order))
		testapp_print("Invalid input.");

	sorting_rule_list[0].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
	sorting_rule_list[0].sort_order                                    = sort_order;

	sorting_rule_list[1].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_DATE_TIME;
	sorting_rule_list[1].sort_order                                    = EMAIL_SORT_ORDER_DESCEND;

	err = email_get_mail_list_ex(filter_list, filter_rule_count, sorting_rule_list, sorting_rule_count, -1, -1, &result_mail_list, &result_mail_count);

	if (err == EMAIL_ERROR_NONE) {
		testapp_print("email_get_mail_list_ex succeed.\n");

		for (i = 0; i < result_mail_count; i++) {
			testapp_print("mail_id [%d], subject [%s], mailbox_type [%d] full_address_from [%s]\n", result_mail_list[i].mail_id, result_mail_list[i].subject, result_mail_list[i].mailbox_type, result_mail_list[i].full_address_from);
		}
	} else {
		testapp_print("email_get_mail_list_ex failed.\n");
	}

	email_free_list_filter(&filter_list, 3);

	return FALSE;
}

static gboolean testapp_test_send_cancel()
{
	int num = 0;
	int Y = 0;
	int i = 0;
	int j = 0;
	int *mailIdList = NULL;
	int mail_id = 0;

	testapp_print("\n > Enter total Number of mail  want to send: ");
	if (0 >= scanf("%d", &num))
		testapp_print("Invalid input. ");
	mailIdList = (int *)malloc(sizeof(int)*num);
	if (!mailIdList)
		return false ;

	for (i = 1; i <= num; i++) {
		testapp_test_mail_send(&mail_id);

		testapp_print("mail_id[%d]", mail_id);

		mailIdList[i] = mail_id;
		testapp_print("mailIdList[%d][%d]", i, mailIdList[i]);

		mail_id = 0;
		testapp_print("\n > Do you want to cancel the send mail job '1' or '0': ");
		if (0 >= scanf("%d", &Y))
			testapp_print("Invalid input. ");
		if (Y == 1) {
			testapp_print("\n >Enter mail-id[1-%d] ", i);
			if (0 >= scanf("%d", &j))
				testapp_print("Invalid input. ");
			testapp_print("\n mailIdList[%d] ", mailIdList[j]);
			if (email_cancel_sending_mail(mailIdList[j]) < 0)
				testapp_print("email_cancel_sending_mail failed..!");
			else
				testapp_print("email_cancel_sending_mail success..!");
		}
	}
	return FALSE;
}

static gboolean testapp_test_delete()
{
	int mail_id = 0, account_id = 0;
	int mailbox_id = 0;
	int err = EMAIL_ERROR_NONE;
	int from_server = 0;

	testapp_print("\n > Enter Account_id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter Mail_id: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter Mailbox id: ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter from_server: ");
	if (0 >= scanf("%d", &from_server))
		testapp_print("Invalid input. ");

	/* delete message */
	if ((err = email_delete_mail(mailbox_id, &mail_id, 1, from_server)) < 0)
		testapp_print("\n email_delete_mail failed[%d]\n", err);
	else
		testapp_print("\n email_delete_mail success\n");

	return FALSE;
}

static gboolean testapp_test_update_mail_attribute()
{
	int  err = EMAIL_ERROR_NONE;
	int  i = 0;
	int  account_id = 0;
	int *mail_id_array = NULL;
	int  mail_id_count = 0;
	email_mail_attribute_type attribute_type;
	email_mail_attribute_value_t attribute_value;

	testapp_print("\n > Enter account_id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter attribute_type: ");
	if (0 >= scanf("%d", (int*)&attribute_type))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter integer type value: ");
	if (0 >= scanf("%d", (int*)&(attribute_value.integer_type_value)))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter mail_id_count: ");
	if (0 >= scanf("%d", &mail_id_count))
		testapp_print("Invalid input. ");

	mail_id_count = (mail_id_count < 5000) ? mail_id_count : 5000;

	if (mail_id_count > 0) {
		mail_id_array = malloc(sizeof(int) * mail_id_count);
	}

	for (i = 0; i < mail_id_count; i++) {
		testapp_print("\n > Enter mail id: ");
		if (0 >= scanf("%d", (mail_id_array + i)))
			testapp_print("Invalid input. ");
	}

	/* delete message */
	if ((err = email_update_mail_attribute(account_id, mail_id_array, mail_id_count, attribute_type, attribute_value)) < EMAIL_ERROR_NONE)
		testapp_print("\n email_update_mail_attribute failed[%d]\n", err);
	else
		testapp_print("\n email_update_mail_attribute success\n");

	if (mail_id_array)
		free(mail_id_array);

	return FALSE;
}

static gboolean testapp_test_move()
{
	int mail_id[3];
	int i = 0;
	int mailbox_id = 0;

	for (i = 0; i < 3; i++) {
		testapp_print("\n > Enter mail_id: ");
		if (0 >= scanf("%d", &mail_id[i]))
			testapp_print("Invalid input. ");
	}

	testapp_print("\n > Enter mailbox_id: ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input. ");

	/* move mail */
	email_move_mail_to_mailbox(mail_id, 3, mailbox_id);
	return FALSE;
}

static gboolean testapp_test_delete_all()
{
	int mailbox_id = 0;
	int err = EMAIL_ERROR_NONE;

	testapp_print("\n > Enter mailbox_id: ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input. ");

	/* delete all message */
	if ((err = email_delete_all_mails_in_mailbox(mailbox_id, 0)) < 0)
		testapp_print("email_delete_all_mails_in_mailbox failed [%d]\n", err);
	else
		testapp_print("email_delete_all_mails_in_mailbox Success\n");

	return FALSE;
}


static gboolean testapp_test_add_attachment()
{
	int mail_id = 0;
	char arg[100];
	email_attachment_data_t attachment;

	testapp_print("\n > Enter Mail Id: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	memset(&attachment, 0x00, sizeof(email_attachment_data_t));
	memset(arg, 0x00, 100);
	testapp_print("\n > Enter attachment name: ");
	if (0 >= scanf("%s", arg))
		testapp_print("Invalid input. ");

	attachment.attachment_name = strdup(arg);

	memset(arg, 0x00, 100);
	testapp_print("\n > Enter attachment absolute path: ");
	if (0 >= scanf("%s", arg))
		testapp_print("Invalid input. ");

	attachment.save_status = true;
	attachment.attachment_path = strdup(arg);
	if (email_add_attachment(mail_id, &attachment) < 0)
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

	testapp_print("\n >>> Input target account id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	do {
		testapp_print("\n >>> Input target mail id( Input 0 to terminate ) [MAX = 100]: ");
		if (0 >= scanf("%d", &temp_mail_id))
			testapp_print("Invalid input. ");
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
	int handle = 0;

	testapp_print("\n >>> Input target mailbox id: ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input. ");

	testapp_print("\n >>> Expunge on server?: ");
	if (0 >= scanf("%d", &on_server))
		testapp_print("Invalid input. ");

	err_code = email_expunge_mails_deleted_flagged(mailbox_id, on_server, &handle);

	testapp_print("email_expunge_mails_deleted_flagged returns - err[%d]\n", err_code);

	return 0;
}

static gboolean testapp_test_send_read_receipt()
{
	int read_mail_id = 0;
	int receipt_mail_id = 0;
	int err_code = EMAIL_ERROR_NONE;
	int handle = 0;

	testapp_print("\n >>> Input read mail id: ");
	if (0 >= scanf("%d", &read_mail_id))
		testapp_print("Invalid input. ");

	err_code = email_add_read_receipt(read_mail_id, &receipt_mail_id);

	testapp_print("eamil_add_read_receipt returns receipt_mail_id [%d] - err[%d]\n", receipt_mail_id, err_code);
	testapp_print("Calling email_send_mail...\n");

	if ((err_code = email_send_mail(receipt_mail_id, &handle)) != EMAIL_ERROR_NONE) {
		testapp_print("Sending failed[%d]\n", err_code);
	} else {
		testapp_print("Start sending. handle[%d]\n", handle);
	}

	return 0;
}

static gboolean testapp_test_delete_attachment()
{
	int attachment_id = 0;
	int err_code = EMAIL_ERROR_NONE;

	testapp_print("\n >>> Input attachment id: ");
	if (0 >= scanf("%d", &attachment_id))
		testapp_print("Invalid input. ");

	if ((err_code = email_delete_attachment(attachment_id)) != EMAIL_ERROR_NONE) {
		testapp_print("email_delete_attachment failed[%d]\n", err_code);
	}

	return 0;
}

static gboolean testapp_test_get_mail_list()
{
	testapp_print("\n >>> testapp_test_get_mail_list : Entered \n");
	email_mail_list_item_t *mail_list = NULL, **mail_list_pointer = NULL;
	int mailbox_id = 0;
	int count = 0, i = 0;
	int account_id = 0;
	int start_index = 0;
	int limit_count = 0;
	int sorting = 0;
	int err_code = EMAIL_ERROR_NONE;
	int to_get_count = 0;
	int is_for_thread_view = 0;
	int list_type;
	struct tm *temp_time_info;

	testapp_print("\n > Enter Account_id(0 = all accounts) : ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter Mailbox id(0 = all mailboxes) :");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter Sorting : ");
	if (0 >= scanf("%d", &sorting))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter Start index(starting at 0): ");
	if (0 >= scanf("%d", &start_index))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter max_count : ");
	if (0 >= scanf("%d", &limit_count))
		testapp_print("Invalid input. ");

	testapp_print("\n > For thread view : ");
	if (0 >= scanf("%d", &is_for_thread_view))
		testapp_print("Invalid input. ");

	testapp_print("\n > Count mails?(1: YES):");
	if (0 >= scanf("%d", &to_get_count))
		testapp_print("Invalid input. ");

	if (to_get_count)
		mail_list_pointer = NULL;
	else
		mail_list_pointer = &mail_list;

	if (is_for_thread_view == -2) {
		list_type = EMAIL_LIST_TYPE_LOCAL;
 	} else if (is_for_thread_view == -1)
		list_type = EMAIL_LIST_TYPE_THREAD;
	else
		list_type = EMAIL_LIST_TYPE_NORMAL;

	/* Get mail list */
	if (mailbox_id == 0) {
		testapp_print("Calling email_get_mail_list for all mailbox.\n");
		err_code = email_get_mail_list(account_id, 0, list_type, start_index, limit_count, sorting,  mail_list_pointer, &count);
		if (err_code < 0)
			testapp_print("email_get_mail_list failed - err[%d]\n", err_code);
	} else {
		testapp_print("Calling email_get_mail_list for %d mailbox_id.\n", mailbox_id);
		err_code = email_get_mail_list(account_id, mailbox_id, list_type, start_index, limit_count, sorting,  mail_list_pointer, &count);
		if (err_code < 0)
			testapp_print("email_get_mail_list failed - err[%d]\n", err_code);
	}
	testapp_print("email_get_mail_list >>>>>>count - %d\n", count);

	if (mail_list) {
		for (i = 0; i < count; i++) {
			testapp_print("\n[%d]\n", i);
			testapp_print(" >>> mailbox_id [ %d ] \n", mail_list[i].mailbox_id);
			testapp_print(" >>> mailbox_type [ %d ] \n", mail_list[i].mailbox_type);
			testapp_print(" >>> mail_id [ %d ] \n", mail_list[i].mail_id);
			testapp_print(" >>> account_id [ %d ] \n", mail_list[i].account_id);
			if (mail_list[i].full_address_from != NULL)
				testapp_print(" >>> full_address_from [ %s ] \n", mail_list[i].full_address_from);
			if (mail_list[i].email_address_recipient != NULL)
				testapp_print(" >>> email_address_recipient [ %s ] \n", mail_list[i].email_address_recipient);
			if (mail_list[i].subject != NULL)
				testapp_print(" >>> subject [ %s ] \n", mail_list[i].subject);
			testapp_print(" >>> body_download_status [ %d ] \n", mail_list[i].body_download_status);
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
			testapp_print(" >>> lock_status [ %d ] \n", mail_list[i].lock_status);
			testapp_print(" >>> attachment_count [ %d ] \n", mail_list[i].attachment_count);
			if (mail_list[i].preview_text != NULL)
				testapp_print(" >>> preview_text [ %s ] \n", mail_list[i].preview_text);
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
	int mailbox_id = 0;
	int err_code = EMAIL_ERROR_NONE;
	struct tm *time_info;

	testapp_print("\nEnter account id\n");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\nEnter mailbox id\n");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input. ");

	/* Get mail list */
	if (email_get_mail_list(account_id, mailbox_id , EMAIL_LIST_TYPE_THREAD, 0, 500, EMAIL_SORT_DATETIME_HIGH, &mail_list, &count) < 0)  {
		testapp_print("email_get_mail_list failed : %d\n", err_code);
		return FALSE;
	}
	testapp_print("email_get_mail_list >>>>>>count - %d\n", count);
	if (mail_list) {
		for (i = 0; i < count; i++) {
			testapp_print(" i [%d]\n", i);
			testapp_print(" >>> mail_id [ %d ] \n", mail_list[i].mail_id);
			testapp_print(" >>> account_id [ %d ] \n", mail_list[i].account_id);
			testapp_print(" >>> mailbox_id [ %d ] \n", mail_list[i].mailbox_id);
			testapp_print(" >>> full_address_from [ %s ] \n", mail_list[i].full_address_from);
			testapp_print(" >>> email_address_recipient [ %s ] \n", mail_list[i].email_address_recipient);
			testapp_print(" >>> subject [ %s ] \n", mail_list[i].subject);
			testapp_print(" >>> body_download_status [ %d ] \n", mail_list[i].body_download_status);
			time_info = localtime(&mail_list[i].date_time);
			testapp_print(" >>> date_time [ %s ] \n", asctime(time_info));
			testapp_print(" >>> flags_seen_field [ %d ] \n", mail_list[i].flags_seen_field);
			testapp_print(" >>> priority [ %d ] \n", mail_list[i].priority);
			testapp_print(" >>> save_status [ %d ] \n", mail_list[i].save_status);
			testapp_print(" >>> lock [ %d ] \n", mail_list[i].lock_status);
			testapp_print(" >>> attachment_count [ %d ] \n", mail_list[i].attachment_count);
			testapp_print(" >>> preview_text [ %s ] \n", mail_list[i].preview_text);
			testapp_print(" >>> thread_id [ %d ] \n", mail_list[i].thread_id);
			testapp_print(" >>> thread__item_count [ %d ] \n", mail_list[i].thread_item_count);
		}
		free(mail_list);
	}
	testapp_print(" >>> testapp_test_get_mail_list_for_thread_view : End \n");
	return 0;
}

static gboolean testapp_test_count()
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

	if (EMAIL_ERROR_NONE == email_count_mail(filter_list, 3, &total, &unseen))
		printf("\n Total: %d, Unseen: %d \n", total, unseen);
	return 0;
}

static gboolean testapp_test_move_mails_to_mailbox_of_another_account()
{
	int  err = EMAIL_ERROR_NONE;
	int  mail_id_count = 0 ;
	int *mail_id_array = NULL;
	int  source_mailbox_id = 0;
	int  target_mailbox_id = 0;
	int  task_id = 0;
	int  i = 0;

	testapp_print("\n > Enter source mailbox id: ");
	if (0 >= scanf("%d", &source_mailbox_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter target mailbox id: ");
	if (0 >= scanf("%d", &target_mailbox_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter mail count: ");
	if (0 >= scanf("%d", &mail_id_count))
		testapp_print("Invalid input. ");

	mail_id_count = (mail_id_count < 5000) ? mail_id_count : 5000;

	if (mail_id_count > 0) {
		mail_id_array = malloc(sizeof(int) * mail_id_count);
	}

	for (i = 0; i < mail_id_count; i++) {
		testapp_print("\n > Enter mail id: ");
		if (0 >= scanf("%d", (mail_id_array + i)))
			testapp_print("Invalid input. ");
	}

	err = email_move_mails_to_mailbox_of_another_account(source_mailbox_id, mail_id_array, mail_id_count, target_mailbox_id, &task_id);

	testapp_print("\nemail_move_mails_to_mailbox_of_another_account returns [%d], tast_id [%d] \n", err, task_id);
	return 0;
}

static gboolean	copy_file(char *input_source_path, char *input_dest_path)
{
    int childExitStatus;
    pid_t pid;
    if (!input_source_path || !input_dest_path) {
        return FALSE;
    }
    testapp_print("copy_file started\n");

    pid = fork();

    if (pid == 0) {
    	testapp_print("Copying file [%s] [%s]\n", input_source_path, input_dest_path);
        execl("/bin/cp", "/bin/cp", input_source_path, input_dest_path, (char *)0);
    } else {
    	testapp_print("Wating child process\n");
        pid_t ws = waitpid(pid, &childExitStatus, WNOHANG);
        if (ws == -1) { /* error - handle as you wish */
        	testapp_print("waitpid returns error\n");
        }

        if (WIFEXITED(childExitStatus)) {
			/* exit code in childExitStatus */
            WEXITSTATUS(childExitStatus); /* zero is normal exit */
            testapp_print("WEXITSTATUS\n");
            /* handle non-zero as you wish */
        } else if (WIFSIGNALED(childExitStatus)) { /* killed */
        	testapp_print("WIFSIGNALED\n");
        } else if (WIFSTOPPED(childExitStatus)) { /* stopped */
        	testapp_print("WIFSTOPPED\n");
        }
    }
    testapp_print("copy_file finished\n");
    return TRUE;
}


static gboolean	testapp_test_send_mail_with_downloading_attachment_of_original_mail()
{
	int err = EMAIL_ERROR_NONE;
	int original_mail_id = 0;
	int original_attachment_count = 0;
	int i = 0;
	int handle = 0;
	char *plain_text_path = MAIL_TEMP_BODY;
	char *html_file_path = HTML_TEMP_BODY;
	char new_subject[4086] = { 0, };
/*	FILE *body_file; */
	email_mail_data_t *original_mail_data = NULL;
	email_mailbox_t *outbox = NULL;
	email_attachment_data_t *original_attachment_array = NULL;

	testapp_print("\n > Enter original mail id: ");
	if (0 >= scanf("%d", &original_mail_id))
		testapp_print("Invalid input. ");

	/* Get original mail */
	if ((err = email_get_mail_data(original_mail_id, &original_mail_data)) != EMAIL_ERROR_NONE || !original_mail_data) {
		testapp_print("email_get_mail_data failed [%d]\n", err);
		return FALSE;
	}

	/* Get attachment of original mail */
	if ((err = email_get_attachment_data_list(original_mail_id, &original_attachment_array, &original_attachment_count)) != EMAIL_ERROR_NONE || !original_attachment_array) {
		testapp_print("email_get_attachment_data_list failed [%d]\n", err);
		return FALSE;
	}

	/* Remove attachment file path */
	for (i = 0; i < original_attachment_count; i++) {
		original_attachment_array[i].save_status = 0;
		if (original_attachment_array[i].attachment_path)
			free(original_attachment_array[i].attachment_path);
		original_attachment_array[i].attachment_path = NULL;
	}

	/* Set reference mail id */
	original_mail_data->reference_mail_id = original_mail_data->mail_id;
	original_mail_data->body_download_status = 1;

	/* Set from address */
	if (!original_mail_data->full_address_from) {
		original_mail_data->full_address_from = strdup("<abc@abc.com>");
	}

	/* Rewrite subject */
	if (original_mail_data->subject) {
		snprintf(new_subject, 4086, "Fw:%s", original_mail_data->subject);
		free(original_mail_data->subject);
		original_mail_data->subject = NULL;
	} else {
		snprintf(new_subject, 4086, "Forward test");
	}

	original_mail_data->subject = strdup(new_subject);

	/* Set mailbox information */
	if ((err = email_get_mailbox_by_mailbox_type(original_mail_data->account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &outbox)) != EMAIL_ERROR_NONE || !outbox) {
		testapp_print("email_get_mailbox_by_mailbox_type failed [%d]\n", err);
		return FALSE;
	}
	original_mail_data->mailbox_id = outbox->mailbox_id;
	original_mail_data->mailbox_type = outbox->mailbox_type;

	/* Copy body text */
	if (original_mail_data->file_path_html) {
		copy_file(original_mail_data->file_path_html, html_file_path);
		/*execl("/bin/cp", "/bin/cp", original_mail_data->file_path_html, html_file_path,(char *)0); */
		free(original_mail_data->file_path_html);
		original_mail_data->file_path_html = strdup(html_file_path);
	}

	if (original_mail_data->file_path_plain) {
		copy_file(original_mail_data->file_path_plain, plain_text_path);
		/*execl("/bin/cp", "/bin/cp", original_mail_data->file_path_plain, plain_text_path,(char *)0);*/
		free(original_mail_data->file_path_plain);
		original_mail_data->file_path_plain = strdup(plain_text_path);
	}


	/*
	body_file = fopen(body_file_path, "w");

	testapp_print("\n body_file [%p]\n", body_file);

	if (body_file == NULL) {
		testapp_print("\n fopen [%s]failed\n", body_file_path);
		return FALSE;
	}

	for (i = 0; i < 100; i++)
		fprintf(body_file, "Mail sending Test. [%d]\n", i);

	fflush(body_file);
	fclose(body_file);
	*/

	/* Add mail */
	if ((err = email_add_mail(original_mail_data, original_attachment_array, original_attachment_count, NULL, false)) != EMAIL_ERROR_NONE) {
		testapp_print("email_get_attachment_data_list failed [%d]\n", err);
		return FALSE;
	}

	/* Send mail */
	if ((err = email_send_mail_with_downloading_attachment_of_original_mail(original_mail_data->mail_id, &handle)) != EMAIL_ERROR_NONE) {
		testapp_print("email_send_mail_with_downloading_attachment_of_original_mail failed [%d]\n", err);
		return FALSE;
	}

	if (original_mail_data)
		email_free_mail_data(&original_mail_data, 1);

	if (outbox)
		email_free_mailbox(&outbox, 1);

	if (original_attachment_array)
		email_free_attachment_data(&original_attachment_array, original_attachment_count);

	return TRUE;
}

static gboolean	testapp_test_get_mail_data()
{
	int err = EMAIL_ERROR_NONE;
	int mail_id = 0;
	email_mail_data_t *mail_data = NULL;

	testapp_print("\n > Enter mail id: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	/* Get original mail */
	if ((err = email_get_mail_data(mail_id, &mail_data)) != EMAIL_ERROR_NONE || !mail_data) {
		testapp_print("email_get_mail_data failed [%d]\n", err);
		return FALSE;
	}

	testapp_print("mail_id [%d]\n", mail_data->mail_id);
	testapp_print("account_id [%d]\n", mail_data->account_id);
	testapp_print("mailbox_id [%d]\n", mail_data->mailbox_id);
	testapp_print("mailbox_type [%d]\n", mail_data->mailbox_type);
	if (mail_data->subject)
		testapp_print("subject [%s]\n", mail_data->subject);
	testapp_print("date_time [%d]\n", mail_data->date_time);
	testapp_print("server_mail_status [%d]\n", mail_data->server_mail_status);

	if (mail_data->server_mailbox_name)
		testapp_print("server_mailbox_name [%s]\n", mail_data->server_mailbox_name);

	if (mail_data->server_mail_id)
		testapp_print("server_mail_id [%s]\n", mail_data->server_mail_id);

	if (mail_data->message_id)
		testapp_print("message_id [%s]\n", mail_data->message_id);

	testapp_print("reference_mail_id [%d]\n", mail_data->reference_mail_id);

	if (mail_data->full_address_from)
		testapp_print("full_address_from [%s]\n", mail_data->full_address_from);

	if (mail_data->full_address_reply)
		testapp_print("full_address_reply [%s]\n", mail_data->full_address_reply);

	if (mail_data->full_address_to)
		testapp_print("full_address_to [%s]\n", mail_data->full_address_to);

	if (mail_data->full_address_cc)
		testapp_print("full_address_cc [%s]\n", mail_data->full_address_cc);

	if (mail_data->full_address_bcc)
		testapp_print("full_address_bcc [%s]\n", mail_data->full_address_bcc);

	if (mail_data->full_address_return)
		testapp_print("full_address_return [%s]\n", mail_data->full_address_return);

	if (mail_data->email_address_sender)
		testapp_print("email_address_sender [%s]\n", mail_data->email_address_sender);

	if (mail_data->email_address_recipient)
		testapp_print("email_address_recipient [%s]\n", mail_data->email_address_recipient);

	if (mail_data->alias_sender)
		testapp_print("alias_sender [%s]\n", mail_data->alias_sender);

	if (mail_data->alias_recipient)
		testapp_print("alias_recipient [%s]\n", mail_data->alias_recipient);

	testapp_print("body_download_status [%d]\n", mail_data->body_download_status);

	if (mail_data->file_path_plain)
		testapp_print("file_path_plain [%s]\n", mail_data->file_path_plain);

	if (mail_data->file_path_html)
		testapp_print("file_path_html [%s]\n", mail_data->file_path_html);

	testapp_print("file_path_mime_entity [%s]\n", mail_data->file_path_mime_entity);
	testapp_print("mail_size [%d]\n", mail_data->mail_size);
	testapp_print("flags_seen_field [%d]\n", mail_data->flags_seen_field);
	testapp_print("flags_deleted_field [%d]\n", mail_data->flags_deleted_field);
	testapp_print("flags_flagged_field [%d]\n", mail_data->flags_flagged_field);
	testapp_print("flags_answered_field [%d]\n", mail_data->flags_answered_field);
	testapp_print("flags_recent_field [%d]\n", mail_data->flags_recent_field);
	testapp_print("flags_draft_field [%d]\n", mail_data->flags_draft_field);
	testapp_print("flags_forwarded_field [%d]\n", mail_data->flags_forwarded_field);
	testapp_print("DRM_status [%d]\n", mail_data->DRM_status);
	testapp_print("priority [%d]\n", mail_data->priority);
	testapp_print("save_status [%d]\n", mail_data->save_status);
	testapp_print("lock_status [%d]\n", mail_data->lock_status);
	testapp_print("report_status [%d]\n", mail_data->report_status);
	testapp_print("attachment_count [%d]\n", mail_data->attachment_count);
	testapp_print("inline_content_count [%d]\n", mail_data->inline_content_count);
	testapp_print("thread_id [%d]\n", mail_data->thread_id);
	testapp_print("thread_item_count [%d]\n", mail_data->thread_item_count);

	if (mail_data->preview_text)
		testapp_print("preview_text [%s]\n", mail_data->preview_text);

	testapp_print("meeting_request_status [%d]\n", mail_data->meeting_request_status);
	testapp_print("message_class [%d]\n", mail_data->message_class);
	testapp_print("digest_type [%d]\n", mail_data->digest_type);
	testapp_print("smime_type [%d]\n", mail_data->smime_type);
	testapp_print("scheduled_sending_time [%d]\n", mail_data->scheduled_sending_time);
	testapp_print("remaining_resend_times [%d]\n", mail_data->remaining_resend_times);
	testapp_print("eas_data_length [%d]\n", mail_data->eas_data_length);

	return TRUE;
}

static gboolean	testapp_test_schedule_sending_mail()
{
	int                    added_mail_id = 0;
	int                    err = EMAIL_ERROR_NONE;
	int                    handle = 0;
	time_t                 time_to_send;

	testapp_add_mail_for_sending(&added_mail_id);

	if (added_mail_id) {
		time(&time_to_send);
		time_to_send += 60;

		testapp_print("Calling email_schedule_sending_mail...\n");

		if (email_schedule_sending_mail(added_mail_id, time_to_send) < 0) {
			testapp_print("email_schedule_sending_mail failed[%d]\n", err);
		} else {
			testapp_print("Start sending. handle[%d]\n", handle);
		}
	}

	return TRUE;
}

static gboolean	testapp_test_set_flags_field()
{
	int account_id = 0;
	int mail_id = 0;

	testapp_print("\n > Enter Account ID: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter Mail ID: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	if (email_set_flags_field(account_id, &mail_id, 1, EMAIL_FLAGS_FLAGGED_FIELD, 1, 1) < 0)
		testapp_print("email_set_flags_field failed");
	else
		testapp_print("email_set_flags_field success");

	return TRUE;
}

static gboolean testapp_test_download_body()
{
	int mail_id = 0;
	int handle = 0, err;

	testapp_print("\n > Enter Mail Id: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");
	err = email_download_body(mail_id, 0, &handle);
	if (err  < 0)
		testapp_print("email_download_body failed");
	else {
		testapp_print("email_download_body success");
		testapp_print("handle[%d]\n", handle);
	}
	return TRUE;
}


static gboolean testapp_test_cancel_download_body()
{
	int mail_id = 0;
	int account_id = 0;
	int yes = 0;
	int handle = 0;

	email_mailbox_t mailbox;
	memset(&mailbox, 0x00, sizeof(email_mailbox_t));

	testapp_print("\n > Enter account_id: ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter mail id: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	if (email_download_body(mail_id, 0, &handle) < 0)
		testapp_print("email_download_body failed");
	else {
		testapp_print("email_download_body success\n");
		testapp_print("Do u want to cancel download job>>>>>1/0\n");
		if (0 >= scanf("%d", &yes))
			testapp_print("Invalid input. ");
		if (1 == yes) {
			if (email_cancel_job(account_id, handle , EMAIL_CANCELED_BY_USER) < 0)
				testapp_print("email_cancel_job failed..!");
			else {
				testapp_print("email_cancel_job success..!");
				testapp_print("handle[%d]\n", handle);
			}
		}

	}
	return TRUE;
}
static gboolean testapp_test_download_attachment()
{
	int mail_id = 0;
	int attachment_no = 0;
	int handle = 0;

	testapp_print("\n > Enter Mail Id: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter attachment number: ");
	if (0 >= scanf("%d", &attachment_no))
		testapp_print("Invalid input. ");

	if (email_download_attachment(mail_id, attachment_no, &handle) < 0)
		testapp_print("email_download_attachment failed");
	else {
		testapp_print("email_download_attachment success");
		testapp_print("handle[%d]\n", handle);
	}
	return TRUE;

}

static gboolean testapp_test_get_attachment_data_list()
{
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int mail_id = 0;
	int test_attachment_data_count;
	email_attachment_data_t *test_attachment_data_list = NULL;

	testapp_print("\n > Enter Mail id: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	if ((err = email_get_attachment_data_list(mail_id, &test_attachment_data_list, &test_attachment_data_count)) != EMAIL_ERROR_NONE) {
		testapp_print("email_get_attachment_data_list() failed [%d]\n", err);
		goto FINISH_OFF;
	}
	if (test_attachment_data_list) {
		for (i = 0; i < test_attachment_data_count; i++) {
			testapp_print("index [%d]\n", i);
			testapp_print("attachment_name [%s]\n", test_attachment_data_list[i].attachment_name);
			testapp_print("attachment_path [%s]\n", test_attachment_data_list[i].attachment_path);
			testapp_print("attachment_size [%d]\n", test_attachment_data_list[i].attachment_size);
			testapp_print("mail_id [%d]\n", test_attachment_data_list[i].mail_id);
			testapp_print("attachment_mime_type [%s]\n", test_attachment_data_list[i].attachment_mime_type);
		}
		email_free_attachment_data(&test_attachment_data_list, test_attachment_data_count);
	}

FINISH_OFF:

	return TRUE;
}

static gboolean testapp_test_get_meeting_request()
{
	int mail_id = 0;
	int err = EMAIL_ERROR_NONE;
	email_meeting_request_t *meeting_request;

	testapp_print("\n > Enter Mail Id: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	err = email_get_meeting_request(mail_id, &meeting_request);

	testapp_print("err[%d]\n", err);

	if (err == EMAIL_ERROR_NONE && meeting_request) {
		testapp_print("mail_id [%d]\n", meeting_request->mail_id);
		testapp_print("global_object_id [%s]\n", meeting_request->global_object_id);
		testapp_print("meeting_response [%d]\n", meeting_request->meeting_response);
	}

	email_free_meeting_request(&meeting_request, 1);
	return TRUE;
}

static gboolean testapp_test_retry_send()
{
	int mail_id = 0;
	int timeout = 0;

	email_mailbox_t mailbox;
	memset(&mailbox, 0x00, sizeof(email_mailbox_t));

	testapp_print("\n > Enter Mail Id: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter timeout in seconds: ");
	if (0 >= scanf("%d", &timeout))
		testapp_print("Invalid input. ");

	if (email_retry_sending_mail(mail_id, timeout) < 0)
		testapp_print("email_retry_sending_mail failed");
	return TRUE;
}

static gboolean testapp_test_get_attachment_data()
{
	int attachment_id = 0;
	email_attachment_data_t *attachment = NULL;

	testapp_print("\n > Enter attachment id: ");
	if (0 >= scanf("%d", &attachment_id))
		testapp_print("Invalid input. ");

	email_get_attachment_data(attachment_id, &attachment);

	if (attachment) {
		testapp_print("attachment_name [%s]\n", attachment->attachment_name);
		testapp_print("attachment_path [%s]\n", attachment->attachment_path);
		testapp_print("attachment_size [%d]\n", attachment->attachment_size);
		testapp_print("mail_id [%d]\n", attachment->mail_id);
		testapp_print("attachment_mime_type [%s]\n", attachment->attachment_mime_type);
	}

	return TRUE;
}

static gboolean testapp_test_move_all_mails_to_mailbox()
{
	int err = EMAIL_ERROR_NONE;
	int src_mailbox_id;
	int dest_mailbox_id;

	testapp_print("src mailbox id> ");
	if (0 >= scanf("%d", &src_mailbox_id))
		testapp_print("Invalid input. ");

	testapp_print("dest mailbox id> ");
	if (0 >= scanf("%d", &dest_mailbox_id))
		testapp_print("Invalid input. ");

	err = email_move_all_mails_to_mailbox(src_mailbox_id, dest_mailbox_id);
	if (err < 0) {
		testapp_print("email_move_all_mails_to_mailbox failed: err[%d]\n", err);
	} else {
		testapp_print("email_move_all_mails_to_mailbox suceeded\n");
	}

	return false;
}

static gboolean testapp_test_get_totaldiskusage()
{
	unsigned long total_size = 0;
	int err_code = EMAIL_ERROR_NONE ;

	err_code = email_get_disk_space_usage(&total_size);
	if (err_code < 0)
		testapp_print("email_get_disk_space_usage failed err : %d\n", err_code);
	else
		testapp_print("email_get_disk_space_usage SUCCESS, total disk usage in KB : %ld \n", total_size);

	return FALSE;
}



static gboolean testapp_test_db_test()
{
	int err = EMAIL_ERROR_NONE;
	int mail_id;
	int account_id;
	char *to = NULL;
	char *cc = NULL;
	char *bcc = NULL;

	to = (char *)malloc(500000);
	cc = (char *)malloc(500000);
	bcc = (char *)malloc(500000);

	memset(to, 0x00, sizeof(500000));
	memset(cc, 0x00, sizeof(500000));
	memset(bcc, 0x00, sizeof(500000));

	testapp_print("Input Mail id:\n");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");
	testapp_print("Input Account id:\n");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");
	testapp_print("Input TO field:\n");
	if (0 >= scanf("%s", to))
		testapp_print("Invalid input. ");
	testapp_print("Input CC field:\n");
	if (0 >= scanf("%s", cc))
		testapp_print("Invalid input. ");
	testapp_print("Input BCC field:\n");
	if (0 >= scanf("%s", bcc))
		testapp_print("Invalid input. ");

	if (emstorage_test(NULL, mail_id, account_id, to, cc, bcc, &err) == true) {
		testapp_print(">> Saving Succeeded\n");
	} else {
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
	if (0 >= scanf("%d", &type);

	switch (type) {
		case 1:
			do  {
				for (i = 0; address_list[i] != NULL; i++) {
					testapp_print("[%d] addr[%s]\n", i, address_list[i]);
					address_count++;
				}
				testapp_print("Choose address to be tested:[99:quit]\n");
				if (0 >= scanf("%d", &index);
				if (index == 99 )
					break;

				testapp_print(">> [%d] Checking?(1:Yes, Other:No) [%s]\n", index, address_list[index]);
				if (0 >= scanf("%d", &check_yn);
				if (check_yn == 1) {
					pAddress = strdup(address_list[index]);
					if (em_verify_email_address(pAddress, false, &err ) == true) {
						testapp_print("<< [%d] Checking Succeeded : addr[%s]\n", index, address_list[index]);
					}
					else {
						testapp_print("<< [%d] Checking Failed    : addr[%s], err[%d]\n", index, address_list[index], err);
					}
					if (pAddress) {
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
			if (0 >= scanf("%s", input_address);
			if (em_verify_email_address(input_address, false, &err ) == true) {
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
	testapp_print("\n\t>>>>> email_get_max_mail_count() return [%d][%d]\n\n", max_count, err);

	return TRUE;
}

#include "email-storage.h"
static gboolean testapp_test_test_get_thread_information()
{
	int error_code, thread_id = 38;
	email_mail_list_item_t *result_mail;

	if ((error_code = email_get_thread_information_ex(thread_id, &result_mail)) == EMAIL_ERROR_NONE) {
		testapp_print("email_get_thread_information returns = %d\n", error_code);
		testapp_print("subject = %s\n", result_mail->subject);
		testapp_print("mail_id = %d\n", result_mail->mail_id);
		testapp_print("full_address_from = %s\n", result_mail->full_address_from);
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
	email_address_info_list_t *address_info_list = NULL;
	email_address_info_t *p_address_info = NULL;
	GList *list = NULL;
	GList *node = NULL;

	memset(buf, 0x00, sizeof(buf));
	testapp_print("\n > Enter mail_id: ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	email_get_address_info_list(mail_id, &address_info_list);

	testapp_print("===================================================\n");
	testapp_print("\t\t\t address info list\n");
	testapp_print("===================================================\n");
	testapp_print("address_type\t address \t\tdisplay_name\t contact id\n");
	testapp_print("===================================================\n");

	for (i = EMAIL_ADDRESS_TYPE_FROM; i <= EMAIL_ADDRESS_TYPE_BCC; i++) {
		switch (i) {
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
		while (node != NULL) {
			p_address_info = (email_address_info_t *)node->data;
			testapp_print("%d\t\t%s\t%s\t%d\n", p_address_info->address_type, p_address_info->address, p_address_info->display_name, p_address_info->contact_id);

			node = g_list_next(node);
		}
		testapp_print("\n");
	}
	testapp_print("===================================================\n");

	if (address_info_list)
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
	int search_filter_count = 0;
	email_search_filter_type search_filter_type = 0;
	email_search_filter_t search_filter[10];
	int handle = 0;
	time_t current_time = 0;
	char search_key_value_string[256];

	testapp_print("input account id : ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("input mailbox id : ");
	if (0 >= scanf("%d", &mailbox_id))
		testapp_print("Invalid input. ");

	memset(&search_filter, 0x00, sizeof(email_search_filter_t) * 10);

	while (1) {
		testapp_print(
			"	EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO       =  1, (integer type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_UID              =  2, (integer type ) \n"
			"   EMAIL_SEARCH_FILTER_TYPE_ALL              =  3, (integer type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_BCC              =  7, (string type ) \n"
			"   EMAIL_SEARCH_FILTER_TYPE_BODY             =  8, (string type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_CC               =  9, (string type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_FROM             = 10, (string type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_KEYWORD          = 11, (string type ) \n"
			"   EMAIL_SEARCH_FILTER_TYPE_TEXT             = 12, (string type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_SUBJECT          = 13, (string type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_TO               = 15, (string type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER      = 16, (integet type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER     = 17, (integet type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE = 20, (time type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON     = 21, (time type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE  = 22, (time type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED   = 26, (integer type ) \n"
			"   EMAIL_SEARCH_FILTER_TYPE_FLAGS_NEW        = 27, (integer type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED    = 28, (integer type ) \n"
			"   EMAIL_SEARCH_FILTER_TYPE_FLAGS_OLD        = 29, (integer type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT      = 30, (integer type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED     = 32, (integer type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT     = 34, (integer type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN       = 36, (integer type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID       = 43, (string type ) \n"
			"   EMAIL_SEARCH_FILTER_TYPE_HEADER_PRIORITY  = 50, (integer type ) \n"
			"	EMAIL_SEARCH_FILTER_TYPE_ATTACHMENT_NAME  = 60, (string type ) \n"
			"   EMAIL_SEARCH_FILTER_TYPE_CHARSET          = 61, (string type ) \n"
			"   EMAIL_SEARCH_FILTER_TYPE_USER_DEFINED     = 62, (string type ) \n"
			"	END                                       = 0 \n");

		testapp_print("input search filter type : ");
		if (0 >= scanf("%d", (int *)&search_filter_type))
			testapp_print("Invalid input. ");

		search_filter[search_filter_count].search_filter_type = search_filter_type;

		switch (search_filter_type) {
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_NO:
			case EMAIL_SEARCH_FILTER_TYPE_UID:
			case EMAIL_SEARCH_FILTER_TYPE_ALL:
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_LARSER:
			case EMAIL_SEARCH_FILTER_TYPE_SIZE_SMALLER:
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_ANSWERED:
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_NEW:
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DELETED:
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_OLD:
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_DRAFT:
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_FLAGED:
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_RECENT:
			case EMAIL_SEARCH_FILTER_TYPE_FLAGS_SEEN:
			case EMAIL_SEARCH_FILTER_TYPE_HEADER_PRIORITY:
				testapp_print("input search filter key value : ");
				if (0 >= scanf("%d", &search_key_value_integer))
					testapp_print("Invalid input. ");
				search_filter[search_filter_count].search_filter_key_value.integer_type_key_value = search_key_value_integer;
				break;

			case EMAIL_SEARCH_FILTER_TYPE_BCC:
			case EMAIL_SEARCH_FILTER_TYPE_BODY:
			case EMAIL_SEARCH_FILTER_TYPE_CC:
			case EMAIL_SEARCH_FILTER_TYPE_FROM:
			case EMAIL_SEARCH_FILTER_TYPE_KEYWORD:
			case EMAIL_SEARCH_FILTER_TYPE_TEXT:
			case EMAIL_SEARCH_FILTER_TYPE_SUBJECT:
			case EMAIL_SEARCH_FILTER_TYPE_TO:
			case EMAIL_SEARCH_FILTER_TYPE_MESSAGE_ID:
			case EMAIL_SEARCH_FILTER_TYPE_ATTACHMENT_NAME:
			case EMAIL_SEARCH_FILTER_TYPE_CHARSET:
			case EMAIL_SEARCH_FILTER_TYPE_USER_DEFINED:
				testapp_print("input search filter key value : ");
				int readn = read(0, search_key_value_string, 256);
				if (readn < 0)
					testapp_print("Invalid input. ");

				search_filter[search_filter_count].search_filter_key_value.string_type_key_value = strndup(search_key_value_string, readn - 1);
				break;

			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_BEFORE:
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_ON:
			case EMAIL_SEARCH_FILTER_TYPE_SENT_DATE_SINCE:
				time(&current_time);
				/* TODO : write codes for converting string to time */
				/* search_filter.search_filter_key_value.time_type_key_value = search_key_value_string; */
				search_filter[search_filter_count].search_filter_key_value.time_type_key_value = current_time;
				break;
			default:
				testapp_print("END filter type [%d]", search_filter_type);
				break;
		}

		if (!search_filter_type)
			break;

		search_filter_count++;
	}

	if ((err_code = email_search_mail_on_server(account_id, mailbox_id, search_filter, search_filter_count, &handle)) != EMAIL_ERROR_NONE) {
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
	char                   arg[50] = { 0 , };
	char                  *body_file_path = MAIL_TEMP_BODY;
	email_mailbox_t         *mailbox_data = NULL;
	email_mail_data_t       *test_mail_data = NULL;
	email_attachment_data_t *attachment_data = NULL;
	email_meeting_request_t *meeting_req = NULL;
	FILE                  *body_file = NULL;

	testapp_print("\n > Enter account id : ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	email_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_SEARCH_RESULT, &mailbox_data);

	test_mail_data = malloc(sizeof(email_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));

	testapp_print("\n From EAS? [0/1]> ");
	if (0 >= scanf("%d", &from_eas))
		testapp_print("Invalid input. ");

	test_mail_data->account_id             = account_id;
	test_mail_data->save_status            = 1;
	test_mail_data->body_download_status   = 1;
	test_mail_data->flags_seen_field       = 1;
	test_mail_data->file_path_plain        = strdup(body_file_path);
	test_mail_data->mailbox_id             = mailbox_data->mailbox_id;
	test_mail_data->mailbox_type           = mailbox_data->mailbox_type;
	test_mail_data->full_address_from      = strdup("<test1@test.com>");
	test_mail_data->full_address_to        = strdup("<test2@test.com>");
	test_mail_data->full_address_cc        = strdup("<test3@test.com>");
	test_mail_data->full_address_bcc       = strdup("<test4@test.com>");
	test_mail_data->subject                = strdup("Into search result mailbox");
	test_mail_data->remaining_resend_times = 3;

	body_file = fopen(body_file_path, "w");

	for (i = 0; i < 500; i++)
		fprintf(body_file, "X2 X2 X2 X2 X2 X2 X2");
	fflush(body_file);
	fclose(body_file);

	testapp_print(" > Attach file? [0/1] : ");
	if (0 >= scanf("%d", &attachment_count))
		testapp_print("Invalid input. ");

	if (attachment_count) {
		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment name : ");
		if (0 >= scanf("%s", arg))
			testapp_print("Invalid input. ");

		attachment_data = malloc(sizeof(email_attachment_data_t));

		attachment_data->attachment_name  = strdup(arg);

		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment absolute path : ");
		if (0 >= scanf("%s", arg))
			testapp_print("Invalid input. ");

		attachment_data->attachment_path  = strdup(arg);
		attachment_data->save_status      = 1;
		test_mail_data->attachment_count  = attachment_count;
	}

	testapp_print("\n > Meeting Request? [0: no, 1: yes(request from server), 2: yes(response from local)]");
	if (0 >= scanf("%d", (int *)&(test_mail_data->meeting_request_status)))
		testapp_print("Invalid input. ");

	if (test_mail_data->meeting_request_status == 1
		|| test_mail_data->meeting_request_status == 2) {
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

	if ((err = email_add_mail(test_mail_data, attachment_data, attachment_count, meeting_req, from_eas)) != EMAIL_ERROR_NONE)
		testapp_print("email_add_mail failed. [%d]\n", err);
	else
		testapp_print("email_add_mail success.\n");

	testapp_print("saved mail id = [%d]\n", test_mail_data->mail_id);

	if (attachment_data)
		email_free_attachment_data(&attachment_data, attachment_count);

	if (meeting_req)
		email_free_meeting_request(&meeting_req, 1);

	email_free_mail_data(&test_mail_data, 1);
	email_free_mailbox(&mailbox_data, 1);

	return FALSE;
}

static gboolean testapp_test_email_parse_mime_file()
{
	int i = 0;
	int attachment_count = 0;
	int output_attachment_count = 0;
	int verify = 0;
	int err = EMAIL_ERROR_NONE;
	char eml_file_path[255] = {0, };
        struct tm *struct_time;

	email_mail_data_t *mail_data = NULL;
	email_attachment_data_t *mail_attachment_data = NULL;
	email_mail_data_t *output_mail_data = NULL;
	email_attachment_data_t *output_mail_attachment_data = NULL;

	testapp_print("Input eml file path : ");
	if (0 >= scanf("%s", eml_file_path))
		testapp_print("Invalid input. ");

	if ((err = email_parse_mime_file(eml_file_path, &mail_data, &mail_attachment_data, &attachment_count)) != EMAIL_ERROR_NONE) {
		testapp_print("email_parse_mime_file failed : [%d]\n", err);
		return false;
	}

        testapp_print("load success\n");

        struct_time = localtime(&(mail_data->date_time));

        testapp_print("%4d year", struct_time->tm_year +1900);
        testapp_print("%2d month(0-11)\n", struct_time->tm_mon + 1);
        testapp_print("%2d day(1-31)\n", struct_time->tm_mday);
        testapp_print("%2d wday\n", struct_time->tm_wday);
        testapp_print("%2d hour(0-23)\n", struct_time->tm_hour);
        testapp_print("%2d minutes(0-59)\n", struct_time->tm_min);
        testapp_print("%2d second(0-59)\n", struct_time->tm_sec);
        testapp_print("year day %3d\n", struct_time->tm_yday);

	testapp_print("Return-Path: %s\n", mail_data->full_address_return);
	testapp_print("To: %s\n", mail_data->full_address_to);
	testapp_print("Subject: %s\n", mail_data->subject);
	testapp_print("Priority: %d\n", mail_data->priority);
	testapp_print("From: %s\n", mail_data->full_address_from);
	testapp_print("Reply-To: %s\n", mail_data->full_address_reply);
	testapp_print("Sender: %s\n", mail_data->email_address_sender);
	testapp_print("Message-ID: %s\n", mail_data->message_id);
	testapp_print("attachment_count: %d\n", mail_data->attachment_count);
	testapp_print("SMIME type : %d\n", mail_data->smime_type);
	testapp_print("inline content count : %d\n", mail_data->inline_content_count);
	testapp_print("mail_size : %d\n", mail_data->mail_size);
	testapp_print("download_body_status : %d\n", mail_data->body_download_status);


	for (i = 0; i < attachment_count ; i++) {
		testapp_print("%d attachment\n", i);
		testapp_print("attachment_id: %d\n", mail_attachment_data[i].attachment_id);
		testapp_print("attachment_size: %d\n", mail_attachment_data[i].attachment_size);
		testapp_print("inline_attachment_status: %d\n", mail_attachment_data[i].inline_content_status);
		testapp_print("attachment_name: %s\n", mail_attachment_data[i].attachment_name);
		testapp_print("attachment_path: %s\n", mail_attachment_data[i].attachment_path);
		testapp_print("attachment_save_status: %d\n", mail_attachment_data[i].save_status);
		testapp_print("content_id: %s\n", mail_attachment_data[i].content_id);
		testapp_print("mailbox_id: %d\n", mail_attachment_data[i].mailbox_id);
	}

	testapp_print("Success : Open eml file\n");

	if (mail_data->smime_type != EMAIL_SMIME_NONE) {
		if (mail_data->smime_type == EMAIL_SMIME_SIGNED || mail_data->smime_type == EMAIL_PGP_SIGNED) {
			if (!email_verify_signature_ex(mail_data, mail_attachment_data, attachment_count, &verify))
				testapp_print("email_verify_signature_ex failed\n");
		} else {
			if ((err = email_get_decrypt_message_ex(mail_data,
													mail_attachment_data,
													attachment_count,
													&output_mail_data,
													&output_mail_attachment_data,
													&output_attachment_count,
													&verify)) != EMAIL_ERROR_NONE)
				testapp_print("email_get_decrypt_message_ex failed\n");
		}

		testapp_print("verify : [%d]\n", verify);
	}

	if ((err = email_delete_parsed_data(mail_data)) != EMAIL_ERROR_NONE) {
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

static gboolean testapp_test_email_write_mime_file()
{
	int err = EMAIL_ERROR_NONE;
	int mail_id = 0;
	int attachment_count = 0;
	char *file_path = NULL;
	email_mail_data_t *mail_data = NULL;
	email_attachment_data_t *mail_attachment_data = NULL;


	file_path = malloc(512);
	memset(file_path, 0x00, 512);

	testapp_print("Input mail id : ");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	err = email_get_mail_data(mail_id, &mail_data);
	if (err != EMAIL_ERROR_NONE || mail_data == NULL) {
		testapp_print("email_get_mail_data failed : [%d]\n", err);
		return false;
	}

	err = email_get_attachment_data_list(mail_id, &mail_attachment_data, &attachment_count);
	if (err != EMAIL_ERROR_NONE) {
		testapp_print("email_get_attachment_data_list failed : [%d]\n", err);
		return false;
	}

	snprintf(file_path, 512, tzplatform_mkpath(TZ_SYS_DATA,"email/.email_data/tmp/%d_%8d.eml"), mail_id, (int)time(NULL));

	err = email_write_mime_file(mail_data, mail_attachment_data, attachment_count, &file_path);
	if (err != EMAIL_ERROR_NONE) {
		testapp_print("email_write_mime_file failed : [%d]\n", err);
		return false;
	}

	testapp_print("file path : [%s]\n", file_path);

	if (mail_data)
		email_free_mail_data(&mail_data, 1);

	if (mail_attachment_data)
		email_free_attachment_data(&mail_attachment_data, attachment_count);

	if (file_path)
		free(file_path);

	return true;
}

static gboolean testapp_test_smime_verify_signature()
{
	int mail_id = 0;
	int verify = 0;
	int err = EMAIL_ERROR_NONE;

	testapp_print("input mail_id :");
	if (0 >= scanf("%d", &mail_id))
		testapp_print("Invalid input. ");

	err = email_verify_signature(mail_id, &verify);
	if (err != EMAIL_ERROR_NONE) {
		testapp_print("Failed Verify\n");
		return false;
	}

	testapp_print("verify value : [%d]\n", verify);
	return true;
}

static gboolean testapp_test_email_add_mail_with_multiple_recipient()
{
	int                    i = 0;
	int                    account_id = 0;
	int                    err = EMAIL_ERROR_NONE;
//	int                    smime_type = 0;
	int                    recipient_addresses_count = 0;
	char                  *recipient_addresses = NULL;
	char                   recipient_address[234] = { 0, };
	char                   from_address[300] = { 0 , };
	const char            *body_file_path = MAIL_TEMP_BODY;
	email_account_t       *account_data = NULL;
	email_mailbox_t       *mailbox_data = NULL;
	email_mail_data_t     *test_mail_data = NULL;
	FILE                  *body_file;

	testapp_print("\n > Enter account id : ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter recipient address count : ");
	if (0 >= scanf("%d", &recipient_addresses_count))
		testapp_print("Invalid input. ");

	if (recipient_addresses_count < 1)
		return TRUE;

	recipient_addresses = malloc(sizeof(char) * 234 * recipient_addresses_count);

	for (i = 0; i < recipient_addresses_count; i++) {
		snprintf(recipient_address, 234, "\"mailtest%05d\" <mailtest%05d@a1234567890b1234567890.com>; ", i, i);
		strcat(recipient_addresses, recipient_address);
	}

	email_get_account(account_id, GET_FULL_DATA_WITHOUT_PASSWORD, &account_data);

	email_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &mailbox_data);

	test_mail_data = malloc(sizeof(email_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));

	SNPRINTF(from_address, 300, "<%s>", account_data->user_email_address);

	test_mail_data->account_id             = account_id;
	test_mail_data->save_status            = EMAIL_MAIL_STATUS_SEND_DELAYED;
	test_mail_data->body_download_status   = 1;
	test_mail_data->flags_seen_field       = 1;
	test_mail_data->file_path_plain        = strdup(body_file_path);
	test_mail_data->mailbox_id             = mailbox_data->mailbox_id;
	test_mail_data->mailbox_type           = mailbox_data->mailbox_type;
	test_mail_data->full_address_from      = strdup(from_address);
	test_mail_data->full_address_to        = recipient_addresses;
	test_mail_data->subject                = strdup("Read receipt request from TIZEN");
	test_mail_data->remaining_resend_times = 3;
	test_mail_data->report_status          = EMAIL_MAIL_REQUEST_DSN | EMAIL_MAIL_REQUEST_MDN;
	test_mail_data->smime_type             = 0;

	body_file = fopen(body_file_path, "w");

	testapp_print("\n body_file [%p]\n", body_file);

	if (body_file == NULL) {
		testapp_print("\n fopen [%s]failed\n", body_file_path);
		return FALSE;
	}

	for (i = 0; i < 100; i++)
		fprintf(body_file, "Mail sending Test. [%d]\n", i);

	fflush(body_file);
	fclose(body_file);


	if ((err = email_add_mail(test_mail_data, NULL, 0, NULL, 0)) != EMAIL_ERROR_NONE)
		testapp_print("email_add_mail failed. [%d]\n", err);
	else
		testapp_print("email_add_mail success.\n");

	testapp_print("saved mail id = [%d]\n", test_mail_data->mail_id);

	email_free_mail_data(&test_mail_data, 1);
	email_free_mailbox(&mailbox_data, 1);
	email_free_account(&account_data, 1);

	return FALSE;
}

static gboolean testapp_test_send_mails_every_x_minutes()
{
	int                    added_mail_id = 0;
	int                    err = EMAIL_ERROR_NONE;
	int                    handle = 0;
	time_t                 time_to_send;
	int                    i = 0;
	int                    j = 0;
	int                    account_id = 0;
	int                    send_interval_in_minutes = 0;
	int                    number_of_mails = 0;
	char                   recipient_address[300] = { 0 , };
	char                   from_address[300] = { 0 , };
	char                   subject_form[1024] = { 0 , };
	const char            *body_file_path = MAIL_TEMP_BODY;
	struct tm             *time_to_send_tm;
	email_account_t       *account_data = NULL;
	email_mailbox_t       *mailbox_data = NULL;
	email_mail_data_t     *test_mail_data = NULL;
	FILE                  *body_file;

	testapp_print("\n > Enter account id : ");
	if (0 >= scanf("%d", &account_id))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter recipient address : ");
	if (0 >= scanf("%s", recipient_address))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter number of mails: ");
	if (0 >= scanf("%d", &number_of_mails))
		testapp_print("Invalid input. ");

	testapp_print("\n > Enter send interval in minutes: ");
	if (0 >= scanf("%d", &send_interval_in_minutes))
		testapp_print("Invalid input. ");

	email_get_account(account_id, GET_FULL_DATA_WITHOUT_PASSWORD, &account_data);

	email_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_OUTBOX, &mailbox_data);

	test_mail_data = malloc(sizeof(email_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));

	SNPRINTF(from_address, 300, "<%s>", account_data->user_email_address);

	test_mail_data->account_id             = account_id;
	test_mail_data->save_status            = EMAIL_MAIL_STATUS_SEND_DELAYED;
	test_mail_data->body_download_status   = 1;
	test_mail_data->flags_seen_field       = 1;
	test_mail_data->file_path_plain        = strdup(body_file_path);
	test_mail_data->mailbox_id             = mailbox_data->mailbox_id;
	test_mail_data->mailbox_type           = mailbox_data->mailbox_type;
	test_mail_data->full_address_from      = strdup(from_address);
	test_mail_data->full_address_to        = strdup(recipient_address);
	test_mail_data->remaining_resend_times = 3;
	test_mail_data->report_status          = EMAIL_MAIL_REPORT_NONE;


	for (i = 0; i < number_of_mails; i++) {
		if (test_mail_data->subject)
			free(test_mail_data->subject);

		time(&time_to_send);
		time_to_send += (60 * send_interval_in_minutes) * (i + 1);
		time_to_send_tm = localtime(&time_to_send);

		strftime(subject_form, 1024, "[%H:%M] TEST.", time_to_send_tm);
		test_mail_data->subject = strdup(subject_form);

		body_file = fopen(body_file_path, "w");

		testapp_print("\n body_file [%p]\n", body_file);

		if (body_file == NULL) {
			testapp_print("\n fopen [%s]failed\n", body_file_path);
			return FALSE;
		}

		for (j = 0; j < 100; j++)
			fprintf(body_file, "Mail sending Test. [%d]\n", j);

		fflush(body_file);
		fclose(body_file);

		if ((err = email_add_mail(test_mail_data, NULL, 0, NULL, 0)) != EMAIL_ERROR_NONE) {
			testapp_print("email_add_mail failed. [%d]\n", err);
			added_mail_id = 0;
		} else {
			testapp_print("email_add_mail success.\n");
			added_mail_id = test_mail_data->mail_id;
		}

		testapp_print("saved mail id = [%d]\n", added_mail_id);

		if (added_mail_id) {
			testapp_print("Calling email_schedule_sending_mail...\n");

			if (email_schedule_sending_mail(added_mail_id, time_to_send) < 0) {
				testapp_print("email_schedule_sending_mail failed[%d]\n", err);
			} else  {
				testapp_print("Start sending. handle[%d]\n", handle);
			}
		}
	}

	email_free_mail_data(&test_mail_data, 1);
	email_free_mailbox(&mailbox_data, 1);
	email_free_account(&account_data, 1);

	return TRUE;
}

/* internal functions */
static gboolean testapp_test_interpret_command(int menu_number)
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
			testapp_test_get_mail_list_ex();
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
		case 7:
			testapp_test_send_read_receipt();
			break;
		case 8:
			testapp_test_delete_attachment();
			break;
		case 9:
			testapp_test_count();
			break;
		case 10:
			testapp_test_move_mails_to_mailbox_of_another_account();
			break;
		case 11:
			testapp_test_send_mail_with_downloading_attachment_of_original_mail();
			break;
		case 12:
			testapp_test_get_mail_data();
			break;
		case 13:
			testapp_test_schedule_sending_mail();
			break;
		case 14:
			testapp_test_delete();
			break;
		case 15:
			testapp_test_update_mail_attribute();
			break;
 		case 16:
			testapp_test_download_body();
			break;
		case 17:
			testapp_test_download_attachment();
			break;
		case 18:
			testapp_test_get_attachment_data_list();
			break;
		case 19:
			testapp_test_get_meeting_request();
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
 		case 24:
			testapp_test_get_attachment_data();
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
			testapp_test_get_mail_list();
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
			testapp_test_email_parse_mime_file();
			break;
		case 61:
			testapp_test_email_write_mime_file();
			break;
		case 62:
			testapp_test_smime_verify_signature();
			break;
		case 63:
			testapp_test_email_add_mail_with_multiple_recipient();
			break;
		case 64:
			testapp_test_send_mails_every_x_minutes();
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

	while (go_to_loop) {
		testapp_show_menu(EMAIL_MAIL_MENU);
		testapp_show_prompt(EMAIL_MAIL_MENU);

		if (0 >= scanf("%d", &menu_number))
			testapp_print("Invalid input");

		go_to_loop = testapp_test_interpret_command(menu_number);
	}
}

