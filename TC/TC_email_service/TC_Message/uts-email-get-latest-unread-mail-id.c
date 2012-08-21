/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-latest-unread-mail-id.h"
#include "../TC_Utility/uts-email-dummy-utility.c"


sqlite3 *sqlite_emmb;
static int g_accountId; 



static void startup()
{
	int err_code = EMF_ERROR_NONE;
	int count = 0;
	int i = 0;
	emf_account_t *pAccount = NULL ;
	emf_mailbox_t mbox;
	emf_mail_list_item_t *mail_list = NULL;

	tet_printf("\n TC startup");
	if (EMF_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
		if (EMF_ERROR_NONE == email_open_db()) {
			tet_infoline("Email open DB success\n");

			g_accountId = 0;
			/*  Check if there are any accounts and Get account id. If there is no account in the db, add new dummy account data to the d */
			err_code = email_get_account_list(&pAccount, &count);
			if (EMF_ERROR_NONE == err_code) {
				/* get the account id from the d */
				g_accountId = pAccount[i].account_id;
				email_free_account(&pAccount, count);	
				tet_printf("g_accountId[%d]\n", g_accountId);
			}
			else if (EMF_ERROR_ACCOUNT_NOT_FOUND == err_code) {
				/*  Add new dummy account to the db if there is no account in the d */
				tet_printf("Add new account\n");
				g_accountId = uts_Email_Add_Dummy_Account_01();
				tet_printf("g_accountId[%d]\n", g_accountId);
			}
			else {
				tet_printf("email_get_account_list() failed :  err_code[%d]\n", err_code);
			}

			/*  Check if there are any mails and Get mail id of one of them. If there is no mail in the db, add new dummy mail data to the d */
			memset(&mbox, 0x00, sizeof(emf_mailbox_t));
			mbox.name = NULL;	/*  for all mailboxe */
			mbox.account_id = ALL_ACCOUNT;
			count = 0;
			err_code = email_get_mail_list(mbox.account_id, mbox.name, &mail_list, &count, 0);
			if (EMF_ERROR_NONE == err_code) {
				tet_printf("email_get_mail_list() success\n");
			}
			else if (EMF_ERROR_MAIL_NOT_FOUND == err_code) {
				tet_printf("Add new email\n");
				err_code = uts_Email_Add_Dummy_Message_02();
				if (EMF_ERROR_NONE == err_code) {	/*  Make db contain at least one mai */
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

	if (EMF_ERROR_NONE == email_close_db()) {
		tet_infoline("Email Close DB Success\n");
		if (EMF_ERROR_NONE == email_service_end())
			tet_infoline("Email service close Success\n");
		else
			tet_infoline("Email service end failed\n");
	}
	else
		tet_infoline("Email Close DB failed\n");
}






/*Testcase   :		uts_Email_Get_Latest_Unread_Mail_Id_01
  TestObjective  :	To get the latest unread mail id of given account
  APIs Tested    :	int email_get_latest_unread_mail_id(int account_id, int *pMailID)
 */

static void uts_Email_Get_Latest_Unread_Mail_Id_01()
{
	int err_code = EMF_ERROR_NONE;
	int account_id = -1;
	int mail_id = 0;
	
	tet_infoline("uts_Email_Get_Latest_Unread_Mail_Id_01 Begin\n");

	err_code = email_get_latest_unread_mail_id(account_id, &mail_id);
	if (EMF_ERROR_NONE == err_code) {
		tet_infoline("Email get Latest Unread mail id Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email get Latest Unread mail id  failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}				
}



/*Testcase   :		uts_Email_Get_Latest_Unread_Mail_Id_02
  TestObjective  :	To validate parameter for account_id in  get the latest unread mail id of given account
  APIs Tested    :	int email_get_latest_unread_mail_id(int account_id, int *pMailID)
 */

static void uts_Email_Get_Latest_Unread_Mail_Id_02()
{
	int err_code = EMF_ERROR_NONE;
	int mail_id = 0;	

	tet_infoline("uts_Email_Get_Latest_Unread_Mail_Id_02 Begin\n");
	
	err_code = email_get_latest_unread_mail_id(0, &mail_id);
	if (EMF_ERROR_NONE != err_code) {
		tet_infoline("Email get latest Unread mail id  Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_infoline("Email get latest Unread mail id Failed\n");
		tet_result(TET_FAIL);
	}
				
}



/*Testcase   :		uts_Email_Get_Latest_Unread_Mail_Id_03
  TestObjective  :	To validate parameter for pMailID in  get the latest unread mail id of given account
  APIs Tested    :	int email_get_latest_unread_mail_id(int account_id, int *pMailID)
 */

static void uts_Email_Get_Latest_Unread_Mail_Id_03()
{
	int err_code = EMF_ERROR_NONE;
	int account_id = 0;	

	tet_infoline("uts_Email_Get_Latest_Unread_Mail_Id_03 Begin\n");
	
	err_code = email_get_latest_unread_mail_id(account_id, 0);
	if (EMF_ERROR_NONE != err_code) {
		tet_infoline("Email get latest Unread mail id  Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_infoline("Email get latest Unread mail id Failed\n");
		tet_result(TET_FAIL);
	}
				
}
