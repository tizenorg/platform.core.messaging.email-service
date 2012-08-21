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
#include "uts_email_get_pending_job.h"
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


/*Testcase   :		uts_Email_Get_Pending_job_01
  TestObjective  :	To get pending job list
  APIs Tested    :	int email_get_pending_job(email_action_t action, int account_id, int mail_id, email_event_status_type_t *status)  
 */

static void uts_Email_Get_Pending_Job_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle = 0;

	tet_infoline("uts_Email_Get_pending_Job_01 Begin\n");

	err_code = email_get_pending_job(EMAIL_ACTION_SEND_MAIL, g_accountId, 1, &handle);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_printf("Email get pending job  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email get pending  job failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
}


/*Testcase   :		uts_Email_Get_Pending_Job_02
  TestObjective  :	To get pending job list
  APIs Tested    :	int email_get_pending_job(email_action_t action, int account_id, int mail_id, email_event_status_type_t *status)  
 */

static void uts_Email_Get_Pending_Job_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle = 0;
	int account_id = -1;
	int mail_id = 1;

	tet_infoline("uts_Email_Get_Pending_Job_02 Begin\n");

	err_code = email_get_pending_job(EMAIL_ACTION_SEND_MAIL, account_id, mail_id, &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email get pending job  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email get pending  job failed : \n");
		tet_result(TET_FAIL);
	}
}


/*Testcase   :		uts_Email_Get_Pending_Job_03
  TestObjective  :	To get pending job list
  APIs Tested    :	int email_get_pending_job(email_action_t action, int account_id, int mail_id, email_event_status_type_t *status)  
 */

static void uts_Email_Get_Pending_Job_03()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle = 0;
	int account_id = -1 ;
	int mail_id = 0;

	tet_infoline("uts_Email_Network_11 Begin\n");

	err_code = email_get_pending_job(EMAIL_ACTION_SEND_MAIL, account_id , mail_id, &handle);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email get pending job  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email get pending  job failed : \n");
		tet_result(TET_FAIL);
	}
}
