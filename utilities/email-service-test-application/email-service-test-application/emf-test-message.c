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

/* open header */
#include <glib.h>
#include <time.h>

#include "Emf_Mapi_Message.h"
#include "Emf_Mapi_Network.h"
#include "Emf_Mapi_Mailbox.h"

/* internal header */
#include "emf-test-utility.h"
#include "emf-test-message.h"
#include "em-core-utils.h"

#define MAIL_TEMP_BODY "/tmp/mail.txt"
#define NO_OF_TESTS    40

/*
static void testapp_test_print_sorting_menu()
{
   	testapp_print("   EMF_SORT_DATETIME_HIGH = 0\n");
   	testapp_print("   EMF_SORT_DATETIME_LOW = 1\n");	
   	testapp_print("   EMF_SORT_SENDER_HIGH = 2\n");
   	testapp_print("   EMF_SORT_SENDER_LOW = 3\n");   
   	testapp_print("   EMF_SORT_RCPT_HIGH = 4\n");
   	testapp_print("   EMF_SORT_RCPT_LOW = 5\n");    
   	testapp_print("   EMF_SORT_SUBJECT_HIGH = 6\n");
   	testapp_print("   EMF_SORT_SUBJECT_LOW = 7\n");   
   	testapp_print("   EMF_SORT_PRIORITY_HIGH = 8\n");
   	testapp_print("   EMF_SORT_PRIORITY_LOW = 9\n");   
   	testapp_print("   EMF_SORT_ATTACHMENT_HIGH = 10\n");
   	testapp_print("   EMF_SORT_ATTACHMENT_LOW = 11\n");   
   	testapp_print("   EMF_SORT_FAVORITE_HIGH = 12\n");
   	testapp_print("   EMF_SORT_FAVORITE_LOW = 13\n");   
}

static void testapp_test_print_mail_list_item(emf_mail_list_item_t *mail_list_item, int count)
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
		if (  mail_list_item[i].datetime != NULL ) {
			testapp_print(" >>> datetime [ %s ] \n", mail_list_item[i].datetime);
		}
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

static gboolean testapp_test_send_html_mail_with_inline_content()
{
	int   account_id = 0, i = 0, from_composer = 1;
	int   err_code = EMF_ERROR_NONE;
	unsigned handle = 0;
	FILE *p_file = NULL;
	char *p_plain_text_file_path = "/opt/data/email/.emfdata/tmp/mail.txt";
	char *p_html_text_file_path  = "/opt/data/email/.emfdata/tmp/mail.html";
	char *p_attachment_file_path = "/opt/media/Images and videos/Wallpapers/Default.png";
	char *p_attachment_name      = "Default.png";
	emf_mail_t            *mail = NULL;
	emf_mailbox_t         *mailbox = NULL;
	emf_attachment_info_t *attachment = NULL;
	emf_option_t           option;

	
	testapp_print("\n > Enter account no:");
	scanf("%d", &account_id);

	mail = malloc(sizeof(emf_mail_t));
	memset(mail, 0x00, sizeof(emf_mail_t));

	mail->info = malloc(sizeof(emf_mail_info_t));
	memset(mail->info, 0x00, sizeof(emf_mail_info_t));


	mail->info->account_id = account_id;
	mail->info->extra_flags.text_download_yn = 1;
	mail->info->flags.seen = 1;
	mail->info->account_id = account_id;

	mail->head = malloc(sizeof(emf_mail_head_t));
	memset(mail->head, 0x00, sizeof(emf_mail_head_t));

	mail->head->to      = strdup("<kyuho.jo@samsung.com>"); 
	mail->head->cc      = strdup("<slpmany@gmail.com>");
	mail->head->bcc     = strdup("<samsungtest09@gmail.com>");
	mail->head->from    = strdup("<slpmany@gmail.com>");
	mail->head->subject = strdup("HTML Inline");
	
	mail->body = malloc(sizeof(emf_mail_body_t));
	memset(mail->body, 0x00, sizeof(emf_mail_body_t));
	mail->body->plain = strdup(p_plain_text_file_path);
	mail->body->html  = strdup(p_html_text_file_path);

	mailbox = malloc(sizeof(emf_mailbox_t));
	if(!mailbox) {
		testapp_print("malloc failed\n");
		email_free_mail(&mail, 1);
		return false;
	}
	memset(mailbox, 0x00, sizeof(emf_mailbox_t));
	mailbox->name = strdup("INBOX");
	mailbox->account_id = account_id;
	
	attachment = malloc(sizeof(emf_attachment_info_t));
	memset(attachment, 0x00, sizeof(emf_attachment_info_t));
	attachment->name = strdup(p_attachment_name);
	attachment->savename = strdup(p_attachment_file_path);
	attachment->downloaded = 1;
	attachment->inline_content = 1;
	attachment->next = NULL;
	
	mail->body->attachment = attachment;
	mail->body->attachment_num = 1;

	p_file = fopen(p_plain_text_file_path, "w");

	if(!p_file) {
		testapp_print("fopen failed\n");
		email_free_mail(&mail, 1);
		email_free_mailbox(&mailbox, 1);		
		return false;
	}
	for(i = 0; i < 50; i++)
    	fprintf(p_file, "X2 X2 X2 X2 X2 X2 X2");
    fclose(p_file);

	p_file = fopen(p_html_text_file_path, "w");
	if(!p_file) {
		testapp_print("fopen failed\n");
		email_free_mail(&mail, 1);
		email_free_mailbox(&mailbox, 1);
		return false;
	}
   	fprintf(p_file, "<HTML><HEAD></HEAD>\r\n<BODY>\r\n<IMG SRC = \"%s\"></IMG>\r\n</BODY></HTML>", p_attachment_name);
    fclose(p_file);

	if((err_code = email_add_message(mail, mailbox, from_composer)) < 0)
		testapp_print("email_add_message failed. [%d]\n", err_code);
	else
		testapp_print("email_add_message success\n");

	testapp_print("saved mail id = [%d]\n", mail->info->uid);

	memset(&option, 0x00, sizeof(emf_option_t));
	option.keep_local_copy = 1;
	err_code = email_send_mail(mailbox, mail->info->uid, &option, &handle);
	if( err_code < 0)
		testapp_print("   fail sending [%d]\n", err_code);
	else 
		testapp_print("   finish sending. handle[%d]\n", handle);

	email_free_mail(&mail, 1);
	email_free_mailbox(&mailbox, 1);

	return true;
}

static char *testapp_test_generate_random_string(int length)
{
	int i, string_length = length + 10,  random_number, loop_cnt = length / 8;
	char *result_string = NULL, temp_string[20] = { 0, };

	result_string = em_core_malloc(string_length);

	if (!result_string) {
		return NULL;
	}
	
	memset(result_string, 0, string_length);
	srand(time(NULL));

	for(i = 0; i < loop_cnt; i++) { 
		random_number = rand() * rand();
		snprintf(temp_string, 20, "%08x", random_number);
		strcat(result_string, temp_string);
	}
	
	return result_string;
}

static gboolean testapp_test_add_mail ()
{
	int                    i = 0;
	int                    account_id = 0;
	int                    sync_server = 0;
	int                    attachment_count = 0;
	int                    err = EMF_ERROR_NONE;
	char                   arg[50] = { 0 , };
	char                  *body_file_path = "/opt/data/email/.emfdata/tmp/mail.txt";
	emf_mailbox_t         *mailbox_data = NULL;
	emf_mail_data_t       *test_mail_data = NULL;
	emf_attachment_data_t *attachment_data = NULL;
	emf_meeting_request_t *meeting_req = NULL;
	FILE                  *body_file;
 
	testapp_print("\n > Enter account id : ");
	scanf("%d", &account_id);

	
	memset(arg, 0x00, 50);
	testapp_print("\n > Enter mailbox name : ");
	scanf("%s", arg);	

	email_get_mailbox_by_name(account_id, arg, &mailbox_data);

	test_mail_data = malloc(sizeof(emf_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(emf_mail_data_t));
	
	testapp_print("\n Sync server? [0/1]> ");
	scanf("%d", &sync_server);

	test_mail_data->account_id        = account_id;
	test_mail_data->save_status       = 1;
	test_mail_data->flags_seen_field  = 1;
	test_mail_data->file_path_plain   = strdup(body_file_path);
	test_mail_data->mailbox_name      = strdup(mailbox_data->name);
	test_mail_data->mailbox_type      = mailbox_data->mailbox_type;
	test_mail_data->full_address_from = strdup("<test1@test.com>");
	test_mail_data->full_address_to   = strdup("<test2@test.com>"); 
	test_mail_data->full_address_cc   = strdup("<test3@test.com>");
	test_mail_data->full_address_bcc  = strdup("<test4@test.com>");
	test_mail_data->subject           = strdup("Meeting request mail");

	body_file = fopen(body_file_path, "w");
		
	for(i = 0; i < 500; i++)
		fprintf(body_file, "X2 X2 X2 X2 X2 X2 X2");
	fflush(body_file);
  fclose(body_file);
	
	testapp_print(" > Attach file? [0/1] : ");
	scanf("%d",&attachment_count);
	
	if ( attachment_count )  {
		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment name : ");
		scanf("%s", arg);

		attachment_data = malloc(sizeof(emf_attachment_data_t));
		
		attachment_data->attachment_name  = strdup(arg);
		
		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment absolute path : ");
		scanf("%s",arg);		
		
		attachment_data->attachment_path  = strdup(arg);
		attachment_data->save_status      = 1;
		test_mail_data->attachment_count  = attachment_count;
	}
	
	testapp_print("\n > Meeting Request? [0: no, 1: yes (request from server), 2: yes (response from local)]");
	scanf("%d", &(test_mail_data->meeting_request_status));
	
	if ( test_mail_data->meeting_request_status == 1 
		|| test_mail_data->meeting_request_status == 2 ) {
		time_t current_time;
		/*  dummy data for meeting request */
		meeting_req = malloc(sizeof(emf_meeting_request_t));
		memset(meeting_req, 0x00, sizeof(emf_meeting_request_t));
		
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
	
	if((err = email_add_mail(test_mail_data, attachment_data, attachment_count, meeting_req, sync_server)) != EMF_ERROR_NONE)
		testapp_print("email_add_mail failed. [%d]\n", err);
	else
		testapp_print("email_add_mail success.\n");

	if(attachment_data)
		email_free_attachment_data(&attachment_data, attachment_count);
		
	if(meeting_req)
		email_free_meeting_request(&meeting_req, 1);
		
	email_free_mail_data(&test_mail_data, 1);
	email_free_mailbox(&mailbox_data, 1);
	
	testapp_print("saved mail id = [%d]\n", test_mail_data->mail_id);

	return FALSE;
}

