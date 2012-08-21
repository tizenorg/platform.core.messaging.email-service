/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/

/***************************************
* add your header file here
* e.g., 
* #include "uts_ApplicationLib_recurGetDayOfWeek_func.h"
***************************************/
#include "uts_email_send_mail.h"
#include "../TC_Utility/uts-email-real-utility.c"
#include "../TC_Utility/uts-email-dummy-utility.c"


sqlite3 *sqlite_emmb; 
int g_accountId;

static void startup()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	int i = 0;
	email_account_t *pAccount = NULL ;

	tet_printf("\n TC startup");
		
	if (EMAIL_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
		if (EMAIL_ERROR_NONE == email_open_db()) {
			tet_infoline("Email open DB success\n");
			err_code = email_get_account_list(&pAccount, &count);
			if (!count) {   
			g_accountId = uts_Email_Add_Real_Account_01();
			}   
			else
			g_accountId = pAccount[i].account_id;
			email_free_account(&pAccount, count);

			tet_printf("Add new email\n");
			err_code = uts_Email_Add_Real_Message_02();
				if (EMAIL_ERROR_NONE == err_code) {	/*  Make db contain at least one mai */
					tet_printf("uts_Email_Add_Real_Message_02() success.\n");
				}
			else {
					tet_printf("uts_Email_Add_Real_Message_02() failed  : err_code[%d]\n", err_code);
			}
			
		}
		else
			tet_infoline("Email open DB failed\n");
	}
	else
		tet_infoline("Email service not started\n");   
	
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




/***************************************
* add your Test Cases
* e.g., 
* static void uts_ApplicationLib_recurGetDayOfWeek_01()
* {
* 	int ret;
*	
* 	ret = target_api();
* 	if (ret == 1)
* 		tet_result(TET_PASS);
* 	else
* 		tet_result(TET_FAIL);	
* }
*
* static void uts_ApplicationLib_recurGetDayOfWeek_02()
* {
* 		code..
*		condition1
* 			tet_result(TET_PASS);
*		condition2
*			tet_result(TET_FAIL);
* }
*
***************************************/

/*Testcase   :		uts_Email_Send_Mail_01
  TestObjective  :	To send a mail
  APIs Tested    :	int email_send_mail(int mail_id, email_option_t* sending_option, unsigned* handle)
 */

static void uts_Email_Send_Mail_01()
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

	tet_infoline("uts_Email_Send_Mail_01 Begin\n");

	memset(&option, 0x00, sizeof(email_option_t));
	option.keep_local_copy = 1;

	test_mail_data = malloc(sizeof(email_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));

	attachment_data = malloc(sizeof(email_attachment_data_t));
	memset(attachment_data, 0x00, sizeof(email_attachment_data_t));
	
	test_mail_data->account_id = g_accountId;
	test_mail_data->flags_draft_field = 1;

	test_mail_data->full_address_from = strdup("<samsungtest09@gmail.com>");
	test_mail_data->full_address_to = strdup("<samsungtest09@gmail.com>");
	test_mail_data->subject = strdup("Test");

	err_code = email_get_mailbox_by_mailbox_type(g_accountId , EMAIL_MAILBOX_TYPE_OUTBOX, &mailbox);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email get mailbox by mailbox type failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
		return ;
	}

	test_mail_data->mailbox_id           = mailbox->mailbox_id;
	test_mail_data->mailbox_type         = mailbox->mailbox_type;

	fp = fopen("/tmp/mail.txt", "w");
	fprintf(fp, "xxxxxxxxx");
	fclose(fp);

	test_mail_data->file_path_plain = strdup("/tmp/mail.txt");
	test_mail_data->attachment_count = 1;

	fp = fopen("/tmp/attach.txt", "w");
	fprintf(fp, "Simple Attach");
	fclose(fp);	

	attachment_data->attachment_name = strdup("Attach");
	attachment_data->attachment_path = strdup("/tmp/attach.txt");

	err_code = email_add_mail(test_mail_data, attachment_data, 1, NULL, 0);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("email_add_mail Success\n");
		tet_printf("account_id[%d], test_mail_data->mail_id[%d]", g_accountId, test_mail_data->mail_id);
		err_code = email_send_mail(test_mail_data->mail_id, &option, &handle);
		if (EMAIL_ERROR_NONE == err_code) {
			tet_infoline("email_send_mail : success\n");
			tet_result(TET_PASS);
		}
		else {
			tet_printf("email_send_mail failed : error_code[%d]\n", err_code);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_printf("email_add_message failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}
	if (mailbox) {
		email_free_mailbox(&mailbox, 1);
		mailbox = NULL;
	}
}



/*Testcase   :		uts_Email_Send_Mail_02
  TestObjective  :	To validate parameter for send a mail
  APIs Tested    :	int email_send_mail(int mail_id, email_option_t* sending_option, unsigned* handle)
 */

static void uts_Email_Send_Mail_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	email_option_t option;
	
	tet_infoline("Email Email_send_mail_02 Begin\n");

	memset(&option, 0x00, sizeof(email_option_t));
	option.keep_local_copy = 1;
	err_code = email_send_mail(0, &option, &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email send mail : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email send mail failed : \n");
		tet_result(TET_FAIL);
	}
}

