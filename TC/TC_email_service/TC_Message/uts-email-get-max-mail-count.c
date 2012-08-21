/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-max-mail-count.h"


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






/*Testcase   :		uts_Email_Get_Max_Mail_Count_01
  TestObjective  :	To get the maximum count of emails
  APIs Tested    :	int email_get_max_mail_count(int *Count)  
 */

static void uts_Email_Get_Max_Mail_Count_01()
{
	int err_code = EMAIL_ERROR_NONE;
	int count = 0;
	
	tet_infoline("uts_Email_Get_Max_Mail_Count_01 Begin\n");

	err_code = email_get_max_mail_count(&count);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email Get Max Mail Count success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email Get Max Mail Count Failed err_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}				
}


/*Testcase   :		uts_Email_Get_Max_Mail_Count_02
  TestObjective  :	validate parameter
  APIs Tested    :	int email_get_max_mail_count(int *Count)  
 */

static void uts_Email_Get_Max_Mail_Count_02()
{
	int err_code = EMAIL_ERROR_NONE;
	
	tet_infoline("uts_Email_Get_Max_Mail_Count_01 Begin\n");

	err_code = email_get_max_mail_count(NULL);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email Get Max Mail Count success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email Get Max Mail Count Failed err_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}				
}



