/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-update-message.h"
#include "../TC_Utility/uts-email-dummy-utility.c"



sqlite3 *sqlite_emmb;
static int g_accountId; 



static void startup()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	int i = 0;
	email_account_t *pAccount = NULL ;
	email_mail_list_item_t *mail_list = NULL;

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

			/*  Check if there are any mails and Get mail id of one of them. If there is no mail in the db, add new dummy mail data to the d */
			count = 0;
			err_code = uts_Email_Get_Mail_List_03(ALL_ACCOUNT, 0, &mail_list, &count, 0);
			if (EMAIL_ERROR_NONE == err_code) {
				tet_printf("email_get_mail_list() success\n");
			}
			else if (EMAIL_ERROR_MAIL_NOT_FOUND == err_code) {
				tet_printf("Add new email\n");
				err_code = uts_Email_Add_Dummy_Message_02();
				if (EMAIL_ERROR_NONE == err_code) {	/*  Make db contain at least one mai */
						tet_printf("uts_Email_Add_Dummy_Message_02() success.\n");
					}
				else {
						tet_printf("uts_Email_Add_Dummy_Message_02() failed  : err_code[%d]\n", err_code);
				}
			}
			else {
				tet_printf("Email Get Mail List Failed  : err_code[%d]", err_code);
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







/*Testcase   :		uts_Email_Update_Message_01
  TestObjective  :	To update message
  APIs Tested    :	int email_update_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas);
  */

static void uts_Email_Update_Message_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	int count = 0;
	email_account_t *account = NULL ;
	email_mail_list_item_t *mail_list = NULL;
	email_mail_data_t       *mail_data = NULL;
	email_attachment_data_t *test_attachment_data_list = NULL;
	email_meeting_request_t *meeting_req = NULL;
	int test_attachment_data_count = 0;

	tet_infoline("uts_Email_Update_Message_01 Begin\n");


	err_code = email_get_account_list(&account, &count);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email get account list success\n");
		if (count > 0) {
			count = 0;
			err_code = uts_Email_Get_Mail_List_03(account[0].account_id, EMAIL_MAILBOX_TYPE_INBOX, &mail_list, &count, 0);
			if (EMAIL_ERROR_NONE == err_code) {
				tet_infoline("Email get mail list : success\n");
				if (count > 0) {
					err_code = email_get_mail_data(mail_list[0].mail_id , &mail_data);
					if (EMAIL_ERROR_NONE == err_code) {
						tet_infoline("Email get mail success\n");

						mail_data->full_address_cc = strdup("samsungtest09@gmail.com");

						err_code = email_update_mail(mail_data, NULL, 0, NULL, 0);
						if (EMAIL_ERROR_NONE == err_code) {
							tet_infoline("email_update_mail  :  success\n");
							tet_result(TET_PASS);
						}
						else {
							tet_printf("email_update_mail failed : error_code[%d]\n", err_code);
							tet_result(TET_FAIL);
						}
					}
					else {
						tet_printf("Email get mail failed : error_code[%d]\n", err_code);
						tet_result(TET_UNRESOLVED);
					}
				}
				}
				else {
					tet_printf("Email get mail list failed : error_code[%d]\n", err_code);
					tet_result(TET_UNRESOLVED);
				}
//			email_free_mail_list(&mail_list, count)
				free(mail_list);
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

}

																							
/*Testcase   :		uts_Email_Update_Message_02
  TestObjective  :	To validate parameter for mail_id in  update message
  APIs Tested    :	int email_update_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas);
 */

static void uts_Email_Update_Message_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int mail_id = 0;
	
	tet_infoline("uts_Email_Update_Message_02 Begin\n");

	err_code = email_update_mail(NULL, NULL, 0, NULL, 0);

	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email update message  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_infoline("Email update message  :  failed \n");
		tet_result(TET_FAIL);
	}
}