static gboolean testapp_test_update_mail()
{
	int                    mail_id = 0;
	int                    err = EMF_ERROR_NONE;
	int                    test_attachment_data_count = 0;
	char                   arg[50];
	emf_mail_data_t       *test_mail_data = NULL;
	emf_attachment_data_t *test_attachment_data_list = NULL;
	emf_meeting_request_t *meeting_req = NULL;
	
	testapp_print("\n > Enter mail id : ");
	scanf("%d", &mail_id);

	email_get_mail_data(mail_id, &test_mail_data);

	testapp_print("\n > Enter Subject: ");
	scanf("%s", arg);	

	test_mail_data->subject= strdup(arg);

	

	if (test_mail_data->attachment_count > 0) {
		if ( (err = email_get_attachment_data_list(mail_id, &test_attachment_data_list, &test_attachment_data_count)) != EMF_ERROR_NONE ) {
			testapp_print("email_get_meeting_request() failed [%d]\n", err);
			return FALSE;
		}
	}	

	if ( test_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_REQUEST 
		|| test_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_RESPONSE 
		|| test_mail_data->meeting_request_status == EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		
		if ( (err = email_get_meeting_request(mail_id, &meeting_req)) != EMF_ERROR_NONE ) {
			testapp_print("email_get_meeting_request() failed [%d]\n", err);
			return FALSE;
		}
	
		testapp_print("\n > Enter meeting response: ");
		scanf("%d", (int*)&(meeting_req->meeting_response));
	}
	
	if ( (err = email_update_mail(test_mail_data, test_attachment_data_list, test_attachment_data_count, meeting_req, 0)) != EMF_ERROR_NONE) 
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


static gboolean testapp_test_save_mail ()
{
	int account_id=0;
	int i, j, from_composer = 0, mail_count = 0;
	int attach_num = 0;
	int err;
	emf_mailbox_t *mailbox = NULL;
	emf_mail_t *mail = malloc(sizeof(emf_mail_t));
	emf_attachment_info_t attachment;
	char arg[50], *temp_string_1 = NULL, *temp_string_2 = NULL, temp_address[512] = {0, };
	emf_meeting_request_t *meeting_req = NULL;

	testapp_print("\n > Enter account no:");
	scanf("%d", &account_id);
	memset(mail, 0x00, sizeof(emf_mail_t));
	mail->info = malloc(sizeof(emf_mail_info_t));
	memset(mail->info, 0x00, sizeof(emf_mail_info_t));
	mail->info->account_id = account_id;
	mail->info->extra_flags.text_download_yn = 1;
	mail->info->flags.seen = 1;
	mail->info->account_id = account_id;

	mail->head = malloc(sizeof(emf_mail_head_t));
	memset(mail->head, 0x00, sizeof(emf_mail_head_t));
	
	mail->body = malloc(sizeof(emf_mail_body_t));
	memset(mail->body, 0x00, sizeof(emf_mail_body_t));
	
	FILE *pFile;
    char *pFilePath = "/opt/data/email/.emfdata/tmp/mail.txt";
    
	mail->body->plain = strdup(pFilePath);

	mailbox = malloc(sizeof(emf_mailbox_t));
	memset(mailbox, 0x00, sizeof(emf_mailbox_t));

	memset(arg, 0x00, 50);
	testapp_print("\n > Enter Mailbox:");
	scanf("%s", arg);	
		
	mailbox->name = strdup(arg);
	mailbox->account_id = account_id;
	
	testapp_print(" Attach file 1/0 > ");
	scanf("%d",&attach_num);
	if ( attach_num )  {
		memset(&attachment, 0x00, sizeof(emf_attachment_info_t));
		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment name: ");
		scanf("%s",arg);
		
		attachment.name = strdup(arg);
		
		memset(arg, 0x00, 50);
		testapp_print("\n > Enter attachment absolute path: ");
		scanf("%s",arg);		
		
		attachment.savename = strdup(arg);
		attachment.downloaded = 1;
		attachment.next = NULL;
		
		mail->body->attachment = &attachment;
		mail->body->attachment_num = attach_num;
	}
	/* set flag1 as seen */
	
	
	testapp_print("\n From composer? (0/1)> ");
	scanf("%d", &from_composer);

	testapp_print("\n mail count? (n)> ");
	scanf("%d", &mail_count);
	
	testapp_print("\n > Meeting Request? (0: no, 1: yes (request from server), 2: yes (response from local) )");
	scanf("%d", &(mail->info->is_meeting_request));
	if ( mail->info->is_meeting_request == 1 
		|| mail->info->is_meeting_request == 2 ) {
		time_t current_time;

		mail->head->from = strdup("<test@test.com>");
		mail->head->to   = strdup("\"ActiveSync8\" <ActiveSync8@test.local>"); 
		mail->head->cc   = strdup("<bapina@gawab.com>");
		mail->head->bcc  = strdup("<tom@gmail.com>");
		
		mail->head->subject = strdup("save.mailbox...........");

		/*  dummy data for meeting request */
		meeting_req = malloc(sizeof(emf_meeting_request_t));
		memset(meeting_req, 0x00, sizeof(emf_meeting_request_t));
		
		meeting_req->meeting_response = 1;
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

		if((err = email_add_message_with_meeting_request(mail, mailbox, meeting_req, from_composer)) < 0)
			testapp_print("email_add_message failed. [%d]\n", err);
		else
			testapp_print("email_add_message success\n");
	}
	else {
		mail->info->is_meeting_request = 0;

		for(j = 0; j < mail_count; j++) {
			pFile = fopen(pFilePath, "w");
			for(i = 0; i < 500; i++)
		    	fprintf(pFile, "X2 X2 X2 X2 X2 X2 X2");
			fflush(pFile);
		    fclose(pFile);

			temp_string_1 = testapp_test_generate_random_string(128);
			temp_string_2 = testapp_test_generate_random_string(128);
			if(temp_string_1 && temp_string_2) {
				snprintf(temp_address, 512, "<%s@%s.com>", temp_string_1, temp_string_2);
				free(temp_string_1);
				free(temp_string_2);
				temp_string_1 = NULL;
				temp_string_2 = NULL;
			}

			mail->head->from = strdup(temp_address);

			temp_string_1 = testapp_test_generate_random_string(128);
			temp_string_2 = testapp_test_generate_random_string(128);
			if(temp_string_1 && temp_string_2) {
				snprintf(temp_address, 512, "<%s@%s.com>", temp_string_1, temp_string_2);
				free(temp_string_1);
				free(temp_string_2);
				temp_string_1 = NULL;
				temp_string_2 = NULL;

			}

			mail->head->to   = strdup(temp_address);

			temp_string_1 = testapp_test_generate_random_string(128);
			temp_string_2 = testapp_test_generate_random_string(128);
			if(temp_string_1 && temp_string_2) {
				snprintf(temp_address, 512, "<%s@%s.com>", temp_string_1, temp_string_2);
				free(temp_string_1);
				free(temp_string_2);
				temp_string_1 = NULL;
				temp_string_2 = NULL;
			}

			mail->head->cc   = strdup(temp_address);
			
			temp_string_1 = testapp_test_generate_random_string(128);
			temp_string_2 = testapp_test_generate_random_string(128);
			if(temp_string_1 && temp_string_2) {
				snprintf(temp_address, 512, "<%s@%s.com>", temp_string_1, temp_string_2);
				free(temp_string_1);
				free(temp_string_2);
				temp_string_1 = NULL;
				temp_string_2 = NULL;
			}

			mail->head->bcc  = strdup(temp_address);
			
			mail->head->subject = testapp_test_generate_random_string(128);

			if((err = email_add_message(mail, mailbox, from_composer)) < 0)
				testapp_print("email_add_message failed. [%d]\n", err);
			else {
				testapp_print("email_add_message success [%d/%d]\n", j, mail_count);
				if(attach_num) {
					testapp_print("attachment id[%d]\n", mail->body->attachment->attachment_id);
				}
			}
			if(mail->head->from) free(mail->head->from);
			if(mail->head->to) free(mail->head->to);
			if(mail->head->cc) free(mail->head->cc);
			if(mail->head->bcc) free(mail->head->bcc);
			if(mail->head->subject) free(mail->head->subject);
		}
	}

	testapp_print("saved mail id = [%d]\n", mail->info->uid);

	return FALSE;
}


static gboolean testapp_test_mail_send ()
{
	FILE *fp;
	emf_mailbox_t mbox;
	emf_mail_t *mail = malloc(sizeof(emf_mail_t));
	emf_mail_head_t *head = malloc(sizeof(emf_mail_head_t));
	emf_mail_body_t *body = malloc(sizeof(emf_mail_body_t));
	char arg[4096];
	int i=0;
	int account_id = 0;
	emf_option_t option;
	emf_attachment_info_t attachment;
	int attach_num = 0;
	int err = EMF_ERROR_NONE;;
	
	memset(&option, 0x00, sizeof(emf_option_t));

	memset(mail, 0x00, sizeof(emf_mail_t));
	memset(head, 0x00, sizeof(emf_mail_head_t));
	memset(body, 0x00, sizeof(emf_mail_body_t));

	memset(arg, 0x00, 4096);
	testapp_print("   mail to> ");
	scanf("%s",arg);

	if(arg[0]=='\0' || !strstr(arg, "@")) {
		printf("   invalid argument");
		return false;
	}
	
	for(i=0; i<strlen(arg); i++) {
		if(arg[i]==';') arg[i] = ',';
	}
	head->to = malloc(strlen(arg)+1); 
	strcpy(head->to, arg);

	head->from = strdup("<tom@gmail.com>");

	/* head->to = strdup("&quot;test09&quot; <+ test09@gmail.com>"); */

	memset(arg, 0x00, 4096);
	testapp_print("   input subject> ");
	scanf("%s",arg);
		
	if(arg[0]=='\0') {
		printf("   invalid argument");
		return FALSE;
	}
	head->subject = malloc(strlen(arg)+1); 
	strcpy(head->subject, arg);

	testapp_print("mail account_id >");
	scanf("%d",&account_id);
	
	memset(arg, 0x00, 4096);
	testapp_print("   input body> ");
	scanf("%s",arg);
	
	if(arg[0]=='\0') {
		printf("   invalid argument");
		return FALSE;
	}

	fp = fopen(MAIL_TEMP_BODY, "w+");
	fprintf(fp, arg);
	fclose(fp);
	body->plain = strdup(MAIL_TEMP_BODY);
	mail->body = body;
	mail->head = head;
	mail->info = malloc(sizeof(emf_mail_info_t));
	memset(mail->info, 0x00, sizeof(emf_mail_info_t));
	mail->info->account_id = account_id;
	mail->info->flags.draft = 1;
	mail->info->extra_flags.priority = 4;

	/* set flag1 as seen */
	mail->info->flags.seen = 1;
	testapp_print("\n > mail->info->flags:%d \n",mail->info->flags);
	
	memset(&mbox, 0x00, sizeof(emf_mailbox_t));
	mbox.account_id = account_id;
	mbox.name = strdup("OUTBOX");

	testapp_print(" Attach file 1/0 > ");
	scanf("%d",&attach_num);
	if ( attach_num ) {
		memset(&attachment, 0x00, sizeof(emf_attachment_info_t));
		memset(arg, 0x00, 4096);
		testapp_print("\n > Enter attachment name: ");
		scanf("%s",arg);
		
		attachment.name = strdup(arg);
		
		memset(arg, 0x00, 4096);
		testapp_print("\n > Enter attachment absolute path: ");
		scanf("%s",arg);		

		attachment.savename = strdup(arg);
		attachment.next = NULL;
		
		mail->body->attachment = &attachment;
		mail->body->attachment_num = attach_num;
	}

	if(email_add_message(mail, &mbox, 1) < 0) {
		testapp_print("email_add_message failed[%d]\n", err);
		return FALSE;
	} 
	else
		testapp_print("email_add_message success\n");

	testapp_print("   sending...\n");

	unsigned handle = 0;
	option.keep_local_copy = 1;
	
	if( email_send_mail(&mbox, mail->info->uid, &option, &handle) < 0) {
		testapp_print("   fail sending [%d]\n", err);
	} 
	else  {
		testapp_print("   finish sending. handle[%d]\n", handle);
	}

	return FALSE;
}

static gboolean testapp_test_send_test (int *MailId)
{

	FILE *fp;
	emf_mailbox_t mbox;
	emf_mail_t *mail = malloc(sizeof(emf_mail_t));
	emf_mail_head_t *head = malloc(sizeof(emf_mail_head_t));
	emf_mail_body_t *body = malloc(sizeof(emf_mail_body_t));
	char arg[4096];
	int i=0;
	int account_id = 0;
	emf_option_t option;
	emf_attachment_info_t attachment;
	int attach_num = 0;
	int err = EMF_ERROR_NONE;;
	
	memset(&option, 0x00, sizeof(emf_option_t));

	memset(mail, 0x00, sizeof(emf_mail_t));
	memset(head, 0x00, sizeof(emf_mail_head_t));
	memset(body, 0x00, sizeof(emf_mail_body_t));

	memset(arg, 0x00, 4096);
	testapp_print("   mail to> ");
	scanf("%s",arg);
	
	if(arg[0]=='\0' || !strstr(arg, "@")){
		printf("   invalid argument");
		return false;
	}
	
	for(i=0; i<strlen(arg); i++){
		if(arg[i]==';') arg[i] = ',';
	}
	head->to = malloc(strlen(arg)+1); 
	strcpy(head->to, arg);

	memset(arg, 0x00, 4096);
	testapp_print("   input subject> ");
	scanf("%s",arg);
		
	if(arg[0]=='\0'){
		printf("   invalid argument");
		return FALSE;
	}
	head->subject = malloc(strlen(arg)+1); 
	strcpy(head->subject, arg);

	testapp_print("mail account_id >");
	scanf("%d",&account_id);
	
	memset(arg, 0x00, 4096);
	testapp_print("   input body> ");
	scanf("%s",arg);
	
	if(arg[0]=='\0'){
		printf("   invalid argument");
		return FALSE;
	}

	fp = fopen(MAIL_TEMP_BODY, "w+");
	fprintf(fp, arg);
	fclose(fp);
	body->plain = strdup(MAIL_TEMP_BODY);
	mail->body = body;
	mail->head = head;
	mail->info = malloc(sizeof(emf_mail_info_t));
	memset(mail->info, 0x00, sizeof(emf_mail_info_t));
	mail->info->account_id = account_id;
	mail->info->flags.draft = 1;
	
	/* set flag1 as seen */
	mail->info->flags.seen = 1;
	testapp_print("\n > mail->info->flags:%d \n",mail->info->flags);
	
	memset(&mbox, 0x00, sizeof(emf_mailbox_t));
	mbox.account_id = account_id;
	mbox.name = strdup("OUTBOX");

	testapp_print(" Attach file 1/0 > ");
	scanf("%d",&attach_num);
	if ( attach_num ) {
		memset(&attachment, 0x00, sizeof(emf_attachment_info_t));
		memset(arg, 0x00, 4096);
		testapp_print("\n > Enter attachment name: ");
		scanf("%s",arg);
		
		attachment.name = strdup(arg);
		
		memset(arg, 0x00, 4096);
		testapp_print("\n > Enter attachment absolute path: ");
		scanf("%s",arg);		

		attachment.savename = strdup(arg);
		attachment.next = NULL;
		
		mail->body->attachment = &attachment;
		mail->body->attachment_num = attach_num;
	}

	if(email_add_message(mail, &mbox, 1) < 0) {
		testapp_print("email_add_message failed[%d]\n", err);
		return FALSE;
	} 
	else
		testapp_print("email_add_message success\n");

	testapp_print("   sending...\n");

	unsigned handle = 0;
	option.keep_local_copy = 1;
	email_send_mail(&mbox, mail->info->uid, &option, &handle);
	*MailId = mail->info->uid;
	testapp_print("*MailId[%d]\n",*MailId);
	printf("*MailId[%d]\n",*MailId);
	testapp_print("handle[%d]\n", handle);
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

	testapp_print("\n > Enter total Number of mail  want to send: ");
	scanf("%d", &num);
	mailIdList = (int *)malloc(sizeof(int)*num);
	if(!mailIdList)
		return false ;
	
	for(i = 1;i <=num ; i++) {
		testapp_test_send_test(&mail_id);
		
		testapp_print("mail_id[%d]",mail_id);

		mailIdList[i] = mail_id;
		testapp_print("mailIdList[%d][%d]",i,mailIdList[i]);

		mail_id = 0;
		testapp_print("\n > Do you want to cancel the send mail job '1' or '0': ");
		scanf("%d", &Y);
		if(Y == 1) {
			testapp_print("\n >Enter mail-id[1-%d] ",i);
				scanf("%d", &j);
			testapp_print("\n mailIdList[%d] ",mailIdList[j]);	
			if(email_cancel_send_mail( mailIdList[j]) < 0)
				testapp_print("EmfMailSendCancel failed..!");
			else
				testapp_print("EmfMailSendCancel success..!");
		}
	}
	return FALSE;
}

static gboolean testapp_test_send_mail_with_option ()
{
	FILE *fp;
	emf_mailbox_t mbox;
	emf_mail_t *mail = malloc(sizeof(emf_mail_t));
	emf_mail_head_t *head = malloc(sizeof(emf_mail_head_t));
	emf_mail_body_t *body = malloc(sizeof(emf_mail_body_t));
	char arg[4096];
	int i=0;
	int account_id = 0;
	emf_option_t option;
	emf_attachment_info_t attachment;
	int attach_num = 0;	
	
	memset(&option, 0x00, sizeof(emf_option_t));

	memset(mail, 0x00, sizeof(emf_mail_t));
	memset(head, 0x00, sizeof(emf_mail_head_t));
	memset(body, 0x00, sizeof(emf_mail_body_t));

	memset(arg, 0x00, 4096);
	testapp_print("   mail to> ");
	scanf("%s",arg);
	if(arg[0]=='\0' || !strstr(arg, "@")){
		printf("   invalid argument");
		return false;
	}
	for(i=0; i<strlen(arg); i++){
		if(arg[i]==';') arg[i] = ',';
	}
	head->to = malloc(strlen(arg)+1); 
	strcpy(head->to, arg);

	memset(arg, 0x00, 4096);
	testapp_print("   input subject> ");
	scanf("%s",arg);
		
	if(arg[0]=='\0'){
		printf("   invalid argument");
		return FALSE;
	}
	head->subject = malloc(strlen(arg)+1); 
	strcpy(head->subject, arg);

	testapp_print("mail account_id >");
	scanf("%d",&account_id);
	
	memset(arg, 0x00, 4096);
	testapp_print("   input body> ");
	scanf("%s",arg);
	
	if(arg[0]=='\0'){
		printf("   invalid argument");
		return FALSE;
	}

	fp = fopen(MAIL_TEMP_BODY, "w+");
	fprintf(fp, arg);
	fclose(fp);
	body->plain = strdup(MAIL_TEMP_BODY);
	mail->body = body;
	mail->head = head;
	mail->info = malloc(sizeof(emf_mail_info_t));
	memset(mail->info, 0x00, sizeof(emf_mail_info_t));
	mail->info->account_id = account_id;
	mail->info->flags.draft = 1;
	
	/* set flag1 as seen */
	mail->info->flags.seen = 1;
	testapp_print("\n > mail->info->flags:%d ",mail->info->flags);
	
	memset(&mbox, 0x00, sizeof(emf_mailbox_t));
	mbox.account_id = account_id;
	mbox.name = strdup("OUTBOX");

	testapp_print(" Attach file 1/0 > ");
	scanf("%d",&attach_num);
	if ( attach_num ) {
		memset(&attachment, 0x00, sizeof(emf_attachment_info_t));
		memset(arg, 0x00, 4096);
		testapp_print("\n > Enter attachment name: ");
		scanf("%s",arg);
		
		attachment.name = strdup(arg);
		
		memset(arg, 0x00, 4096);
		testapp_print("\n > Enter attachment absolute path: ");
		scanf("%s",arg);		

		attachment.savename = strdup(arg);
		attachment.next = NULL;
		
		mail->body->attachment = &attachment;
		mail->body->attachment_num = attach_num;
	}

	testapp_print("Keep local copy? (1=true, 0=false)");
	scanf("%d",&option.keep_local_copy);

	testapp_print("Request  delivery  receipt? (1=true, 0=false)");
	scanf("%d",&option.req_delivery_receipt);

	testapp_print("Request read receipt? (1=true, 0=false)");
	scanf("%d",&option.req_read_receipt);

	if(email_add_message(mail, &mbox, 1) < 0) {
		testapp_print("email_add_message failed\n");
	} 
	else
		testapp_print("email_add_message success\n");

	testapp_print("   sending...\n");

	unsigned handle = 0;

	
	if(email_send_mail(&mbox, mail->info->uid, &option, &handle) < 0) {
		testapp_print("   fail sending\n");
	} else {
		testapp_print("   finish sending\n");
		testapp_print("handle[%d]\n", handle);
	}

	return FALSE;
}
	
static gboolean testapp_test_download()
{
	
	emf_mailbox_t mbox;
	int account_id = 0;
	char arg[50];
	unsigned handle = 0;	
	testapp_print("\n > Enter Account id (0: for all account) : ");
	scanf("%d",&account_id);

	memset(arg, 0x00, 50);
	testapp_print("\n > Enter Mailbox name (ALL: for all mailboxes) : ");
	scanf("%s",arg);

	memset(&mbox, 0x00, sizeof(emf_mailbox_t));
	
	mbox.account_id = account_id;

	if(strcmp("ALL", arg) == 0)
		mbox.name = NULL;
	else
		mbox.name = strdup(arg);

	if(account_id == ALL_ACCOUNT) {
		if(email_sync_header_for_all_account(&handle) < 0)
			testapp_print("\n email_sync_header_for_all_account failed\n");
		else
			testapp_print("\n email_sync_header_for_all_account success. Handle[%d]\n", handle);
	}
	else {
		if(email_sync_header(&mbox,&handle) < 0)
			testapp_print("\n email_sync_header failed\n");
		else
			testapp_print("\n email_sync_header success. Handle[%d]\n", handle);
	}
		
	return FALSE;
}

static gboolean testapp_test_delete()
{
	int mail_id=0, account_id =0;
	emf_mailbox_t mbox = {0};
	char arg[10];	
	int err = EMF_ERROR_NONE;
	int from_server = 0;

	testapp_print("\n > Enter Account_id: ");
	scanf("%d",&account_id);

	testapp_print("\n > Enter Mail_id: ");
	scanf("%d",&mail_id);
	
	testapp_print("\n > Enter Mailbox name: ");
	scanf("%s",arg);
	
	testapp_print("\n > Enter from_server: ");
	scanf("%d",&from_server);
	
	mbox.account_id = account_id;
	mbox.name = strdup(arg);		


	/* delete message */
	if( (err = email_delete_message(&mbox, &mail_id, 1, from_server)) < 0)
		testapp_print("\n email_delete_message failed[%d]\n", err);
	else
		testapp_print("\n email_delete_message success\n");

	if ( mbox.name )
		free(mbox.name);
		
	return FALSE;
}

static gboolean testapp_test_get_mailbox_list ()
{

	int account_id =0;
	int mailbox_sync_type;
	int count = 0;
	int i = 0, error_code = EMF_ERROR_NONE;
	emf_mailbox_t *mailbox_list=NULL;
	testapp_print("\n > Enter account id: ");
	scanf("%d", &account_id);
	testapp_print("\n > Enter mailbox_sync_type\n[-1 :for all mailboxes, 0 : for mailboxes from server, 1 : local mailboxes\n : ");
	scanf("%d", &mailbox_sync_type);

	if((error_code = email_get_mailbox_list(account_id, mailbox_sync_type, &mailbox_list, &count)) < 0) {
		testapp_print("   email_get_mailbox_list error %d\n", error_code);
		return false ;
	}

	testapp_print("There are %d mailboxes\n", count);

	testapp_print("============================================================================\n");
	testapp_print("number\taccount_id\t name\t\t alias\t local_yn\t unread\tmailbox_type\t has_archived_mails\n");
	testapp_print("============================================================================\n");
	if ( count == 0 ) {
		testapp_print("No mailbox is matched\n");
	}
	else {
		for(i=0;i<count;i++) {
	    	testapp_print("[%d] - ", i);
			testapp_print(" %2d\t [%-15s]\t[%-15s]\t", mailbox_list[i].account_id, mailbox_list[i].name, mailbox_list[i].alias);
			testapp_print(" %d\t %d\t %d\n", mailbox_list[i].local, mailbox_list[i].unread_count,mailbox_list[i].mailbox_type, mailbox_list[i].has_archived_mails);
		}
	}
	testapp_print("============================================================================\n");
	/* EmfMailboxFree(emf_mailbox_t** mailbox_list, int count, int* err_code) */

	if((error_code = email_get_mailbox_list_ex(account_id, mailbox_sync_type, 1, &mailbox_list, &count)) < 0) {
		testapp_print("   email_get_mailbox_list_ex error %d\n", error_code);
		return false ;
	}

	testapp_print("There are %d mailboxes\n", count);

	testapp_print("============================================================================\n");
	testapp_print("number\taccount_id\t name\t\t alias\t local_yn\t unread\t total\t total_on_ server\tmailbox_type\t has_archived_mails\n");
	testapp_print("============================================================================\n");
	if ( count == 0 ) {
		testapp_print("No mailbox is matched\n");
	}
	else {
		for(i=0;i<count;i++) {
	    	testapp_print("[%d] - ", i);
			testapp_print(" %2d\t [%-15s]\t[%-15s]\t", mailbox_list[i].account_id, mailbox_list[i].name, mailbox_list[i].alias);
			testapp_print(" %d\t %d\t %d\t %d\t %d\t %d\n", mailbox_list[i].local, mailbox_list[i].unread_count, mailbox_list[i].total_mail_count_on_local, mailbox_list[i].total_mail_count_on_server, mailbox_list[i].mailbox_type, mailbox_list[i].has_archived_mails);
		}
	}
	testapp_print("============================================================================\n");
	testapp_print("Start to free\n");
	
	email_free_mailbox(&mailbox_list, count);
	return FALSE;
}

static gboolean testapp_test_move()
{
	int mail_id[3];
	int account_id =0;
	emf_mailbox_t mbox;
	char arg[10];
	int i = 0;
	
	testapp_print("\n > Enter Account_id: ");
	scanf("%d",&account_id);
	
	for(i = 0; i< 3; i++) {
	testapp_print("\n > Enter Mail_id: ");
		scanf("%d",&mail_id[i]);
	}
	
	memset(&mbox, 0x00, sizeof(emf_mailbox_t));
					
	testapp_print("\n > Enter Mailbox name: ");
	scanf("%s",arg);
	mbox.account_id = account_id;
	mbox.name = strdup(arg);		
	/* move message */
	email_move_mail_to_mailbox(mail_id,	3, &mbox);
	return FALSE;
}

static gboolean testapp_test_delete_all()
{
	int account_id =0;
	emf_mailbox_t mbox = {0};
	char  arg[100] = {0};
	int err = EMF_ERROR_NONE;
	testapp_print("\n > Enter Account_id: ");
	scanf("%d",&account_id);
						
	testapp_print("\n > Enter Mailbox name: ");
	scanf("%s",arg);
										
	mbox.account_id = account_id;
	mbox.name = strdup(arg);		

	/* delete all message */
	if ( (err = email_delete_all_message_in_mailbox(&mbox, 0)) < 0)
		testapp_print("email_delete_all_message_in_mailbox failed[%d]\n", err);	
	else
		testapp_print("email_delete_all_message_in_mailbox Success\n");	
															
	return FALSE;
}


static gboolean testapp_test_add_attachment()
{	
	int account_id = 0;
	int mail_id = 0;
	char arg[100];
	emf_mailbox_t mbox;
	emf_attachment_info_t attachment;
	
	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);	
	
	testapp_print("\n > Enter Account_id: ");
	scanf("%d",&account_id);

	memset(arg, 0x00, 100);
	testapp_print("\n > Enter Mailbox name: ");
	scanf("%s",arg);

	memset(&mbox, 0x00, sizeof(emf_attachment_info_t));
	mbox.account_id = account_id;
	mbox.name = strdup(arg);
	
	memset(&attachment, 0x00, sizeof(emf_attachment_info_t));
	memset(arg, 0x00, 100);
	testapp_print("\n > Enter attachment name: ");
	scanf("%s",arg);
	
	attachment.name = strdup(arg);
	
	memset(arg, 0x00, 100);
	testapp_print("\n > Enter attachment absolute path: ");
	scanf("%s",arg);		

	attachment.downloaded = true;
	attachment.savename = strdup(arg);
	attachment.next = NULL;
	if(email_add_attachment( &mbox, mail_id, &attachment) < 0)
		testapp_print("email_add_attachment failed\n");	
	else
		testapp_print("email_add_attachment success\n");


	return FALSE;

}
 
