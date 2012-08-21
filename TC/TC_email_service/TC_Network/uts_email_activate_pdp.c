/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/

/***************************************
* add your header file here
* e.g., 
* #include "uts_ApplicationLib_recurGetDayOfWeek_func.h"
***************************************/
#include "uts_email_activate_pdp.h"
#include "../TC_Utility/uts-email-real-utility.c"


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
			if (!count) {   
				g_accountId = uts_Email_Add_Real_Account_01();
			}   
			else
				g_accountId = pAccount[i].account_id;
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




/***************************************
* add your Test Cases
* e.g., 
* static void uts_ApplicationLib_recurGetDayOfWeek_01()
* {
* 	int ret;
*	
* 	ret = target_api();
* 	if (ret == 1)
* 		tet_result(TET_PASS);
* 	else
* 		tet_result(TET_FAIL);	
* }
*
* static void uts_ApplicationLib_recurGetDayOfWeek_02()
* {
* 		code..
*		condition1
* 			tet_result(TET_PASS);
*		condition2
*			tet_result(TET_FAIL);
* }
*
***************************************/


/*Testcase   :		uts_Email_Activate_Pdp_01
  TestObjective  :	To activate pdp
  APIs Tested    :	int email_activate_pdp(int thread_type)  
 */

static void uts_Email_Activate_Pdp_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int thread_type = 1;

	tet_infoline("uts_Email_Activate_Pdp_01 Begin\n");

	err_code = email_activate_pdp(thread_type);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_printf("Email activate pdp  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email activate pdp failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
}


/*Testcase   :		uts_Email_Activate_Pdp_02
  TestObjective  :	To validate parameter for activate pdp
  APIs Tested    :	int email_activate_pdp(int thread_type)  
 */

static void uts_Email_Activate_Pdp_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int thread_type = -1;
	
	tet_infoline("uts_Email_Activate_pdp_01 Begin\n");

	err_code = email_activate_pdp(thread_type);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email activate pdp  : success\n");
		tet_result(TET_PASS);
	}   
	else {   
		tet_printf("Email activate pdp failed : \n");
		tet_result(TET_FAIL);
	}   
}

