/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-delete-all-message-in-folder.h"
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







/*Testcase   :		uts_Email_Delete_All_Message_In_Folder_01
  TestObjective  :	To delete all message in mailbox message
  APIs Tested    :	int email_delete_all_mails_in_mailbox(int input_mailbox_id, int input_from_server)
  */

static void uts_Email_Delete_All_Message_In_Folder_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle;
	int count = 0;
	int i = 0;
	email_account_t *account = NULL ;
	email_mailbox_t *mailbox = NULL;

	tet_infoline("utc_Email_Delete_All_Message_In_Folder_01 Begin\n");

	err_code = email_get_account_list(&account, &count);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email get account list success\n");
		if (count > 0) {
			err_code = email_get_mailbox_by_mailbox_type(account[i].account_id , EMAIL_MAILBOX_TYPE_SENTBOX, &mailbox);
			if (EMAIL_ERROR_NONE != err_code) {
				tet_printf("Email get mailbox by mailbox type failed : error_code[%d]\n", err_code);
				tet_result(TET_UNRESOLVED);
			}
			err_code = email_delete_all_mails_in_mailbox(mailbox->mailbox_id, 0);
			if (EMAIL_ERROR_NONE == err_code) {
				tet_infoline("Email delete all message in mailbox :  success\n");
				tet_result(TET_PASS);
			}
			else {
				tet_printf("Email delete all message in mailbox failed : error_code[%d]\n", err_code);
				tet_printf("account_id[%d], mailbox->mailbox_id[%d]\n", account[i].account_id, mailbox->mailbox_id);
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

	if (mailbox) {
		email_free_mailbox(&mailbox, 1);
		mailbox = NULL;
	}

}

																							

/*Testcase   :		uts_Email_Delete_All_Message_In_Folder_02
  TestObjective  :	To validate parameter for mailbox in delete all message in mailbox message
  APIs Tested    :	int email_delete_all_mails_in_mailbox(int input_mailbox_id, int input_from_server)
  */

static void uts_Email_Delete_All_Message_In_Folder_02()
{
	int err_code = EMAIL_ERROR_NONE;

	tet_infoline("utc_Email_delete_all_Message_in_folder  Begin\n");

	err_code = email_delete_all_mails_in_mailbox(0, 0);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email delete all_message_in_folder  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_infoline("Email delete  all_message in mailbox  :  failed \n");
		tet_result(TET_FAIL);
	}
}
