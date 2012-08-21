/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-add-message.h"
#include "../TC_Utility/uts-email-dummy-utility.c"


sqlite3 *sqlite_emmb;
static int g_accountId; 


static void startup()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	int i = 0;
	email_account_t *pAccount = NULL ;
	email_mailbox_t mbox;

	tet_printf("\n TC startup");
	if (EMAIL_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
		if (EMAIL_ERROR_NONE == email_open_db()) {
			tet_infoline("Email open DB success\n");

			g_accountId = 0;
			/*  Check if there are any accounts and Get account id. If there is no account in the db, add new dummy account data to the d */
			err_code = email_get_account_list(&pAccount, &count);
			if (EMAIL_ERROR_NONE == err_code) {
				/* get the account id from the d */
				g_accountId = pAccount[i].account_id;
				email_free_account(&pAccount, count);	
				tet_printf("g_accountId[%d]\n", g_accountId);
			}
			else if (EMAIL_ERROR_ACCOUNT_NOT_FOUND == err_code) {
				/*  Add new dummy account to the db if there is no account in the d */
				tet_printf("Add new account\n");
				g_accountId = uts_Email_Add_Dummy_Account_01();
				tet_printf("g_accountId[%d]\n", g_accountId);
			}
			else {
				tet_printf("email_get_account_list() failed :  err_code[%d]\n", err_code);
			}

		}
		else
			tet_infoline("Email open DB failed\n");
	}
	else {	
		tet_infoline("Email service not started\n");	
	}
}

static void cleanup()
{		
	tet_printf("\n TC End");

	if (EMAIL_ERROR_NONE == email_close_db()) {
		tet_infoline("Email Close DB Success\n");
		if (EMAIL_ERROR_NONE == email_service_end())
			tet_infoline("Email service close Success\n");
		else
			tet_infoline("Email service end failed\n");
	}
	else
		tet_infoline("Email Close DB failed\n");
}






/*Testcase   :		uts_Email_Add_Message_01
  TestObjective  :	To save message
  APIs Tested    :	int email_add_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas)
 */

static void uts_Email_Add_Message_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	email_mail_data_t       *test_mail_data = NULL;
	email_attachment_data_t *attachment_data = NULL;
	email_meeting_request_t *meeting_req = NULL;

	FILE *fp;
	email_option_t option;
	int count = 0;
	int i = 0;
	email_account_t *account = NULL ;
	email_mailbox_t *mailbox = NULL;

	tet_infoline("uts_Email_Add_Message_01 Begin\n");


	memset(&option, 0x00, sizeof(email_option_t));
	option.keep_local_copy = 1;


	test_mail_data = malloc(sizeof(email_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));

	attachment_data = malloc(sizeof(email_attachment_data_t));
	memset(attachment_data, 0x00, sizeof(email_attachment_data_t));


	err_code = email_get_account_list(&account, &count);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email get account list success\n");
		if (count > 0) {
			test_mail_data->account_id = account[i].account_id;
			g_accountId = account[i].account_id;
			test_mail_data->flags_draft_field = 1;

			tet_printf("g_accountId[%d]\n", g_accountId);

			test_mail_data->full_address_from = strdup("<samsungtest09@gmail.com>");
			test_mail_data->full_address_to = strdup("<samsungtest09@gmail.com>");
			test_mail_data->subject = strdup("Test");

			err_code = email_get_mailbox_by_mailbox_type(g_accountId , EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
			if (EMAIL_ERROR_NONE != err_code) {
				tet_printf("Email get mailbox by mailbox type failed : error_code[%d]\n", err_code);
				tet_result(TET_UNRESOLVED);
				return ;
			}

			test_mail_data->mailbox_id           = mailbox->mailbox_id;
			test_mail_data->mailbox_type         = mailbox->mailbox_type;

			test_mail_data->save_status          = 1;
			test_mail_data->body_download_status = 1;

			fp = fopen("/tmp/mail.txt", "w");
			fprintf(fp, "xxxxxxxxx");
			fclose(fp);
			test_mail_data->file_path_plain = strdup("/tmp/mail.txt");
			test_mail_data->attachment_count = 1;

			fp = fopen("/tmp/attach.txt", "w");
			fprintf(fp, "Simple Attachment");
			fclose(fp);

			attachment_data->attachment_name = strdup("Attach");
			attachment_data->attachment_path = strdup("/tmp/attach.txt");
			attachment_data->save_status      = 1;

			err_code = email_add_mail(test_mail_data, attachment_data, 1, NULL, 0);
			if (EMAIL_ERROR_NONE == err_code) {
				tet_infoline("Email add message Success\n");
				tet_result(TET_PASS);
			}
			else {
				tet_printf("Email add message failed : error_code[%d]\n", err_code);
				tet_result(TET_FAIL);
			}
		}
		else {
			tet_infoline("NO email account found : success\n");
			tet_result(TET_UNRESOLVED);
		}
	}
	else {
		tet_printf("Email get account list failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}
	tet_infoline("Free email data\n");

	email_free_attachment_data(&attachment_data, 1);
	email_free_mail_data(&test_mail_data, 1);
	if (mailbox) {
		email_free_mailbox(&mailbox, 1);
		mailbox = NULL;
	}
}


/*Testcase   :		uts_Email_Add_Message_02
  TestObjective  :	To validate parameter for  mail in add message
  APIs Tested    :	int email_add_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas)
 */

static void uts_Email_Add_Message_02()
{
	int err_code = EMAIL_ERROR_NONE;


	tet_infoline("uts_Email_Add_Message_02 Begin\n");

	err_code = email_add_mail(NULL, NULL, 0, NULL, 0);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email add message  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_infoline("Email add message  :  failed \n");
		tet_result(TET_FAIL);
	}


}

