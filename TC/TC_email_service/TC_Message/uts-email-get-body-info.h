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

#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "Emf_Mapi_Init.h"
#include "Emf_Mapi_Account.h"
#include "Emf_Mapi_Message.h"

static void startup(), cleanup();
void (*tet_startup) () = startup;
void (*tet_cleanup) () = cleanup;

static void uts_Email_Get_Body_Info_01(void);
static void uts_Email_Get_Body_Info_02(void);
static void uts_Email_Get_Body_Info_03(void);
static void uts_Email_Get_Body_Info_04(void);





struct tet_testlist tet_testlist[] = {
 { uts_Email_Get_Body_Info_01, 1},  { uts_Email_Get_Body_Info_02, 2},  { uts_Email_Get_Body_Info_03, 3},  { uts_Email_Get_Body_Info_04, 4},  {NULL, 0}


};
			