static gboolean testapp_test_update()
{
	emf_mailbox_t mailbox;
	emf_mail_t *mail = NULL;
	int mail_id = 0;
	int err = EMF_ERROR_NONE;
	char arg[50];
	
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));
	
	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);

	if (email_get_mail(&mailbox, mail_id, &mail) < 0) {	
		testapp_print("email_get_mail failed\n");
		return FALSE;
	}

	testapp_print("\n > Enter Subject: ");
	scanf("%s", arg);	

	mail->head->subject= strdup(arg);

	if (mail->body && mail->body->attachment) {
		emf_attachment_info_t* p = mail->body->attachment;
		struct stat st_buf;
		
		while (p) {
			if (!p->savename || stat(p->savename, &st_buf) < 0) {
				testapp_print("\t stat(\"%s\" failed...\n", p->savename);
				
				err = EMF_ERROR_INVALID_PARAM;		/* TODO: confirm me */
				goto FINISH_OFF;
			}

			p->downloaded = 1;
			
			p = p->next;
		}
	}	

	if ( mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_REQUEST 
		|| mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_RESPONSE 
		|| mail->info->is_meeting_request == EMF_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
		emf_meeting_request_t *meeting_req = NULL;
		if ( (err = email_get_meeting_request(mail_id, &meeting_req)) < 0 ) {
			testapp_print("email_get_meeting_request() failed\n");
			return FALSE;
		}
	
		testapp_print("\n > Enter meeting response: ");
		scanf("%d", (int*)&(meeting_req->meeting_response));
		if ( (err = email_update_message_with_meeting_request(mail_id, mail, meeting_req)) < 0)	
			testapp_print("email_update_message failed.[%d]\n", err);
		else
			testapp_print("email_update_message success\n");

		email_free_meeting_request(&meeting_req, 1);
	}
	else {
		if (email_update_message(mail_id, mail) < 0)	
			testapp_print("email_update_message failed.[%d]\n", err);
		else
			testapp_print("email_update_message success\n");
	}

