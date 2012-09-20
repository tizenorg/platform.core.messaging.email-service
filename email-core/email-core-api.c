/*
*  email-service
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Kyuho Jo <kyuho.jo@samsung.com>, Sunghyun Kwon <sh0701.kwon@samsung.com>
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/


/******************************************************************************
 * File :  email-core-api.h
 * Desc :  Mail Engine API
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "c-client.h"
#include "email-core-global.h"
#include "email-network.h"
#include "email-core-event.h"
#include "email-core-mailbox.h"
#include "email-core-utils.h"
#include "email-debug-log.h"

extern void *
pop3_parameters(long function, void *value);
extern void *
imap_parameters(long function, void *value);



/* initialize mail core */
INTERNAL_FUNC int emcore_init(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if (err_code != NULL) {
		*err_code = EMAIL_ERROR_NONE;
	}

	mail_link(&imapdriver);  /*  link in the imap driver  */
	mail_link(&pop3driver);  /*  link in the pop3 driver  */

	mail_link(&unixdriver);	 /*  link in the unix driver  */
	mail_link(&dummydriver); /*  link in the dummy driver  */

	ssl_onceonlyinit();

	auth_link(&auth_md5);    /*  link in the md5 authenticator  */
	auth_link(&auth_pla);    /*  link in the pla authenticator  */
	auth_link(&auth_log);    /*  link in the log authenticator  */

	/* Disabled to authenticate with plain text */
	mail_parameters(NIL, SET_DISABLEPLAINTEXT, (void *) 2);

	/* Set max trials for login */
	imap_parameters(SET_MAXLOGINTRIALS, (void *)1);
	pop3_parameters(SET_MAXLOGINTRIALS, (void *)1);
	smtp_parameters(SET_MAXLOGINTRIALS, (void *)1);

	mail_parameters(NIL, SET_SSLCERTIFICATEQUERY, (void *)emnetwork_callback_ssl_cert_query);
	mail_parameters(NIL, SET_SSLCAPATH, (void *)SSL_CERT_DIRECTORY);

	/* Set time out in second */
	mail_parameters(NIL, SET_OPENTIMEOUT  , (void *)50);
	mail_parameters(NIL, SET_READTIMEOUT  , (void *)180);
	mail_parameters(NIL, SET_WRITETIMEOUT , (void *)180);
	mail_parameters(NIL, SET_CLOSETIMEOUT , (void *)30);

	if (err_code)
		*err_code = EMAIL_ERROR_NONE;

    return true;
}

