/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-delete-account.h"
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
			email_free_account(&pAccount, count);	
			
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






 
/*Testcase   : 		uts_Email_Delete_Account_01
  TestObjective  : 	To delete an email account   
  APIs Tested    : 	int email_delete_account(int account_id)
 */ 

static void uts_Email_Delete_Account_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int account_id = 0;
	int count = 0;
	int i = 0;
	email_account_t *pAccount = NULL ;
	
	tet_infoline("uts_Email_Delete_Account_01 : Begin\n");
	account_id = g_accountId;
	

	err_code = email_delete_account(account_id);
	if (EMAIL_ERROR_NONE == err_code) {
			tet_infoline("Email delete account  : success\n");
			tet_result(TET_PASS);
	}
	else		{
			tet_printf("Email delete account failed : error_code[%d]\n", err_code);
				tet_result(TET_FAIL);
	}    	
}


/*Testcase   : 		uts_Email_Delete_Account_02
  TestObjective  : 	To validate parameter for  delete an email account   
  APIs Tested    : 	int email_delete_account(int account_id)
 */ 

static void uts_Email_Delete_Account_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int account_id = -1;
		
	tet_infoline("uts_Email_Delete_Account_02 : Begin\n");
		
	err_code = email_delete_account(account_id);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email delete account success\n");
		tet_result(TET_PASS);
	} 
	else		{
		tet_printf("Email delete account failed : \n");
		tet_result(TET_FAIL);
	}		
}