FINISH_OFF:

	return FALSE;
	
}

 
static gboolean testapp_test_get_mail_list_ex()
{
	testapp_print(" >>> testapp_test_get_mail_list_ex : Entered \n");
	emf_mail_list_item_t *mail_list = NULL, **mail_list_pointer = NULL;
	char mailbox_name[50];
	int count = 0, i = 0;
	int account_id = 0;
	int start_index =0;
	int limit_count = 0;
	int sorting = 0;
	int err_code = EMF_ERROR_NONE;
	int to_get_count = 0;
	int is_for_thread_view = 0;
	int list_type;

	memset(mailbox_name, 0x00, 10);
	testapp_print("\n > Enter Mailbox name (0 = all mailboxes) :");
		scanf("%s", mailbox_name);

	testapp_print("\n > Enter Account_id (0 = all accounts) : ");
	scanf("%d",&account_id);		

	testapp_print("\n > Enter Sorting : ");
	scanf("%d",&sorting);	
	
	testapp_print("\n > Enter Start index : ");
	scanf("%d",&start_index);

	testapp_print("\n > Enter max_count : ");
	scanf("%d",&limit_count);

	testapp_print("\n > For thread view : ");
	scanf("%d",&is_for_thread_view);

	testapp_print("\n > To get count : ");
	scanf("%d",&to_get_count);

	if(to_get_count)
		mail_list_pointer = NULL;
	else
		mail_list_pointer = &mail_list;
	
	if(is_for_thread_view == -2) {
		list_type = EMF_LIST_TYPE_LOCAL;
 	}
	else if(is_for_thread_view == -1)
		list_type = EMF_LIST_TYPE_THREAD;
	else
		list_type = EMF_LIST_TYPE_NORMAL;
	
	/* Get mail list */
	if(strcmp(mailbox_name, "0") == 0) {
		testapp_print("Calling email_get_mail_list_ex for all mailbox.\n");
		err_code = email_get_mail_list_ex(account_id, NULL, list_type, start_index, limit_count, sorting,  mail_list_pointer, &count);
		if ( err_code < 0) 
			testapp_print("email_get_mail_list_ex failed - err[%d]\n", err_code);
	}
	else {
		testapp_print("Calling email_get_mail_list_ex for %s mailbox.\n", mailbox_name);
		err_code = email_get_mail_list_ex(account_id, mailbox_name, list_type, start_index, limit_count, sorting,  mail_list_pointer, &count);
		if ( err_code < 0) 
			testapp_print("email_get_mail_list_ex failed - err[%d]\n", err_code);
	}
	testapp_print("email_get_mail_list_ex >>>>>>count - %d\n",count);

	if (mail_list) {
		for (i=0; i< count; i++) {
			testapp_print("\n[%d]\n", i);
			testapp_print(" >>> mailbox_name [ %s ] \n", mail_list[i].mailbox_name);
			testapp_print(" >>> mail_id [ %d ] \n", mail_list[i].mail_id);
			testapp_print(" >>> account_id [ %d ] \n", mail_list[i].account_id);
			if (  mail_list[i].from != NULL )
				testapp_print(" >>> From [ %s ] \n", mail_list[i].from);
			if (  mail_list[i].recipients != NULL )
				testapp_print(" >>> recipients [ %s ] \n", mail_list[i].recipients);
			if (  mail_list[i].subject != NULL )
				testapp_print(" >>> subject [ %s ] \n", mail_list[i].subject);
			testapp_print(" >>> is_text_downloaded [ %d ] \n", mail_list[i].is_text_downloaded);
			if (  mail_list[i].datetime != NULL )
				testapp_print(" >>> datetime [ %s ] \n", mail_list[i].datetime);
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
	
	testapp_print(" >>> email_get_mail_list_ex : End \n");
	return 0;
}


static gboolean testapp_test_get_mail_list_for_thread_view()
{
	testapp_print(" >>> testapp_test_get_mail_list_for_thread_view : Entered \n");
	emf_mail_list_item_t *mail_list = NULL;
	int count = 0, i = 0;
	int account_id = 0;
	int err_code = EMF_ERROR_NONE;

	/* Get mail list */
	if ( email_get_mail_list_ex(account_id, NULL , EMF_LIST_TYPE_THREAD, 0, 500, EMF_SORT_DATETIME_HIGH, &mail_list, &count) < 0)  {
		testapp_print("email_get_mail_list_ex failed : %d\n",err_code);
		return FALSE;
	}
	testapp_print("email_get_mail_list_ex >>>>>>count - %d\n",count);
	if (mail_list) {
		for (i=0; i< count; i++) {
			testapp_print(" i [%d]\n", i);
			testapp_print(" >>> Mail ID [ %d ] \n", mail_list[i].mail_id);
			testapp_print(" >>> Account ID [ %d ] \n", mail_list[i].account_id);
			testapp_print(" >>> Mailbox Name [ %s ] \n", mail_list[i].mailbox_name);
			testapp_print(" >>> From [ %s ] \n", mail_list[i].from);
			testapp_print(" >>> recipients [ %s ] \n", mail_list[i].recipients);
			testapp_print(" >>> subject [ %s ] \n", mail_list[i].subject);
			testapp_print(" >>> is_text_downloaded [ %d ] \n", mail_list[i].is_text_downloaded);
			testapp_print(" >>> datetime [ %s ] \n", mail_list[i].datetime);
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
 

static gboolean	testapp_test_get ()
{
	
	emf_mailbox_t mailbox;
	emf_mail_t *mail = NULL;
	int mail_id = 0;
	int err = EMF_ERROR_NONE;
	
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));

	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);

	if ( (err = email_get_mail(&mailbox, mail_id, &mail)) < 0)
		testapp_print("email_get_mail failed[%d]\n", err);
	else {
		testapp_print("email_get_mail success\n");
		testapp_print("preview : [%s]\n", mail->head->previewBodyText);

		emf_meeting_request_t *meeting_req = NULL;
		testapp_print("mail->info->is_meeting_request[%d]\n", mail->info->is_meeting_request);
		if ( mail->info->is_meeting_request != 0 ) {
			if ( (err = email_get_meeting_request(mail->info->uid, &meeting_req)) < 0 ) {
				testapp_print("email_get_meeting_request() failed[%d]\n", err);
			}
			else {
				testapp_print("email_get_meeting_request() success\n");
				testapp_print(">>>>> meeting_req->mail_id[%d]\n", meeting_req->mail_id);
				testapp_print(">>>>> meeting_req->meeting_response[%d]\n", meeting_req->meeting_response);
				testapp_print(">>>>> meeting_req->start_time[%s]\n", asctime(&(meeting_req->start_time)));
				testapp_print(">>>>> meeting_req->end_time[%s]\n", asctime(&(meeting_req->end_time)));
				testapp_print(">>>>> meeting_req->location[%s]\n", meeting_req->location);
				testapp_print(">>>>> meeting_req->global_object_id[%s]\n", meeting_req->global_object_id);
				testapp_print(">>>>> meeting_req->time_zone.offset_from_GMT[%d]\n", meeting_req->time_zone.offset_from_GMT);
				testapp_print(">>>>> meeting_req->time_zone.standard_name[%s]\n", meeting_req->time_zone.standard_name);
				testapp_print(">>>>> meeting_req->time_zone.standard_time_start_date[%s]\n", asctime(&(meeting_req->time_zone.standard_time_start_date)));
				testapp_print(">>>>> meeting_req->time_zone.standard_bias[%d]\n", meeting_req->time_zone.standard_bias);
				testapp_print(">>>>> meeting_req->time_zone.daylight_name[%s]\n", meeting_req->time_zone.daylight_name);
				testapp_print(">>>>> meeting_req->time_zone.daylight_time_start_date[%s]\n", asctime(&(meeting_req->time_zone.daylight_time_start_date)));
				testapp_print(">>>>> meeting_req->time_zone.daylight_bias[%d]\n", meeting_req->time_zone.daylight_bias);
			}
		}

		if ( meeting_req != NULL )
		    email_free_meeting_request(&meeting_req, 1);
	}

	if ( mail )
		email_free_mail(&mail, 1);
		
	return TRUE;
}

static gboolean testapp_test_get_info()
{
	emf_mailbox_t mailbox;
	emf_mail_info_t *mail_info = NULL;
	int mail_id = 0,account_id = 0;
	
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));

	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);

	testapp_print("\n > Enter account Id: ");
	scanf("%d", &account_id);

	mailbox.account_id = account_id;
	if (email_get_info(&mailbox, mail_id, &mail_info) < 0)
		testapp_print("email_get_info failed\n");
	else	
		testapp_print("email_get_info SUCCESS\n");

	if (mail_info != NULL) {
		em_core_mail_info_free(&mail_info, 1, NULL);
	}
	return TRUE;
}


