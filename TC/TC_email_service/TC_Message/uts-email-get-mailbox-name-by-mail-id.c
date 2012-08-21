/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-mailbox-name-by-mail-id.h"
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






/*Testcase   :		uts_Email_Get_Mailbox_Name_By_Mail_Id_01
  TestObjective  :	To get mailBox naem by mailID
  APIs Tested    :	int email_get_mailbox_name_by_mail_id(int mail_id, char **pMailbox_name)
 */

static void uts_Email_Get_Mailbox_Name_By_Mail_Id_01()
{
	int err_code = EMF_ERROR_NONE;
	char *pMailboxName = NULL;
	int mail_id = 0;
	emf_mailbox_t mbox;
	emf_mail_list_item_t *mail_list = NULL;
	emf_mail_t *pMail = NULL;
	emf_account_t *account = NULL ;
	int account_count = 0;
	int count = 0;


	tet_infoline("uts_Email_Get_Mailbox_Name_By_Mail_Id_01 Begin\n");

	memset(&mbox, 0x00, sizeof(emf_mailbox_t));
	mbox.name = strdup("INBOX");

	err_code = email_get_account_list(&account, &account_count);
	if (EMF_ERROR_NONE != err_code || account_count <= 0) {
		tet_printf("Email Get Account List Failed  : err_code[%d]", err_code);
		tet_result(TET_UNRESOLVED);
		return;
	}
	mbox.account_id = account[0].account_id ;
	count = 0;
	err_code = email_get_mail_list(mbox.account_id, mbox.name, &mail_list, &count, 0);
	if (EMF_ERROR_NONE != err_code || count <= 0) {				
		tet_printf("Email Get Account List Failed  : err_code[%d]", err_code);
		tet_result(TET_UNRESOLVED);
		return;
	}
	mail_id = mail_list[0].mail_id;
	err_code = email_get_mailbox_name_by_mail_id(mail_id , &pMailboxName);
			if (EMF_ERROR_NONE == err_code) {
		tet_infoline("Email get Mailbox Name By Mail id Success\n");
			tet_result(TET_PASS);
	}
			else {
		tet_printf("Email get Mailbox Name By Mail id Failed\n");
			tet_result(TET_FAIL);
	}     

	email_free_account(&account, account_count);
/* 	email_free_mail_list(&mail_list, count) */
	free(mail_list);
	if (pMailboxName)
		free(pMailboxName);
}


/*Testcase   :		uts_Email_Get_Mailbox_Name_By_Mail_Id_02
  TestObjective  :	To validate parameter for mail_id in get mailbox name by mailID
  APIs Tested    :	int email_get_mailbox_name_by_mail_id(int mail_id, char **pMailbox_name)
 */

static void uts_Email_Get_Mailbox_Name_By_Mail_Id_02()
{
	int err_code = EMF_ERROR_NONE;
	char *pMailboxName = NULL;
	int mail_id = -1;
	
	tet_infoline("uts_Email_Get_Mailbox_Name_By_Mail_Id_02 Begin\n");

	err_code = email_get_mailbox_name_by_mail_id(mail_id, &pMailboxName);
	if (EMF_ERROR_NONE != err_code) {
		tet_infoline("Email get mailbox name by Mail id Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email get mailbox name by mail id  failed : \n");
		tet_result(TET_FAIL);
	}				
}


/*Testcase   :		uts_Email_Get_Mailbox_Name_By_Mail_Id_03
  TestObjective  :	To validate parameter for pMailbox_name in get mailbox name by mailID
  APIs Tested    :	int email_get_mailbox_name_by_mail_id(int mail_id, char **pMailbox_name)
 */

static void uts_Email_Get_Mailbox_Name_By_Mail_Id_03()
{
	int err_code = EMF_ERROR_NONE;
	char *pMailboxName = NULL;
	int mail_id = 1;
	
	tet_infoline("uts_Email_Get_Mailbox_Name_By_Mail_Id_03 Begin\n");

	err_code = email_get_mailbox_name_by_mail_id(mail_id, NULL);
	if (EMF_ERROR_NONE != err_code) {
		tet_infoline("Email get mailbox name by Mail id Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email get mailbox name by mail id  failed : \n");
		tet_result(TET_FAIL);
	}				
}
