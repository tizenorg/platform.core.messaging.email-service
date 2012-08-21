/*
 * Copyright (c) 2010  Samsung Electronics, Co., Ltd.
 * All rights reserved.
 *
 * This software is a confidential and proprietary information of Samsung
 * Electronics, Co., Ltd. ("Confidential Information").  You shall not disclose such
 * Confidential Information and shall use it only in accordance with the terms
 * of the license agreement you entered into with Samsung Electronics.
 */

/* #include <tet_api.h */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "email-api-init.h"
#include "email-api-account.h"
#include "email-api-mail.h"

//#define email_get_mail_list(account_id, mailbox_name, mail_list, count, sorting) email_get_mail_list_ex(account_id, mailbox_name, EMAIL_LIST_TYPE_NORMAL, -1, -1, sorting, mail_list, count)
#define EMAIL_GET_MAIL_LIST_MAX 20

int uts_Email_Add_Dummy_Account_01(void);
int uts_Email_Add_Dummy_Message_02(void);
int uts_Email_Get_Mail_List_03(int account_id, email_mailbox_type_e mailbox_type, email_mail_list_item_t** mail_list,  int* result_count, email_sort_type_t sorting);

/* struct tet_testlist tet_testlist[] =  */
/*					*/
/* 			{ uts_Email_Add_Dummy_Account_01, 1} */
/* 			{ uts_Email_Add_Dummy_Message_02, 2} */
/* 			{NULL, 0 */
/* } */