static gboolean testapp_test_get_header ()
{
	emf_mailbox_t mailbox;
	emf_mail_head_t *head = NULL;
	int mail_id = 0,account_id = 0;
	
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));

	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);

	testapp_print("\n > Enter account Id: ");
	scanf("%d", &account_id);

	mailbox.account_id = account_id;

	if ( email_get_header_info(&mailbox, mail_id, &head) < 0) {
		testapp_print("email_get_header_info failed\n");
		return FALSE;
	}
	else	
		testapp_print("email_get_header_info SUCCESS\n");

	if(head->subject)
		testapp_print("\n subject : %s", head->subject);
	if(head->from)
		testapp_print("\n from : %s", head->from);
	if(head->reply_to)
		testapp_print("\n reply_to : %s", head->reply_to);

	return TRUE;
}

static gboolean testapp_test_get_body ()
{
	emf_mailbox_t mailbox;
	emf_mail_body_t *body = NULL;
	int mail_id = 0,account_id = 0;
	
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));

	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);

	testapp_print("\n > Enter account Id: ");
	scanf("%d", &account_id);

	mailbox.account_id = account_id;

	if ( email_get_body_info(&mailbox, mail_id, &body) < 0)
		testapp_print("email_get_body_info failed\n");
	else	
		testapp_print("email_get_body_info SUCCESS\n");

	return TRUE;
}

