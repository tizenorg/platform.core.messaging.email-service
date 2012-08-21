/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-init-storage.h"


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





/*Testcase   : 		uts_Email_Init_Storage_01
  TestObjective  : 	To initialize email storage 
  APIs Tested    : 	int email_init_storage() 
 */

static void uts_Email_Init_Storage_01()
{
	int err_code = EMAIL_ERROR_NONE;

	tet_infoline("uts_Email_Init_Storage_01");
	err_code = email_init_storage();
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("email init storage  :  Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("uts_Email_Init_Storage_01 failed : err_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}
}


