/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-delete-attachment.h"
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
			err_code = uts_Email_Get_Mail_List_03(g_accountId, EMAIL_MAILBOX_TYPE_INBOX, &mail_list, &count, 0);
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






/*Testcase   :		uts_Email_Delete_Attachment_01
  TestObjective  :	To delete a attachment from  mail
  APIs Tested    :	int email_delete_attachment(int attachment_id)
 */

static void uts_Email_Delete_Attachment_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int attachment_id = 0;
	int i = 0;
	email_attachment_data_t *mail_attach_info = NULL;
	email_mail_list_item_t *mail_list = NULL;
	int count = 0;
	email_account_t *account = NULL ;

	tet_infoline("uts_Email_Delete_Attachment_01 Begin\n");

	err_code = email_get_account_list(&account, &count);
	if (EMAIL_ERROR_NONE != err_code || count < 0) {
		tet_printf("Email Get Account List Failed :  err_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
		return;
	}
	g_accountId = account[i].account_id;

	err_code = uts_Email_Get_Mail_List_03(g_accountId, EMAIL_MAILBOX_TYPE_INBOX, &mail_list, &count, 0);
	if (EMAIL_ERROR_NONE != err_code || count < 0) {
		tet_printf("Email Get Account List Failed :  err_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
		return;
	}
	for (i ; i < count ; i++) {
		if(mail_list[i].has_attachment) {
			err_code = email_get_attachment_data_list(mail_list[i].mail_id, &mail_attach_info, &count);
			if (EMAIL_ERROR_NONE == err_code) {
				err_code = email_delete_attachment(mail_attach_info->attachment_id);
				if (EMAIL_ERROR_NONE == err_code) {
					tet_infoline("Email delete attachment  :  success\n");
					tet_result(TET_PASS);
					return ;
				}
				else {
					tet_printf("Email delete attachment  :  Failed err_code[%d]\n", err_code);
					tet_result(TET_FAIL);
					return ;
				}

			}else {
				tet_printf("Email get attachment data list  :  Failed err_code[%d]\n", err_code);
				tet_result(TET_UNRESOLVED);
			}
		}
	}
}


/*Testcase   :		uts_Email_Delete_Attachment_02
  TestObjective  :	To validate parmete for mailbox in delete a attachment from  mail
  APIs Tested    :	int email_delete_attachment(int attachment_id)
 */

static void uts_Email_Delete_Attachment_02()
{
	int err_code = EMAIL_ERROR_NONE;

	tet_infoline("uts_Email_Delete_Attachment_02 Begin\n");

	err_code = email_delete_attachment(0);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email delete attachment  :  success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email delete attachment  :  Failed \n");
		tet_result(TET_FAIL);
	}

}