static gboolean testapp_test_count ()
{
	emf_mailbox_t mailbox;
	int account_id = 0;
	int total = 0;
	int unseen = 0;
	char arg[50];
		
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));

	testapp_print("\n > Enter account_id (0 means all accounts) : ");
	scanf("%d", &account_id);

	if(account_id == 0) {
		mailbox.name = NULL;
	}
	else {
		testapp_print("\n > Enter maibox name: ");
		scanf("%s", arg);
		mailbox.name = strdup(arg);
	}
	
	mailbox.account_id = account_id;
	if(email_count_message(&mailbox, &total, &unseen) >= 0)
		testapp_print("\n Total: %d, Unseen: %d \n", total, unseen);

	return TRUE;
}

static gboolean testapp_test_modify_flag()
{
	emf_mail_flag_t newflag;
	int mail_id = 0;

	memset(&newflag, 0x00, sizeof(emf_mail_flag_t));
	newflag.seen = 1;
	newflag.answered = 0;
	newflag.sticky = 1;
	
	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);	

	if(email_modify_mail_flag(mail_id, newflag, 1) < 0)
		testapp_print("email_modify_mail_flag failed");
	else
		testapp_print("email_modify_mail_flag success");
		
	return TRUE;
}

static gboolean	testapp_test_modify_extra_flag ()
{
	emf_extra_flag_t newflag;
	int mail_id = 0;

	memset(&newflag, 0x00, sizeof(emf_extra_flag_t));

	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);	

	if( email_modify_extra_mail_flag(mail_id, newflag) < 0)
		testapp_print("email_modify_extra_mail_flag failed");
	else
		testapp_print("email_modify_extra_mail_flag success");
	return TRUE;
}

static gboolean	testapp_test_set_flags_field ()
{
	int account_id = 0;
	int mail_id = 0;

	testapp_print("\n > Enter Account ID: ");
	scanf("%d", &account_id);	
	
	testapp_print("\n > Enter Mail ID: ");
	scanf("%d", &mail_id);	

	if(email_set_flags_field(account_id, &mail_id, 1, EMF_FLAGS_FLAGGED_FIELD, 1, 1) < 0)
		testapp_print("email_set_flags_field failed");
	else
		testapp_print("email_set_flags_field success");
		
	return TRUE;
}

static gboolean testapp_test_download_body ()
{
	int mail_id = 0;
	int account_id = 0;	
	char arg[50];
	unsigned handle = 0, err;
	
	emf_mailbox_t mailbox;
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));

	testapp_print("\n > Enter account_id: ");
	scanf("%d", &account_id);
	
	testapp_print("\n > Enter maibox name: ");
	scanf("%s", arg);

	mailbox.name = strdup(arg);
	mailbox.account_id = account_id;
	
	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);	
	err = email_download_body(&mailbox, mail_id, 0, &handle);
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
	char arg[50];
	unsigned handle = 0;
	
	emf_mailbox_t mailbox;
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));

	testapp_print("\n > Enter account_id: ");
	scanf("%d", &account_id);
	
	testapp_print("\n > Enter maibox name: ");
	scanf("%s", arg);

	mailbox.name = strdup(arg);
	mailbox.account_id = account_id;

	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);	
	
	if( email_download_body(&mailbox, mail_id, 0, &handle) < 0)
		testapp_print("email_download_body failed");
	else {
		testapp_print("email_download_body success\n");
		testapp_print("Do u want to cancel download job>>>>>1/0\n");
		scanf("%d",&yes);
		if(1 == yes) {
			if(email_cancel_job(account_id, handle) < 0)
				testapp_print("EmfCancelJob failed..!");
			else {
				testapp_print("EmfCancelJob success..!");
				testapp_print("handle[%d]\n", handle);
			}
		}
			
	}
	return TRUE;
}
static gboolean testapp_test_download_attachment ()
{
	int mail_id = 0;
	int account_id = 0;	
	char arg[50];
	unsigned handle = 0;
	
	emf_mailbox_t mailbox;
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));

	testapp_print("\n > Enter account_id: ");
	scanf("%d", &account_id);
	
	memset(arg, 0x00, 50);	
	testapp_print("\n > Enter maibox name: ");
	scanf("%s", arg);

	mailbox.name = strdup(arg);
	mailbox.account_id = account_id;
	
	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);	
    
	memset(arg, 0x00, 50);
	testapp_print("\n > Enter attachment number: ");
	scanf("%s",arg);
	
	if( email_download_attachment(&mailbox, mail_id, arg ,&handle) < 0)
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
	
	emf_mailbox_t mailbox;
	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));

	testapp_print("\n > Enter Mail Id: ");
	scanf("%d", &mail_id);
	
	testapp_print("\n > Enter timeout in seconds: ");
	scanf("%d", &timeout);

	if( email_retry_send_mail(mail_id, timeout) < 0)
		testapp_print("email_retry_send_mail failed");		
	return TRUE;		
}

static gboolean testapp_test_find()
{
	int i, account_id = 0, result_count;	
	char mailbox_name[255] = { 0, }, *mailbox_name_pointer = NULL, search_keyword[255] = { 0, };
	emf_mail_list_item_t *mail_list = NULL;

	testapp_print("\n > Enter account_id : ");
	scanf("%d", &account_id);
	
	testapp_print("\n > Enter maibox name [0 means all mailboxes] : ");
	scanf("%s", mailbox_name);

	if(strcmp(mailbox_name, "0") == 0)
		mailbox_name_pointer = NULL;
	else
		mailbox_name_pointer = mailbox_name;

	testapp_print("\n > Enter Keyword : ");
	scanf("%s", search_keyword);

	if(email_find_mail(account_id, mailbox_name_pointer, EMF_LIST_TYPE_NORMAL, EMF_SEARCH_FILTER_ALL , search_keyword, -1, -1, EMF_SORT_DATETIME_HIGH, &mail_list, &result_count) == EMF_ERROR_NONE) {
		testapp_print("\n Result count [%d]", result_count);
		for(i = 0; i < result_count; i++) {
			testapp_print("\nmail_id[%d], from[%s], subject[%s]", mail_list[i].mail_id, mail_list[i].from, mail_list[i].subject);
		}
	}
	
	if(mail_list)
		free(mail_list);

	return TRUE;
}

