/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-account-list.h"


sqlite3 *sqlite_emmb; 


static void startup()
{
	tet_printf("\n TC startup");
	if (EMAIL_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
		if (EMAIL_ERROR_NONE == email_open_db())
			tet_infoline("Email open DB success\n");
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






/*Testcase   : 		uts_Email_Get_Account_List_01
  TestObjective  : 	To get account list    
  APIs Tested    : 	int email_get_account_list(email_account_t **account, int *count)
 */ 

static void uts_Email_Get_Account_List_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	int i = 0;
	email_account_t *pAccount = NULL ;
	
	tet_infoline("uts_Email_Get_Account_List_01 : Begin\n");
		
	err_code = email_get_account_list(&pAccount, &count);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email get account list success\n");
		for (i ; i < count;i++)
			tet_printf("account_id[%d]  account_name[%s]  email_addr[%s]\n",
				pAccount[i].account_id,	pAccount[i].account_name, pAccount[i].user_email_address);
		
		tet_result(TET_PASS);
	}
	else		{
		tet_printf("Email get account list failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}		
}


/*Testcase   : 		uts_Email_Get_Account_List_02
  TestObjective  : 	To validate parameter for account_list in get account list    
  APIs Tested    : 	int email_get_account_list(email_account_t **account_list, int *count)
 */ 

static void uts_Email_Get_Account_List_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	
	tet_infoline("uts_Email_Get_Account_List_02 : Begin\n");
		
	err_code = email_get_account_list(NULL, &count);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email get account list success\n");
		tet_result(TET_PASS);
	}
	else		{
		tet_printf("Email get account list failed : \n");
		tet_result(TET_FAIL);
	}		
}


/*Testcase   : 		uts_Email_Get_Account_List_03
  TestObjective  : 	To validate parameter for count in  get account list    
  APIs Tested    : 	int email_get_account_list(email_account_t **account, int *count)
 */ 

static void uts_Email_Get_Account_List_03()
{
	int err_code = EMAIL_ERROR_NONE;
	email_account_t *pAccount = NULL ;
	
	tet_infoline("uts_Email_Get_Account_List_03 : Begin\n");
		
	err_code = email_get_account_list(&pAccount, 0);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email get account list success\n");
		tet_result(TET_PASS);
	}
	else		{
		tet_printf("Email get account list failed : \n");
		tet_result(TET_FAIL);
	}		
}

