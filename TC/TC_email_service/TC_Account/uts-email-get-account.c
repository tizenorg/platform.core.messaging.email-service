/*
* Copyright (c) 2010  Samsung Electronics, Inc.
* All rights reserved.
*
* This software is a confidential and proprietary information of Samsung
* Electronics, Inc. ("Confidential Information").  You shall not disclose such
* Confidential Information and shall use it only in accordance with the terms
* of the license agreement you entered into with Samsung Electronics.
*/


#include "uts-email-get-account.h"
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
			if (!count)	{
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






/*Testcase   : 		uts_Email_Get_Account_01
  TestObjective  : 	To get account information based on account id
  APIs Tested    : 	int email_get_account(int account_id, int pulloption, email_account_t **account)
 */

static void uts_Email_Get_Account_01()
{
	int err_code = EMAIL_ERROR_NONE;
	email_account_t *pAccount = NULL ;

	tet_infoline("uts_Email_Account_01 : Begin\n");

	err_code = email_get_account(g_accountId, GET_FULL_DATA, &pAccount);
			if (EMAIL_ERROR_NONE == err_code) {
				tet_infoline("Email get account success\n");
				tet_printf("account_name[%s]\n email_addr[%s]\n", pAccount->account_name, pAccount->user_email_address);
				tet_result(TET_PASS);
			}
			else		{
				tet_printf("Email get account failed : error_code[%d]\n", err_code);
				tet_result(TET_FAIL);
			}
		}

/*Testcase   : 		uts_Email_Get_Account_02
  TestObjective  : 	To validate parameter for account_id in get account information based on account id
  APIs Tested    : 	int email_get_account(int account_id, int pulloption, email_account_t **account)
 */

static void uts_Email_Get_Account_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int account_id = -1;
	email_account_t *pAccount = NULL ;

	tet_infoline("uts_Email_Get_Account_02 : Begin\n");

	err_code = email_get_account(account_id, GET_FULL_DATA, &pAccount);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email get account success\n");
		tet_result(TET_PASS);
	}
	else		{
		tet_printf("Email get account failed : \n");
		tet_result(TET_FAIL);
	}
}


/*Testcase   : 		uts_Email_Get_Account_03
  TestObjective  : 	To validate parameter for account in  get account information based on account id
  APIs Tested    : 	int email_get_account(int account_id, int pulloption, email_account_t **account)
 */

static void uts_Email_Get_Account_03()
{
	int err_code = EMAIL_ERROR_NONE;

	tet_infoline("uts_Email_Get_Account_03 : Begin\n");

	err_code = email_get_account(g_accountId, GET_FULL_DATA, NULL);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email get account success\n");
		tet_result(TET_PASS);
	}
	else		{
		tet_printf("Email get account failed : \n");
		tet_result(TET_FAIL);
	}
}



