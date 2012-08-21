/*
 * Copyright (c) 2010  Samsung Electronics, Co., Ltd.
 * All rights reserved.
 *
 * This software is a confidential and proprietary information of Samsung
 * Electronics, Co., Ltd. ("Confidential Information").  You shall not disclose such
 * Confidential Information and shall use it only in accordance with the terms
 * of the license agreement you entered into with Samsung Electronics.
 */

#include <tet_api.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "email-api-init.h"
#include "email-api-account.h"
#include "email-api-mailbox.h"

static void startup(), cleanup();
void (*tet_startup) () = startup;
void (*tet_cleanup) () = cleanup;


static void uts_Email_Get_Pending_Job_01(void);
static void uts_Email_Get_Pending_Job_02(void);
static void uts_Email_Get_Pending_Job_03(void);





struct tet_testlist tet_testlist[] = { {uts_Email_Get_Pending_Job_01, 1},  {uts_Email_Get_Pending_Job_02, 2},  {uts_Email_Get_Pending_Job_03, 3},  {NULL, 0}
};			
