/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-disk-space-usage.h"


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






/*Testcase   :		uts_Email_Get_Disk_Space_Usage_01
  TestObjective  :	To get the total disk usage of emails
  APIs Tested    :	int email_get_disk_space_usage(unsigned long *total_size)  
 */

static void uts_Email_Get_Disk_Space_Usage_01()
{
	int err_code = EMAIL_ERROR_NONE;
	unsigned long total_size = 0;
	
	tet_infoline("uts_Email_Get_Disk_Space_Usage_01 Begin\n");

	err_code = email_get_disk_space_usage(&total_size);
	if (EMAIL_ERROR_NONE == err_code) {
		tet_infoline("Email Get Disk Space Usage success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email Get Disk Space Usage Failed err_code[%d]\n", err_code);
		tet_result(TET_FAIL);
	}				
}



/*Testcase   :		uts_Email_Get_Disk Space_Usage_02
  TestObjective  :	To validate parameter for total_size in get disk space usage
  APIs Tested    :	int  email_get_disk_space_usage(unsigned long *total_size)  
 */

static void uts_Email_Get_Disk_Space_Usage_02()
{
	int err_code = EMAIL_ERROR_NONE;
 		
	tet_infoline("uts_Email_Get_Disk_Space_Usage_02 Begin\n");

	err_code = email_get_disk_space_usage(0);
	if (EMAIL_ERROR_NONE != err_code) {
		tet_infoline("Email Get Disk Space Usage Success\n");
		tet_result(TET_PASS);
	}
	else {
		tet_printf("Email Get Disk Space Usage failed\n");
		tet_result(TET_FAIL);
	}
				
}



