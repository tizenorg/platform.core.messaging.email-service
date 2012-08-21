/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-clear-mail-data.h"


sqlite3 *sqlite_emmb;
static int g_accountId;


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






/*Testcase   :		uts_Email_Clear_Mail_Data_01
  TestObjective  :	To Clear Mail data
  APIs Tested    :	int email_clear_mail_data()  
 */

static void uts_Email_Clear_Mail_Data_01()
{
	int err_code = EMAIL_ERROR_NONE;
	err_code = email_clear_mail_data();
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email clear mail data Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email clear mail data failed : error_code[%d]\n", err_code);
		tet_result(TET_UNRESOLVED);
	}
 }


/*Testcase   :		uts_Email_Clear_Mail_Data_02
  TestObjective  :	To validate parameter 
  APIs Tested    :	int email_clear_mail_data()  
 */

static void uts_Email_Clear_Mail_Data_02()
{
	int err_code = EMAIL_ERROR_NONE;

	/*  this function has no paramete */
	tet_result(TET_PASS);
}



