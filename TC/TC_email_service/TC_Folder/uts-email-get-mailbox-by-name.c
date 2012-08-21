/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-mailbox-by-name.h"
#include "../TC_Utility/uts-email-dummy-utility.c"


sqlite3 *sqlite_emmb; 
static int g_accountId; 
		

static void startup()
{
	int err_code = EMF_ERROR_NONE;
	int count = 0;
	int i = 0;
	emf_account_t *pAccount = NULL ;

	tet_printf("\n TC startup");
	
	if (EMF_ERROR_NONE == email_service_begin()) {
		tet_infoline("Email service Begin\n");
		if (EMF_ERROR_NONE == email_open_db()) {
				err_code = email_get_account_list(&pAccount, &count);
				if (!count) {   
				g_accountId = uts_Email_Add_Dummy_Account_01();
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
	
	if (EMF_ERROR_NONE == email_close_db()) {
		tet_infoline("Email Close DB Success\n");
		if (EMF_ERROR_NONE == email_service_end())
		tet_infoline("Email service close Success\n");
		else
			tet_infoline("Email service end failed\n");
	}
 	else
			tet_infoline("Email Close DB failed\n");		
}







/*Testcase   :		uts_Email_Get_Mailbox_By_Name_01
  TestObjective  :	To get mailbox information by name
  APIs Tested    :	int email_get_mailbox_by_name(int account_id, const char *pMailboxName, emf_mailbox_t **mailbox) 
 */

static void uts_Email_Get_Mailbox_By_Name_01()
{
	int err_code = EMF_ERROR_NONE ;
	emf_mailbox_t *pMailbox = NULL;
	char *pMailboxName;

	tet_infoline("uts_Email_Get_Mailbox_By_Name_01 :  Begin\n");

			pMailboxName = strdup("INBOX");
	err_code = email_get_mailbox_by_name(g_accountId , pMailboxName, &pMailbox);
			if (EMF_ERROR_NONE == err_code) {	
				tet_infoline("Email get mailbox by name  :  Success\n");	
					tet_result(TET_PASS);
			}
			else {
   				tet_printf("Email get mailbox by name failed : error_code[%d]\n", err_code);
					tet_result(TET_FAIL);
			}
			
}


/*Testcase   :		uts_Email_Get_Mailbox_by_Name_02
  TestObjective  :	To validate parameter for account_id in  get mailbox by name
  APIs Tested    :	int email_get_mailbox_by_name(int account_id, const char *pMailboxName, emf_mailbox_t **pMailbox) 
 */

static void uts_Email_Get_Mailbox_By_Name_02()
{
	int err_code = EMF_ERROR_NONE ;
	emf_mailbox_t *pMailbox = NULL;
	char *pMailboxName;
	int account_id = -1;

	tet_infoline("uts_Email_Get_Mailbox_By_Name_02 Begin\n");

	pMailboxName = strdup("INBOX");
	err_code = email_get_mailbox_by_name(account_id , pMailboxName, &pMailbox);
	if (EMF_ERROR_NONE != err_code) {	
		tet_infoline("Email get mailbox by name validate parameter  :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
			tet_printf("Email get mailbox by name validate parameter failed : \n");
			tet_result(TET_FAIL);
	}	
}



/*Testcase   :		uts_Email_Get_Mailbox_by_Name_03
  TestObjective  :	To validate parameter for pMailboxName  in  get mailbox by name
  APIs Tested    :	int email_get_mailbox_by_name(int account_id, const char *pMailboxName, emf_mailbox_t **pMailbox) 
 */

static void uts_Email_Get_Mailbox_By_Name_03()
{
	int err_code = EMF_ERROR_NONE ;
	emf_mailbox_t *pMailbox = NULL;

	tet_infoline("uts_Email_Get_Mailbox_By_Name_03 Begin\n");

	err_code = email_get_mailbox_by_name(g_accountId , NULL, &pMailbox);
	if (EMF_ERROR_NONE != err_code) {	
		tet_infoline("Email get mailbox by name validate parameter  :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
			tet_printf("Email get mailbox by name validate parameter failed : \n");
			tet_result(TET_FAIL);
	}	
}


/*Testcase   :		uts_Email_Get_Mailbox_by_Name_04
  TestObjective  :	To validate parameter for pMailbox in  get mailbox by name
  APIs Tested    :	int email_get_mailbox_by_name(int account_id, const char *pMailboxName, emf_mailbox_t **pMailbox) 
 */

static void uts_Email_Get_Mailbox_By_Name_04()
{
	int err_code = EMF_ERROR_NONE ;
	char *pMailboxName;

	tet_infoline("uts_Email_Get_Mailbox_By_Name_04 Begin\n");

	pMailboxName = strdup("INBOX");
	err_code = email_get_mailbox_by_name(g_accountId, pMailboxName, NULL);
	if (EMF_ERROR_NONE != err_code) {	
		tet_infoline("Email get mailbox by name validate parameter  :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
			tet_printf("Email get mailbox by name validate parameter failed : \n");
			tet_result(TET_FAIL);
	}	
}
