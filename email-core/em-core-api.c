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
 * File :  em-core-api.h
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
#include "em-core-global.h"
#include "em-network.h"
#include "em-core-event.h"
#include "em-core-mailbox.h"
#include "em-core-utils.h"
#include "emf-dbglog.h"

extern void *
pop3_parameters(long function, void *value);
extern void *
imap_parameters(long function, void *value);

/*
 * encoding base64
 */
int em_core_encode_base64(char *src, unsigned long src_len, char **enc, unsigned long* enc_len, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

    unsigned char *content;
    int ret = true, err = EMF_ERROR_NONE;

	if (err_code != NULL) {
		*err_code = EMF_ERROR_NONE;
	}

    content = rfc822_binary(src, src_len, enc_len);

    if (content)
        *enc = (char *)content;
    else {
        err = EMF_ERROR_UNKNOWN;
        ret = false;
    }

    if (err_code)
        *err_code = err;

	EM_DEBUG_FUNC_END();
    return ret;
}

/*
 * decoding quoted-printable
 */
int em_core_decode_quotedprintable(unsigned char *enc_text, unsigned long enc_len, char **dec_text, unsigned long* dec_len, int *err_code)
{
    unsigned char *text = enc_text;
    unsigned long size = enc_len;
    unsigned char *content;
    int ret = true, err = EMF_ERROR_NONE;

	if (err_code != NULL) {
		*err_code = EMF_ERROR_NONE;
	}

    EM_DEBUG_FUNC_BEGIN();

    content = rfc822_qprint(text, size, dec_len);

    if (content)
        *dec_text = (char *)content;
    else
    {
        err = EMF_ERROR_UNKNOWN;
        ret = false;
    }

    if (err_code)
        *err_code = err;

    return ret;
}

/*
 * decoding base64
 */
int em_core_decode_base64(unsigned char *enc_text, unsigned long enc_len, char **dec_text, unsigned long* dec_len, int *err_code)
{
    unsigned char *text = enc_text;
    unsigned long size = enc_len;
    unsigned char *content;
    int ret = true, err = EMF_ERROR_NONE;

	if (err_code != NULL) {
		*err_code = EMF_ERROR_NONE;
	}

    EM_DEBUG_FUNC_BEGIN();

    content = rfc822_base64(text, size, dec_len);
    if (content)
        *dec_text = (char *)content;
    else
    {
        err = EMF_ERROR_UNKNOWN;
        ret = false;
    }

    if (err_code)
        *err_code = err;

    return ret;
}

/**
  * em_core_get_logout_status - Get the logout status
  **/
EXPORT_API void
em_core_get_logout_status(int  *status)
{
	EM_DEBUG_FUNC_BEGIN();
	int logout_status = 0;
	*status = logout_status;
	EM_DEBUG_LOG("em_core_get_logout_status is not implemented");
	EM_DEBUG_FUNC_END("status [%d]", logout_status);
}


/**
  * em_core_set_logout_status - Set the logout status
  **/
EXPORT_API void
em_core_set_logout_status(int status)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_LOG("em_core_set_logout_status is not implemented");
	EM_DEBUG_FUNC_END("status [%d] ", status);
}

#define EM_BATTERY_RECOVER_TEMP    6
#define EM_BATTERY_ONLINE          1
int _check_charger_status(int *charging_status, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("err_code[%p] ",  err_code);

	int ret = false;
	int err = EMF_ERROR_NONE;
	int ps_value = 0;
	
	*charging_status = ps_value;

	ret = true;

	if (err_code != NULL)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

EXPORT_API
int em_low_battery_noti_init()
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_LOG("Not implemented");
	EM_DEBUG_FUNC_END();
	return 0;
}

/* initialize mail core */
EXPORT_API int em_core_init(int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	if (err_code != NULL) {
		*err_code = EMF_ERROR_NONE;
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

	mail_parameters(NIL, SET_SSLCERTIFICATEQUERY, (void *)em_core_ssl_cert_query_cb);
	mail_parameters(NIL, SET_SSLCAPATH, (void *)SSL_CERT_DIRECTORY);

	/* Set time out in second */
	mail_parameters(NIL, SET_OPENTIMEOUT  , (void *)50);
	mail_parameters(NIL, SET_READTIMEOUT  , (void *)180);
	mail_parameters(NIL, SET_WRITETIMEOUT , (void *)180);
	mail_parameters(NIL, SET_CLOSETIMEOUT , (void *)30);

	if (err_code)
		*err_code = EMF_ERROR_NONE;

    return true;
}

