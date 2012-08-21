/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-mailbox-by-mailboxtype.h"
#include "../TC_Utility/uts-email-dummy-utility.c"


sqlite3 *sqlite_emmb; 
static int g_accountId; 
		

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
				g_accountId = uts_Email_Add_Dummy_Account_01();
			}   
			else
			g_accountId = pAccount[i].account_id;
			tet_printf("g_accountId [%d], count[%d]\n", g_accountId, count);
			email_free_account(&pAccount, count);

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






/*Testcase   :		uts_Email_Get_Mailbox_By_Mailboxtype_01
  TestObjective  :	To get mailbox by mailbox type
  APIs Tested    :	int email_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type,  email_mailbox_t **mailbox) 
 */

static void uts_Email_Get_Mailbox_By_Mailboxtype_01()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_mailbox_t *pMailbox = NULL;
	int mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
	
	tet_infoline("uts_Email_Get_Mailbox_By_Mailboxtype_01 :  Begin\n");

	err_code = email_get_mailbox_by_mailbox_type(g_accountId , mailbox_type, &pMailbox);
	if (EMAIL_ERROR_NONE == err_code) {	
		tet_infoline("Email get mailbox by mailbox type :  Success\n");	
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email get mailbox by mailbox type failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
	
}


/*Testcase   :		uts_Email_Get_Mailbox_By_Mailboxtype_02
  TestObjective  :	To validate parameter for account_id in  get mailbox by mailbox_type
  APIs Tested    :	int  email_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type,  email_mailbox_t **mailbox) 
 */

static void uts_Email_Get_Mailbox_By_Mailboxtype_02()
{
	int err_code = EMAIL_ERROR_NONE ;
	email_mailbox_t *pMailbox = NULL;
	int account_id = -1;
	int mailbox_type = EMAIL_MAILBOX_TYPE_USER_DEFINED;

	tet_infoline("uts_Email_Get_Mailbox_By_Mailboxtype_02 :  Begin\n");

	err_code = email_get_mailbox_by_mailbox_type(account_id , mailbox_type, &pMailbox);
	if (EMAIL_ERROR_NONE != err_code) {	
		tet_infoline("Email get mailbox by mailbox type :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
 		tet_printf("Email get mailbox by mailbox type failed : \n");
			tet_result(TET_FAIL);
		}
		
}


/*Testcase   :		uts_Email_Get_Mailbox_By_Mailboxtype_03
  TestObjective  :	To validate parameter for mailbox in  get mailbox by mailbox_type
  APIs Tested    :	int  email_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type,  email_mailbox_t **mailbox) 
 */

static void uts_Email_Get_Mailbox_By_Mailboxtype_03()
{
	int err_code = EMAIL_ERROR_NONE ;
	int mailbox_type = EMAIL_MAILBOX_TYPE_USER_DEFINED;

	tet_infoline("uts_Email_Get_Mailbox_By_Mailboxtype_03 :  Begin\n");

	err_code = email_get_mailbox_by_mailbox_type(g_accountId , mailbox_type, NULL);
	if (EMAIL_ERROR_NONE != err_code) {	
		tet_infoline("Email get mailbox by mailbox type :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
 		tet_printf("Email get mailbox by mailbox type failed : \n");
			tet_result(TET_FAIL);
		}
}