static gboolean testapp_test_load_test_mail_send()
{
	FILE *fp;
	emf_mailbox_t mbox;
	emf_mail_t *mail = malloc(sizeof(emf_mail_t));
	emf_mail_head_t *head = malloc(sizeof(emf_mail_head_t));
	emf_mail_body_t *body = malloc(sizeof(emf_mail_body_t));
	char arg[4096];
	int i=0, count =0;
	int account_id = 0;
	emf_option_t option;
	emf_attachment_info_t attachment;
	int attach_num = 0;
	int interval =0;
	int no_of_tests=0;
	memset(&option, 0x00, sizeof(emf_option_t));

	memset(mail, 0x00, sizeof(emf_mail_t));
	memset(head, 0x00, sizeof(emf_mail_head_t));
	memset(body, 0x00, sizeof(emf_mail_body_t));
        
	memset(arg, 0x00, 4096);
	testapp_print("   mail to> ");
	scanf("%s",arg);
	if(arg[0]=='\0' || !strstr(arg, "@")){
		printf("   invalid argument");
		return false;
	}
	for(i=0; i<strlen(arg); i++){
		if(arg[i]==';') arg[i] = ',';
	}
	head->to = malloc(strlen(arg)+1); 
	strcpy(head->to, arg);

	memset(arg, 0x00, 4096);
	testapp_print("   input subject> ");
	scanf("%s",arg);
		
	if(arg[0]=='\0'){
		printf("   invalid argument");
		return FALSE;
	}
	head->subject = malloc(strlen(arg)+1); 
	strcpy(head->subject, arg);

	testapp_print("mail account_id >");
	scanf("%d",&account_id);
	
	memset(arg, 0x00, 4096);
	testapp_print("   input body> ");
	scanf("%s",arg);
	
	if(arg[0]=='\0'){
		printf("   invalid argument");
		return FALSE;
	}

	fp = fopen(MAIL_TEMP_BODY, "w+");
	fprintf(fp, arg);
	fclose(fp);
	body->plain = strdup(MAIL_TEMP_BODY);
	mail->body = body;
	mail->head = head;
	mail->info = malloc(sizeof(emf_mail_info_t));
	memset(mail->info, 0x00, sizeof(emf_mail_info_t));
	mail->info->account_id = account_id;
	mail->info->flags.draft = 1;
	
	memset(&mbox, 0x00, sizeof(emf_mailbox_t));
	mbox.account_id = account_id;
	/* mbox.name = strdup("Drafts"); */
	mbox.name = strdup("DRAFTBOX");

	testapp_print(" Attach file 1/0 > ");
	scanf("%d",&attach_num);
	if ( attach_num ) {
		
		memset(&attachment, 0x00, sizeof(emf_attachment_info_t));
		memset(arg, 0x00, 4096);
		testapp_print("\n > Enter attachment name: ");
		scanf("%s",arg);
		
		attachment.name = strdup(arg);
		
		memset(arg, 0x00, 4096);
		testapp_print("\n > Enter attachment absolute path: ");
		scanf("%s",arg);		

		attachment.savename = strdup(arg);
		attachment.next = NULL;
		
		mail->body->attachment = &attachment;
		mail->body->attachment_num = attach_num;
	}

	testapp_print("> Enter Number of tests: ");
	scanf("%d",&no_of_tests);

	testapp_print("> Enter time interval: ");
	scanf("%d",&interval);
	for(count=0;count<no_of_tests;count++)  {
		if(email_add_message(mail, &mbox, 1) < 0) {
			testapp_print("email_add_message failed\n");
		} 

		testapp_print("   sending...\n");

		unsigned handle = 0;
		option.keep_local_copy = 1;
		
		if(email_send_mail(&mbox, mail->info->uid, &option, &handle) < 0){
			testapp_print("   fail sending\n");
		} else {
			testapp_print("   finish sending\n");
			testapp_print("handle[%d]\n", handle);
		}
		sleep(interval);
	}
	return FALSE;
}
	
 
static gboolean testapp_test_count_message_on_sending()
{
	emf_mailbox_t mailbox;
	int total_count;
	int err = EMF_ERROR_NONE;
	char temp[128];

	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));
	/*  input mailbox information : need account_id and name */
	testapp_print("mail account id(0: All account)> ");
	scanf("%d", &mailbox.account_id);

	testapp_print("mailbox_name(0 : NULL)> ");
	scanf("%s", temp );
	if ( strcmp(temp, "0") == 0 ) {
		mailbox.name = NULL;
	}
	else {
		mailbox.name = malloc(strlen(temp) + 1);
		if ( mailbox.name == NULL ) {
			testapp_print("fail malloc\n");
			return false;
		}
		strcpy(mailbox.name, temp);
	}
	
	err = email_count_message_on_sending(&mailbox, &total_count);
	if ( err < 0 ) {
		testapp_print("  fail email_count_message_on_sending: err[%d]\n", err);
	}
	else {
		testapp_print("  success email_count_message_on_sending: # of messages on senging status : %d\n", total_count);
	}

	if ( mailbox.name ) {
		free(mailbox.name);
		mailbox.name = NULL;
	}
	return false;
}


static gboolean testapp_test_move_all_mails_to_mailbox()
{
	emf_mailbox_t src_mailbox;
	emf_mailbox_t dest_mailbox;
	int err = EMF_ERROR_NONE;
	char temp[128];

	memset(&src_mailbox, 0x00, sizeof(emf_mailbox_t));
	memset(&dest_mailbox, 0x00, sizeof(emf_mailbox_t));

	/*  input mailbox information : need  account_id and name (src & dest) */
	testapp_print("src mail account id(0: All account)> ");
	scanf("%d", &src_mailbox.account_id);

	testapp_print("src mailbox_name(0 : NULL)> ");
	scanf("%s", temp );
	if ( strcmp(temp, "0") == 0 ) {
		src_mailbox.name = NULL;
	}
	else {
		src_mailbox.name = malloc(strlen(temp) + 1);
		if ( src_mailbox.name == NULL ) {
			testapp_print("fail malloc\n");
			return false;
		}
		strcpy(src_mailbox.name, temp);
	}

	/*  Destination mailbox */
	testapp_print("dest mail account id> ");
	scanf("%d", &dest_mailbox.account_id);
	if ( dest_mailbox.account_id <= 0 ) {
		testapp_print("Invalid dest account_id[%d]\n", dest_mailbox.account_id);
		goto FINISH_OFF;
	}

	testapp_print("dest mailbox_name> ");
	scanf("%s", temp);
	if ( strcmp(temp, "") == 0 ) {
		testapp_print("Invalid dest mailbox_name[%s]\n", temp);
		goto FINISH_OFF;
	}
	else {
		dest_mailbox.name = malloc(strlen(temp) + 1);
		if ( dest_mailbox.name == NULL ) {
			testapp_print("fail malloc\n");
			goto FINISH_OFF;
		}
		strcpy(dest_mailbox.name, temp);
	}
	
	err = email_move_all_mails_to_mailbox(&src_mailbox, &dest_mailbox);
	if ( err < 0 ) {
		testapp_print("  fail email_move_all_mails_to_mailbox: err[%d]\n", err);
	}
	else {
		testapp_print("  success email_move_all_mails_to_mailbox: from [%d:%s] to [%d:%s]\n", src_mailbox.account_id, src_mailbox.name, dest_mailbox.account_id, dest_mailbox.name);
	}

FINISH_OFF:
	if ( src_mailbox.name ) {
		free(src_mailbox.name);
		src_mailbox.name = NULL;
	}
	if ( dest_mailbox.name ) {
		free(dest_mailbox.name);
		dest_mailbox.name = NULL;
	}
	return false;
}

static gboolean testapp_test_count_message_with_draft_flag()
{
	emf_mailbox_t mailbox;
	int total_count;
	int err = EMF_ERROR_NONE;
	char temp[128];

	memset(&mailbox, 0x00, sizeof(emf_mailbox_t));
	/*  input mailbox information : need account_id and name */
	testapp_print("mail account id(0: All account)> ");
	scanf("%d", &mailbox.account_id);

	testapp_print("mailbox_name(0 : NULL)> ");
	scanf("%s", temp );
	if ( strcmp(temp, "0") == 0 ) {
		mailbox.name = NULL;
	}
	else {
		mailbox.name = malloc(strlen(temp) + 1);
		if ( mailbox.name == NULL ) {
			testapp_print("fail malloc\n");
			return false;
		}
		strcpy(mailbox.name, temp);
	}
	
	err = email_count_message_with_draft_flag(&mailbox, &total_count);

	if ( err < 0) {
		testapp_print("  fail email_count_message_with_draft_flag: err[%d]\n", err);
	}
	else {
		testapp_print("  success email_count_message_with_draft_flag: # of messages with draft flag : %d\n", total_count);
	}

	if ( mailbox.name ) {
		free(mailbox.name);
		mailbox.name = NULL;
	}
	return false;
}

/* sowmya.kr@samsung.com, 01282010 - Changes to get latest unread mail id for given account */
static gboolean testapp_test_get_latest_unread_mail_id()
{
	
	int mail_id = 0,account_id = 0;
	int err = EMF_ERROR_NONE;
		

	testapp_print("Enter account Id to get latest unread mail Id,<-1 to get latest irrespective of account id> ");
	scanf("%d", &account_id);

	err = email_get_latest_unread_mail_id(account_id, &mail_id);
	if ( err < 0) {
		testapp_print("  fail email_get_latest_unread_mail_id: err[%d]\n", err);
	}
	else {
		testapp_print("  success email_get_latest_unread_mail_id: Latest unread mail id : %d\n", mail_id);
	}	
	return FALSE;
}

static gboolean testapp_test_get_totaldiskusage()
{
	unsigned long total_size = 0;
	int err_code = EMF_ERROR_NONE ;
	
	err_code = email_get_disk_space_usage(&total_size);
	if ( err_code < 0)
		testapp_print("email_get_disk_space_usage failed err : %d\n",err_code);
	else
		testapp_print("email_get_disk_space_usage SUCCESS, total disk usage in KB : %ld \n", total_size);		

	return FALSE;
}



