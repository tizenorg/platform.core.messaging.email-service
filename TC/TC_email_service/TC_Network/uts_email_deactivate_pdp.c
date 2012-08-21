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
#include "uts_email_deactivate_pdp.h"
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


/*Testcase   :		uts_Email_Deactivate_Pdp_01
  TestObjective  :	To Deactivate pdp
  APIs Tested    :	int email_deactivate_pdp(int thread_type)  
 */

static void uts_Email_Deactivate_Pdp_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int thread_type = 1;

	tet_infoline("uts_Email_Deactivate_Pdp_01 Begin\n");

	err_code = email_deactivate_pdp(thread_type);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_printf("Email deactivate pdp  : success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email deactivate pdp failed : error_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
}


/*Testcase   :		uts_Email_Deactivate_Pdp_02
  TestObjective  :	To validate parameter for deactivate pdp
  APIs Tested    :	int email_deactivate_pdp(int thread_type)  
 */

static void uts_Email_Deactivate_Pdp_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int thread_type = -1;
	
	tet_infoline("uts_Email_Deactivate_pdp_01 Begin\n");

	err_code = email_deactivate_pdp(thread_type);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_printf("Email deactivate pdp  : success\n");
		tet_result(TET_PASS);
	}   
	else {   
		tet_printf("Email deactivate pdp failed : \n");
		tet_result(TET_FAIL);
	}   
}

