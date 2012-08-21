/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-open-db.h"


sqlite3 *sqlite_emmb; 
static int g_accountId; 



static void startup()
{
	tet_printf("\n TC startup");
	if (EMAIL_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
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





/*Testcase   : 		uts_Email_Open_Db_01
  TestObjective  : 	To Open Db 
  APIs Tested    : 	int email_open_db() 
 */

static void uts_Email_Open_Db_01()
{
	if (EMAIL_ERROR_NONE == email_open_db()) {	
		tet_infoline("Email open DB success\n");
		tet_result(TET_PASS);	
	}
  	else {
		tet_infoline("Email open DB failed\n");
		tet_result(TET_FAIL);
	}
}