static gboolean testapp_test_db_test()
{
	int err = EMF_ERROR_NONE;
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
	scanf("%d", &mail_id);
	testapp_print("Input Account id:\n");
	scanf("%d", &account_id);
	testapp_print("Input TO field:\n");
	scanf("%s", to);
	testapp_print("Input CC field:\n");
	scanf("%s", cc);
	testapp_print("Input BCC field:\n");
	scanf("%s", bcc);
	
	if ( em_storage_test(mail_id, account_id, to, cc, bcc, &err) == true ) {
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
	int i;
	int type;
	int index;
	int check_yn;
	int err = EMF_ERROR_NONE;
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
	scanf("%d", &type);

	switch ( type ) {
		case 1:
			do  {
				for ( i = 0; address_list[i] != NULL; i++ ) {
					testapp_print("[%d] addr[%s]\n", i, address_list[i]);
					address_count++;
				}
				testapp_print("Choose address to be tested:[99:quit]\n");
				scanf("%d", &index);
				if ( index == 99 )
					break;

				testapp_print(">> [%d] Checking? (1:Yes, Other:No) [%s]\n", index, address_list[index]);
				scanf("%d", &check_yn);
				if ( check_yn == 1 ) {
					pAddress = strdup(address_list[index]);
					if ( em_core_verify_email_address(pAddress, false, &err ) == true ) {
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
			scanf("%s", input_address);
			if ( em_core_verify_email_address(input_address, false, &err ) == true ) {
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

	return FALSE;
}

static gboolean testapp_test_get_max_mail_count()
{
	int max_count = -1;
	int err = EMF_ERROR_NONE;

	err = email_get_max_mail_count(&max_count);
	testapp_print("\n\t>>>>> email_get_max_mail_count() return [%d]\n\n", max_count);
	
	return false;
}

#include "em-storage.h"
static gboolean testapp_test_test_get_thread_information()
{
	int error_code, thread_id= 38;
	emf_mail_list_item_t *result_mail;
	
	if( (error_code = email_get_thread_information_ex(thread_id, &result_mail)) == EMF_ERROR_NONE) {
		testapp_print("email_get_thread_information returns = %d\n", error_code);
		testapp_print("subject = %s\n", result_mail->subject);
		testapp_print("mail_id = %d\n", result_mail->mail_id);
		testapp_print("from = %s\n", result_mail->from);
		testapp_print("thrad_id = %d\n", result_mail->thread_id);
		testapp_print("count = %d\n", result_mail->thread_item_count);
	}

	return TRUE;
}


static gboolean testapp_test_get_sender_list()
{
	testapp_print(" >>> testapp_test_get_sender_list : Entered \n");

	char *mailbox_name = NULL;
	int  i = 0;
	int account_id = 0;
	int sorting = 0;
	int err_code = EMF_ERROR_NONE;
	emf_sender_list_t *sender_list = NULL;
	int sender_count = 0;

	mailbox_name = NULL;
	account_id = 0;
	sorting = 0;

	if ( (err_code = email_get_sender_list(account_id, mailbox_name, EMF_SEARCH_FILTER_NONE, NULL, sorting, &sender_list, &sender_count)) != EMF_ERROR_NONE) {
		testapp_print("email_get_sender_list failed : %d\n",err_code);
		return FALSE;
	}

	testapp_print("===================================================\n");
	testapp_print("\t\t\tSender list\n");
	testapp_print("===================================================\n");
	testapp_print("sender address \tdisplay_name\t [unread/total]\n");
	testapp_print("===================================================\n");
	for ( i =0; i < sender_count; i++ ) {
		testapp_print("[%s]\t[%s]\t[%d/%d]\n", sender_list[i].address, sender_list[i].display_name, sender_list[i].unread_count, sender_list[i].total_count);
	}
	testapp_print("===================================================\n");

	if ( sender_list ) {
		email_free_sender_list(&sender_list, sender_count);
	}
	if ( mailbox_name ) {
		free(mailbox_name);
		mailbox_name = NULL;
	}

	testapp_print(" >>> testapp_test_get_sender_list : END \n");
	return TRUE;
}

 
static gboolean testapp_test_get_address_info_list()
{
	testapp_print(" >>> testapp_test_get_address_info_list : Entered \n");

	char buf[1024];
	int i = 0;
	int mail_id = 0;
	int err_code = EMF_ERROR_NONE;
	emf_address_info_list_t *address_info_list= NULL;
	emf_address_info_t *p_address_info = NULL;
	GList *list = NULL;
	GList *node = NULL;

	memset(buf, 0x00, sizeof(buf));
	testapp_print("\n > Enter mail_id: ");
	scanf("%d",&mail_id);		


	if ((err_code = email_get_address_info_list(mail_id, &address_info_list)) != EMF_ERROR_NONE) {
		testapp_print("email_get_address_info_list failed : %d\n",err_code);
		email_free_address_info_list(&address_info_list);
		return FALSE;
	}

	testapp_print("===================================================\n");
	testapp_print("\t\t\t address info list\n");
	testapp_print("===================================================\n");
	testapp_print("address_type\t address \t\tdisplay_name\t contact id\n");
	testapp_print("===================================================\n");

	for ( i = EMF_ADDRESS_TYPE_FROM; i <= EMF_ADDRESS_TYPE_BCC; i++ ) {
		switch ( i ) {
			case EMF_ADDRESS_TYPE_FROM:
				list = address_info_list->from;
				break;
			case EMF_ADDRESS_TYPE_TO:
				list = address_info_list->to;
				break;
			case EMF_ADDRESS_TYPE_CC:
				list = address_info_list->cc;
				break;
			case EMF_ADDRESS_TYPE_BCC:
				list = address_info_list->bcc;
				break;
		}

		/*  delete dynamic-allocated memory for each item */
		node = g_list_first(list);
		while ( node != NULL ) {
			p_address_info = (emf_address_info_t*)node->data;
			testapp_print("%d\t\t%s\t%s\t%d\n", p_address_info->address_type, p_address_info->address, p_address_info->display_name, p_address_info->contact_id);

			node = g_list_next(node);
		}
		testapp_print("\n");
	}
	testapp_print("===================================================\n");

	email_free_address_info_list(&address_info_list);

	testapp_print(" >>> testapp_test_get_address_info_list : END \n");
	return TRUE;
}

static gboolean testapp_test_find_mail_on_server()
{
	testapp_print(" >>> testapp_test_find_mail_on_server : Entered \n");

	int err_code = EMF_ERROR_NONE;
	int account_id = 0;
	int search_type = 0;
	unsigned int handle = 0;
	char mailbox_name[MAILBOX_NAME_LENGTH];
	char search_value[MAX_EMAIL_ADDRESS_LENGTH];

	testapp_print("input account id : ");
	scanf("%d",&account_id);

	testapp_print("input mailbox name : ");
	scanf("%s", mailbox_name);

	testapp_print("input search type : ");
	scanf("%d",&search_type);

	testapp_print("input search type : ");
	scanf("%s", search_value);

	if( (err_code = email_find_mail_on_server(account_id, mailbox_name, search_type, search_value, &handle)) != EMF_ERROR_NONE) {
		testapp_print("email_find_mail_on_server failed [%d]", err_code);
	}

	testapp_print(" >>> testapp_test_find_mail_on_server : END \n");
	return TRUE;
}

/* internal functions */
static gboolean testapp_test_interpret_command (int menu_number)
{
	gboolean go_to_loop = TRUE;
	
	switch (menu_number) {
		case 1:
			testapp_test_save_mail();
			break;
		case 2:
			testapp_test_mail_send ();
			break;
		case 3:
			testapp_test_update();
			break;
		case 4:
			testapp_test_add_attachment();
			break;			
		case 5:
			testapp_test_get ();
			break;
		case 6:
			testapp_test_get_info();
			break;
		case 7:
			testapp_test_get_header ();
			break;			
		case 8:
			testapp_test_get_body ();
			break;
		case 9:
			testapp_test_count ();
			break;
		case 10:
			testapp_test_modify_flag();
			break;
		case 11:
			testapp_test_modify_extra_flag ();
			break;	
		case 12:
			testapp_test_download();
			break;	
		case 14:
			testapp_test_delete ();
			break;
 		case 16:
			testapp_test_download_body ();
			break;
		case 17:
			testapp_test_download_attachment();
			break;		
		case 18:
			testapp_test_load_test_mail_send();
			break;
		case 19:
			testapp_test_get_mailbox_list ();
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
			testapp_test_find();
			break;
 		case 26:
			testapp_test_count_message_on_sending();
			break;
		case 27:
			testapp_test_move_all_mails_to_mailbox();
			break;
		case 28:
			testapp_test_count_message_with_draft_flag();
			break;
		case 29:
			testapp_test_get_latest_unread_mail_id();
			break;
		case 38:
			testapp_test_get_totaldiskusage();
			break;
		case 39:
			testapp_test_send_mail_with_option();
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
		case 49:
			testapp_test_get_sender_list();
			break;
 		case 51:
			testapp_test_get_mail_list_ex();
			break;
		case 52:
			testapp_test_get_address_info_list();
			break;
		case 53:
			testapp_test_send_html_mail_with_inline_content();
			break;
		case 54:
			email_get_recipients_list(1, NULL, NULL);
			break;
		case 55:
			testapp_test_set_flags_field();
			break;
		case 56:
			testapp_test_add_mail();
			break;
		case 57:
			testapp_test_update_mail();
			break;
		case 58:
			testapp_test_find_mail_on_server();
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
		testapp_show_menu (EMF_MESSAGE_MENU);
		testapp_show_prompt (EMF_MESSAGE_MENU);
			
		scanf ("%d", &menu_number);

		go_to_loop = testapp_test_interpret_command (menu_number);
	}
}

