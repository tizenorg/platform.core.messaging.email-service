/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-move-mail-to-folder.h"
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





/*Testcase   : 		uts_Email_Move_Mail_To_Folder_01
  TestObjective  : 	To move mails to a mailbox
  APIs Tested    : 	int email_move_mail_to_mailbox(int *mail_ids, int num, int input_target_mailbox_id)
 */

static void uts_Email_Move_Mail_To_Folder_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int mail_ids[] = {1, 2};
	int num = 0;
	int new_mailbox_id = 0;
	int count = 0;
	email_mailbox_t mbox;
	email_mail_list_item_t *mail_list = NULL;
	int mailbox_type = EMAIL_MAILBOX_TYPE_TRASH;
	email_mailbox_t *pMailbox = NULL;

	tet_infoline("uts_Email_Move_Mail_To_Folder_01 Begin\n");
	
	if (g_accountId <= 0) {
		tet_infoline("g_accountId is invalid\n");
		tet_result(TET_UNRESOLVED);		
	}
	
	count = 0;
	err_code = uts_Email_Get_Mail_List_03(g_accountId, 0, &mail_list, &count, 0);
	if (EMAIL_ERROR_NONE != err_code || count < 0) {				
		tet_printf("uts_Email_Get_Mail_List_03 Failed  : err_code[%d]", err_code);
		tet_result(TET_UNRESOLVED);
		return;
	}

	err_code = email_get_mailbox_by_mailbox_type(g_accountId , mailbox_type, &pMailbox);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email get mailbox by mailbox type failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}

	mail_ids[0] = mail_list[0].mail_id;
	num = 1;
	new_mailbox_id = pMailbox->mailbox_id;

	tet_printf("New mailbox  : new_mailbox_id[%d]\n", new_mailbox_id);

	err_code = email_move_mail_to_mailbox(mail_ids, num, new_mailbox_id);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_printf("Email Move Mail To mailbox success\n");
		tet_result(TET_PASS);
	}
	else		{
		
		tet_printf("Email Move Mail To mailbox failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
	if (pMailbox) {
		email_free_mailbox(&pMailbox, 1);
		pMailbox = NULL;
	}

}


/*Testcase   : 		uts_Email_Move_Mail_To_Folder_02
  TestObjective  : 	To validate parameter for mail move to mailbox
  APIs Tested    : 	int email_move_mail_to_mailbox(int *mail_ids, int num, int input_target_mailbox_id)
 */ 
static void uts_Email_Move_Mail_To_Folder_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int *mail_ids = NULL; /* mail ids initialized to NULL*/
	int num = 2;
	email_mailbox_t *pMailbox = NULL;
	int count = 0;
	
	tet_infoline("uts_Email_Move_Mail_To_Folder_02 Begin\n");

	err_code = email_get_mailbox_by_mailbox_type(g_accountId , EMAIL_MAILBOX_TYPE_TRASH, &pMailbox);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email get mailbox by mailbox type failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}

	err_code = email_move_mail_to_mailbox(mail_ids, num, pMailbox->mailbox_id);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email Move Mail To mailbox success\n");
		tet_result(TET_PASS);
	}
	else		{
		
		tet_printf("Email Move Mail To mailbox failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}	
	if (pMailbox) {
		email_free_mailbox(&pMailbox, 1);
		pMailbox = NULL;
	}
}

/*Testcase   : 		uts_Email_Move_Mail_To_Folder_03
  TestObjective  : 	To validate parameter for mail move to mailbox
  APIs Tested    : 	int email_move_mail_to_mailbox(int *mail_ids, int num, int input_target_mailbox_id)
 */ 
static void uts_Email_Move_Mail_To_Folder_03()
{
	int err_code = EMAIL_ERROR_NONE;
	int mail_ids[] = {1, 2};
	int num = -1; /* Boundary condition test */
	email_mailbox_t *pMailbox = NULL;
	int count = 0;
	
	tet_infoline("uts_Email_Move_Mail_To_Folder_03 Begin\n");

	err_code = email_get_mailbox_by_mailbox_type(g_accountId , EMAIL_MAILBOX_TYPE_TRASH, &pMailbox);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email get mailbox by mailbox type failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}

	err_code = email_move_mail_to_mailbox(mail_ids, num, pMailbox->mailbox_id);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email Move Mail To mailbox success\n");
		tet_result(TET_PASS);
	}
	else		{
		
		tet_printf("Email Move Mail To mailbox failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}	
	if (pMailbox) {
		email_free_mailbox(&pMailbox, 1);
		pMailbox = NULL;
	}
}

/*Testcase   : 		uts_Email_Move_Mail_To_Folder_04
  TestObjective  : 	To validate parameter for mail move to mailbox
  APIs Tested    : 	int email_move_mail_to_mailbox(int *mail_ids, int num, int input_target_mailbox_id)
 */ 
static void uts_Email_Move_Mail_To_Folder_04()

{
	int err_code = EMAIL_ERROR_NONE;
	int mail_ids[] = {1, 2};
	int num = 2;
	int input_target_mailbox_id = 0; /* input_target_mailbox_id as 0 */
	
	tet_infoline("uts_Email_Move_Mail_To_Folder_04 Begin\n");

	err_code = email_move_mail_to_mailbox(mail_ids, num, input_target_mailbox_id);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email Move Mail To mailbox success\n");
		tet_result(TET_PASS);
	}
	else		{
		
		tet_printf("Email Move Mail To mailbox failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}	

}



