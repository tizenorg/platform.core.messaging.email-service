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
#include "uts_email_cancel_job.h"
#include "../TC_Utility/uts-email-real-utility.c"


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


/*Testcase   :		uts_email_cancel_job_01
  TestObjective  :	To cancel the ongoing job
  APIs Tested    :	int email_cancel_job(int account_id, int handle)  
 */

static void uts_Email_Cancel_Job_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle = 0;

	tet_infoline("uts_Email_cancel_job_01 Begin\n");

	err_code = email_sync_header_for_all_account(&handle);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email sync mail header for all account : success\n");
		err_code = email_cancel_job(0, handle);
		if (EMAIL_ERROR_NONE == err_code) {
				tet_printf("Email cancel job  : success\n");
					tet_result(TET_PASS);
		}
		else {
				tet_printf("Email cancel job failed : error_code[%d]\n", err_code);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_printf("Email sync mail header for all account failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}
}


/*Testcase   :		uts_email_cancel_job_02
  TestObjective  :	To validate the parameter for account id in  cancel the ongoing job
  APIs Tested    :	int email_cancel_job(int account_id, int handle)  
 */
static void uts_Email_Cancel_Job_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int account_id = -1;
	int handle;

	tet_infoline("uts_Email_cancel_job_10 Begin\n");

	err_code = email_sync_header_for_all_account(&handle);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email sync mail header for all account : success\n");
		err_code = email_cancel_job(account_id, handle);
		if (EMAIL_ERROR_NONE != err_code) {
			tet_printf("Email cancel job  : success\n");
			tet_result(TET_PASS);
		}   
		else {   
			tet_printf("Email cancel job failed : \n");
			tet_result(TET_FAIL);
		}   
	}   
	else {   
		tet_printf("Email sync mail header for all account failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}   
}

