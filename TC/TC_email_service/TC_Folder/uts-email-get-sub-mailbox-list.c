/* 
* Copyright (c) 2010  Samsung Electronics, Inc. 
* All rights reserved. 
* 
* This software is a confidential and proprietary information of Samsung 
* Electronics, Inc. ("Confidential Information").  You shall not disclose such 
* Confidential Information and shall use it only in accordance with the terms 
* of the license agreement you entered into with Samsung Electronics. 
*/


#include "uts-email-get-sub-mailbox-list.h"
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
			tet_infoline("Email open DB success\n");
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







/*Testcase   :		uts_Email_Get_Sub_Mailbox_List_01
  TestObjective  :	To get sub folderlist
  APIs Tested    :	int email_get_child_mailbox_list(int account_id, const char *parent_folder, emf_mailbox_t **mailbox_list, int *count) 
 */

static void uts_Email_Get_Sub_Mailbox_List_01()
{
	int err_code = EMF_ERROR_NONE ;
	emf_mailbox_t *pMailbox_list = NULL;
	int i = 0;
	int count = 0;
	char *pParent_folder;
	emf_account_t *pAccount = NULL ;

	tet_infoline("uts_Email_Get_Sub_Mailbox_List_01 :  Begin\n");
	pParent_folder = strdup("INBOX");

	err_code = email_get_child_mailbox_list(g_accountId , pParent_folder, &pMailbox_list, &count);
			if (EMF_ERROR_NONE == err_code) {	
				tet_infoline("Email get subfolder list :  Success\n");	
						tet_result(TET_PASS);
			}
				else {
					tet_printf("Email get subfolder list failed : error_code[%d]\n", err_code);
				tet_result(TET_FAIL);
			}
		
}


/*Testcase   :		uts_Email_Get_Sub_Mailbox_List_02
  TestObjective  :	To validate parameter for account_id in  get sub folderlist
  APIs Tested    :	int email_get_child_mailbox_list(int account_id, const char *parent_folder, emf_mailbox_t **mailbox_list, int *count) 
 */

static void uts_Email_Get_Sub_Mailbox_List_02()
{
	int err_code = EMF_ERROR_NONE ;
	emf_mailbox_t *pMailbox_list = NULL;
	int count = 0;
	int account_id = -1;
	char *pParent_folder = NULL;

	tet_infoline("uts_Email_Get_Sub_Mailbox_List_02 :  Begin\n");

	pParent_folder = strdup("INBOX");
	err_code = email_get_child_mailbox_list(account_id , pParent_folder, &pMailbox_list, &count);
	if (EMF_ERROR_NONE != err_code) {	
		tet_infoline("Email get subfolder list :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
 		tet_printf("Email get subfolder list failed : \n");
			tet_result(TET_FAIL);
		}
		
}



/*Testcase   :		uts_Email_Get_Sub_Mailbox_List_03
  TestObjective  :	To validate parameter for parent mailbox in  get sub folderlist
  APIs Tested    :	int email_get_child_mailbox_list(int account_id, const char *parent_folder, emf_mailbox_t **mailbox_list, int *count) 
 */

static void uts_Email_Get_Sub_Mailbox_List_03()
{
	int err_code = EMF_ERROR_NONE ;
	emf_mailbox_t *pMailbox_list = NULL;
	int count = 0;
	int account_id = -1;

	tet_infoline("uts_Email_Get_Sub_Mailbox_List_03 :  Begin\n");

	err_code = email_get_child_mailbox_list(account_id , NULL, &pMailbox_list, &count);
	if (EMF_ERROR_NONE != err_code) {	
		tet_infoline("Email get subfolder list :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
 		tet_printf("Email get subfolder list failed : \n");
			tet_result(TET_FAIL);
		}
}

		
/*Testcase   :		uts_Email_Get_Sub_Mailbox_List_04
  TestObjective  :	To validate parameter for mailbox_list in  get sub folderlist
  APIs Tested    :	int email_get_child_mailbox_list(int account_id, const char *parent_folder, emf_mailbox_t **mailbox_list, int *count) 
 */

static void uts_Email_Get_Sub_Mailbox_List_04()
{
	int err_code = EMF_ERROR_NONE ;
	int count = 0;
	char *pParent_folder = NULL;

	tet_infoline("uts_Email_Get_Sub_Mailbox_List_04 :  Begin\n");

	pParent_folder = strdup("INBOX");
	err_code = email_get_child_mailbox_list(g_accountId , pParent_folder, NULL, &count);
	if (EMF_ERROR_NONE != err_code) {	
		tet_infoline("Email get subfolder list :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
 		tet_printf("Email get subfolder list failed : \n");
			tet_result(TET_FAIL);
		}
}
		
/*Testcase   :		uts_Email_Get_Sub_Mailbox_List_05
  TestObjective  :	To validate parameter for count in  get sub folderlist
  APIs Tested    :	int email_get_child_mailbox_list(int account_id, const char *parent_folder, emf_mailbox_t **mailbox_list, int *count) 
 */

static void uts_Email_Get_Sub_Mailbox_List_05()
{
	int err_code = EMF_ERROR_NONE ;
	emf_mailbox_t *pMailbox_list = NULL;
	char *pParent_folder = NULL;

	tet_infoline("uts_Email_Get_Sub_Mailbox_List_05 :  Begin\n");

	pParent_folder = strdup("INBOX");
	err_code = email_get_child_mailbox_list(g_accountId , pParent_folder, &pMailbox_list, 0);
	if (EMF_ERROR_NONE != err_code) {	
		tet_infoline("Email get subfolder list :  Success\n");	
			tet_result(TET_PASS);
	}
	else {
 		tet_printf("Email get subfolder list failed : \n");
			tet_result(TET_FAIL);
		}
}
		
