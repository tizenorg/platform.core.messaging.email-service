/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-service-end.h"


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
}





/*Testcase   : 		uts_Email_Service_End_01
  TestObjective  : 	To close email service 
  APIs Tested    : 	int email_service_end() 
 */

static void uts_Email_Service_End_01()
{
	if (EMAIL_ERROR_NONE == email_close_db()) {
		tet_infoline("Email Close DB Success\n");

		if (EMAIL_ERROR_NONE == email_service_end()) {
			tet_infoline("Email service close Success\n");
			tet_result(TET_PASS);
		}
		else {
			tet_infoline("Email service end failed\n");
			tet_result(TET_FAIL);
		}
	}
	else
		tet_infoline("Email Close DB failed\n");

}


