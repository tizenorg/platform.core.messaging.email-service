/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-update-account.h"
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
			if (!count)	{																	
				g_accountId = uts_Email_Add_Dummy_Account_01();										
			}
			else
				g_accountId = pAccount[i].account_id ;	
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





 
/*Testcase   : 		uts_Email_Update_Account_01
  TestObjective  : 	To update account information of an email account   
  APIs Tested    : 	int email_update_account(int account_id, email_account_t *new_account)
 */ 

static void uts_Email_Update_Account_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	int i = 0;
	email_account_t *pAccount = NULL ;
	email_account_t *pNewAccount = NULL ;
	
	tet_infoline("uts_Email_Update_Account_01 : Begin\n");
	
	err_code = email_get_account(g_accountId, GET_FULL_DATA, &pNewAccount);
			tet_printf("account_id[%d]  account_name[%s]  email_addr[%s]\n",
				pNewAccount[i].account_id, pNewAccount[i].account_name, pNewAccount[i].user_email_address);

			pNewAccount[i].account_name = strdup("Test_Tc");
			pNewAccount[i].user_email_address = strdup("Test@gmail.com");

			err_code = email_update_account(pNewAccount->account_id , pNewAccount);
			if (EMAIL_ERROR_NONE == err_code) {
				tet_infoline("Email update account success\n");
				tet_result(TET_PASS);
			}
			else		{
				tet_printf("Email update account failed : error_code[%d]\n", err_code);
				tet_result(TET_FAIL);
			}  
		}
	
/*Testcase  :	uts_Email_Update_Account_02  
  TestObjective  :   To validate update  account parameter.
  APIs tested  :     Tint email_update_account(int account_id, email_account_t *new_account);
 */ 
 
static void uts_Email_Update_Account_02()
{			
	int account_id;
	int err_code = EMAIL_ERROR_NONE;
	email_account_t *pAccount = NULL;

	tet_infoline("uts_Email_Account_02 : Begin\n");
	account_id = -1;

	pAccount = (email_account_t  *) malloc(sizeof(email_account_t));
	memset(pAccount, 0x00, sizeof(email_account_t));

		pAccount->account_name = strdup("UpdateTest");

	err_code = email_update_account(account_id, pAccount); 
	if (EMAIL_ERROR_NONE == err_code)  {
		tet_printf(" email Update Account Falied : \n");
		tet_result (TET_FAIL);						
	}
	else {
		tet_printf ("email Update Account successful \n");
		tet_result (TET_PASS);			
	}
  }

 

/*Testcase  :	uts_Email_Update_Account_03  
  TestObjective  :   To validate update  account parameter.
  APIs tested  :     Tint email_update_account(int account_id, email_account_t *new_account);
 */ 
 
static void uts_Email_Update_Account_03()
{			
	int account_id = 1;
	int err_code = EMAIL_ERROR_NONE;
	email_account_t *pAccount = NULL;

	tet_infoline("utc_Email_Account_14 : Begin\n");

	err_code = email_update_account(account_id, pAccount); 
	if (EMAIL_ERROR_NONE == err_code)  {
		tet_printf(" email Update Account :  Falied\n");
		tet_result (TET_FAIL);						
	}
	else {
		tet_printf ("email Update Account :  successful \n");
		tet_result (TET_PASS);			
	}
  }

