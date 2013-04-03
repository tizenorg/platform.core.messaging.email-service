/*
 *	email-service
 *
 * Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>	/*	Needed for the definition of va_list */
#include <glib.h>
#include <vconf.h>

#include "email-daemon.h"
#include "email-daemon-emn.h"
#include "email-debug-log.h"
#include "email-utilities.h"
#include "email-internal-types.h"
#include "email-storage.h"

#ifdef __FEATURE_OMA_EMN__

#include "msg.h"
#include "msg_storage.h"
#include "msg_transport.h"

#include <wbxml/wbxml.h>
#include <wbxml/wbxml_handlers.h>
#include <wbxml/wbxml_parser.h>
#include <wbxml/wbxml_errors.h>

static void _cb_parser_start_document(void* ctx, WB_LONG charset, const WBXMLLangEntry* lang)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_LOG("Root Element: %s", lang->publicID->xmlRootElt);
	EM_DEBUG_LOG("Public ID: %s",	 lang->publicID->xmlPublicID);
	EM_DEBUG_LOG("DTD: %s",			 lang->publicID->xmlDTD);
	EM_DEBUG_FUNC_END();
}

static void _cb_parser_end_document(void* ctx)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_FUNC_END();
}

static void _cb_parser_start_element(void* ctx, WBXMLTag* element, WBXMLAttribute** atts, WB_BOOL empty)
{
	EM_DEBUG_FUNC_BEGIN();
	WB_UTINY* p, **pp = ctx;
	WB_ULONG j = 0, len = 0;
	EM_DEBUG_LOG("parse_clb_start_element");

	if (strcasecmp((char *)wbxml_tag_get_xml_name(element), "emn") != 0) {
		EM_DEBUG_LOG("not email notification");
		return ;
	}

	if (atts == NULL) {
		EM_DEBUG_LOG("no emn attributes");
		return ;
	}

	len = EM_SAFE_STRLEN((char *)wbxml_tag_get_xml_name(element)) + 1;
	while (atts[j] != NULL) {
		len += (EM_SAFE_STRLEN((char *)wbxml_attribute_get_xml_name(atts[j])) + EM_SAFE_STRLEN((char *)wbxml_attribute_get_xml_value(atts[j])) + 7);
		j++;
	}
	len += 3;

	if (!(p = malloc(sizeof(WB_UTINY) * len + 1))) {
		return ;
	}

	EM_DEBUG_LOG("<%s", (char *)wbxml_tag_get_xml_name(element));
	sprintf((char *)p, "<%s", (char *)wbxml_tag_get_xml_name(element));

	j = 0;
	while (atts[j] != NULL) {
		EM_DEBUG_LOG(" %s=\"%s\"", (char *)wbxml_attribute_get_xml_name(atts[j]), (char *)wbxml_attribute_get_xml_value(atts[j]));
		strcat((char *)p, " ");
		strcat((char *)p, (char *)wbxml_attribute_get_xml_name(atts[j]));
		strcat((char *)p, "=");
		strcat((char *)p, "\"");
		strcat((char *)p, (char *)wbxml_attribute_get_xml_value(atts[j]));
		strcat((char *)p, "\"");
		j++;
	}

	if (empty) {
		EM_DEBUG_LOG("/>");
		strcat((char *)p, "/>");
	}
	else {
		EM_DEBUG_LOG(">");
		strcat((char *)p, ">");
	}

	*pp = p;
	EM_DEBUG_FUNC_END();
}

static void _cb_parser_end_element(void* ctx, WBXMLTag* element, WB_BOOL empty)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_FUNC_END();
}

static void _cb_parser_characters(void* ctx, WB_UTINY* ch, WB_ULONG start, WB_ULONG length)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_FUNC_END();
}

static int _get_addr_from_element(unsigned char* elm,int* type, unsigned char** incoming_server_user_name, unsigned char** host_addr, unsigned char** mbox_name, unsigned char** auth_type)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	unsigned char *p = NULL;
	unsigned char *s = NULL;
	unsigned char *user = NULL;
	unsigned char *host = NULL;
	unsigned char *mailbox = NULL;
	unsigned char *auth = NULL;

	if (!(p = (unsigned char*)strstr((char *)elm, "mailbox"))) {
		EM_DEBUG_EXCEPTION("invalid notification data");
		err = EMAIL_ERROR_XML_PARSER_FAILURE;
		goto FINISH_OFF;
	}

	p += 9;
	s = (unsigned char*)strchr((char *)p, '\"');

	if (s)
		*s = '\0';

	switch (p[0]) {
	case 'm':/*	 mailat (RFC2234) */
		*type = 1;
		p += 7;
		if (!(s = (unsigned char*)strchr((char *)p, '@'))) return 0;
		*s = '\0';
		user = (unsigned char*)EM_SAFE_STRDUP((char *)p);
		s++;
		host = (unsigned char*)EM_SAFE_STRDUP((char *)s);
		mailbox = NULL;
		auth = NULL;
		break;

	case 'p':/*	 pop3 (RFC2384) */
		*type = 2;
		p += 6;
		if ((s = (unsigned char*)strchr((char *)p, ';'))) {
			*s = '\0';
			if (EM_SAFE_STRLEN((char *)p)) user = (unsigned char*)EM_SAFE_STRDUP((char *)p);
			p = s + 1;
		}
		if ((s = (unsigned char*)strchr((char *)p, '@'))) {
			*s = '\0';
			if (user || *(p - 3) == '/') {
				if (strncmp((char *)p, "AUTH=", 5) == 0) auth = (unsigned char*)EM_SAFE_STRDUP((char *)p + 5);
			}
			else
				user = (unsigned char*)EM_SAFE_STRDUP((char *)p);
			p = s + 1;
		}
		if ((s = (unsigned char*)strchr((char *)p, ':'))) {
			*s = '\0';
			s++;
			EM_DEBUG_LOG("PORT:%s", s);
		}
		host = (unsigned char*)EM_SAFE_STRDUP((char *)p);
		mailbox = NULL;
		break;

	case 'i':/*	 imap (RFC2192) */
		*type = 3;
		p += 7;
		if ((s = (unsigned char*)strchr((char *)p, ';'))) {
			*s = '\0';
			if (EM_SAFE_STRLEN((char *)p)) user = (unsigned char*)EM_SAFE_STRDUP((char *)p);
			p = s + 1;
		}
		if ((s = (unsigned char*)strchr((char *)p, '@'))) {
			*s = '\0';
			if (user || *(p - 3) == '/') {
				if (strncmp((char *)p, "AUTH=", 5) == 0) auth = (unsigned char*)EM_SAFE_STRDUP((char *)p + 5);
			}
			else
				user = (unsigned char*)EM_SAFE_STRDUP((char *)p);
			p = s + 1;
		}

		if ((s = (unsigned char *)strchr((char *)p, '/'))) { /*prevent 47327*/
			*s = '\0';
			host = (unsigned char *)EM_SAFE_STRDUP((char *)p);
			p = s + 1;
		}

		if ((s = (unsigned char*)strchr((char *)p, '?')))
			*s = '\0';
		else if ((s = (unsigned char*)strchr((char *)p, ';')))
			*s = '\0';
		else
			s = p + EM_SAFE_STRLEN((char *)p);
		if (*(s - 1) == '/')
			*(s - 1) = '\0';

		if (EM_SAFE_STRLEN((char *)p))
			mailbox =(unsigned char*) EM_SAFE_STRDUP((char *)p);
		break;

	case 'h': /*  not supported */
		EM_DEBUG_LOG("Http URI is not yet supported");
		err = EMAIL_ERROR_XML_PARSER_FAILURE;
		break;

	default:
		err = EMAIL_ERROR_XML_PARSER_FAILURE;
		break;
	}

	if (err == EMAIL_ERROR_NONE) {
		*incoming_server_user_name = user;
		*host_addr = host;
		*mbox_name = mailbox;
		*auth_type = auth;
	}

 FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

static int _get_time_from_element(unsigned char* elm, unsigned char** time_stamp)
{
	EM_DEBUG_FUNC_BEGIN("elm[%p] time_stamp[%p]", elm, time_stamp);
	int err = EMAIL_ERROR_NONE;
	unsigned char* p, *s, *stamp = NULL;

	if (!(p = (unsigned char*)strstr((char *)elm, "timestamp"))) {
		EM_DEBUG_EXCEPTION("invalid notification data");
		err = EMAIL_ERROR_XML_PARSER_FAILURE;
		goto FINISH_OFF;
	}

	p += 11;
	s = (unsigned char*)strchr((char *)p, '\"');

	if (s)
		*s = '\0';

	stamp = malloc(15);
	if (!stamp) {
		EM_DEBUG_EXCEPTION("malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	memcpy(stamp, p, 4);
	memcpy(stamp + 4, p + 5, 2);
	memcpy(stamp + 6, p + 8, 2);
	memcpy(stamp + 8, p + 11, 2);
	memcpy(stamp + 10, p + 14, 2);
	memcpy(stamp + 12, p + 17, 2);
	stamp[14] = '\0';

	*time_stamp = stamp;

 FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return 1;
}

/*	<emn mailbox="mailat:user@wapforum.org" timestamp="2002-04-16T06:40:00Z"/> */
static int _get_data_from_element(unsigned char* elm, int* type, unsigned char** incoming_server_user_name, unsigned char** host_addr, unsigned char** mbox_name, unsigned char** auth_type, unsigned char** time_stamp)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	if ((err = _get_time_from_element(elm, time_stamp)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_XML_PARSER_FAILURE");
		goto FINISH_OFF;
	}

	/*	must call get_addr_from_element after calling _get_time_from_element */
	if (!_get_addr_from_element(elm, type, incoming_server_user_name, host_addr, mbox_name, auth_type)) {
		EM_SAFE_FREE(*time_stamp);
		err = EMAIL_ERROR_XML_PARSER_FAILURE;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_XML_PARSER_FAILURE");
		goto FINISH_OFF;
	}

 FINISH_OFF:
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

/* description
 *	  get account from OMA EMN data
 * arguments
 *	  wbxml		: wbxml data
 *	  account_id: id of account to be retrieved from EMN data
 *	  mailbox	: if mail-box information exists in EMN data, mailbox holds the name of mailbox.
 *	  err_code	: hold error code
 * return
 *	  succeed : 1
 *	  fail : 0
 */
static int _get_emn_account(unsigned char *input_wbxml, int input_wbxml_length, email_account_t *account, char **mailbox)
{
	EM_DEBUG_FUNC_BEGIN("input_wbxml[%p] input_wbxml_length[%d] account[%p] mailbox[%p]", input_wbxml, input_wbxml_length, account, mailbox);
	WBXMLContentHandler parse_handler = {
		(WBXMLStartDocumentHandler) _cb_parser_start_document,
		(WBXMLEndDocumentHandler) _cb_parser_end_document,
		(WBXMLStartElementHandler) _cb_parser_start_element,
		(WBXMLEndElementHandler) _cb_parser_end_element,
		(WBXMLCharactersHandler) _cb_parser_characters,
		NULL
	};

	WBXMLParser*	wbxml_parser = NULL;
	WB_UTINY*		wbxml = NULL;
	WB_UTINY*		elm = NULL;
	WB_ULONG		wbxml_len = 0;
	WB_ULONG		err_idx = 0;
	WBXMLError		ret = WBXML_OK;
	email_account_t*	accounts = NULL;
	unsigned char*	incoming_server_user_name = NULL;
	unsigned char*	host_addr = NULL;
	unsigned char*	mbox_name = NULL;
	unsigned char*	auth_type = NULL;
	unsigned char*	time_stamp = NULL;
	int				type = 0;
	int				i = 0;
	int				count = 0;
	int				retr = false;
	int				err = EMAIL_ERROR_NONE;

	if (!input_wbxml || !account) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("wbxml_b64 [%s]", input_wbxml);

	/* wbxml = g_base64_decode((const char*)wbxml_b64, (unsigned int*)&wbxml_len); */

	wbxml = input_wbxml;
	wbxml_len = input_wbxml_length;

	EM_DEBUG_LOG("wbxml [%s]", wbxml);
	EM_DEBUG_LOG("wbxml_len [%d]", wbxml_len);

	/*	create wbxml parser */
	if (!(wbxml_parser = wbxml_parser_create())) {
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	/*	initialize wbxml parser */
	wbxml_parser_set_user_data(wbxml_parser, (void*)&elm);
	wbxml_parser_set_content_handler(wbxml_parser, &parse_handler);

	/*	parsing wbxml */
	if ((ret = wbxml_parser_parse(wbxml_parser, wbxml, wbxml_len)) != WBXML_OK) {
		err_idx = wbxml_parser_get_current_byte_index(wbxml_parser);
		EM_DEBUG_EXCEPTION("Parsing failed at %u - Token %x - %s", err_idx, wbxml[err_idx], wbxml_errors_string(ret));
		err = EMAIL_ERROR_XML_PARSER_FAILURE;
		goto FINISH_OFF;
	}
	else {
		EM_DEBUG_LOG("Parsing OK !");
	}

	/*	destroy parser */
	wbxml_parser_destroy(wbxml_parser);
	wbxml_parser = NULL;

	/*	free buffer */
	/*
	  wbxml_free(wbxml);
	  wbxml = NULL;
	*/

	if (!elm) {
		EM_DEBUG_EXCEPTION("invalid elements");
		err = EMAIL_ERROR_XML_PARSER_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("elements = [%s]", elm);
	_get_data_from_element(elm, &type, &incoming_server_user_name, &host_addr, &mbox_name, &auth_type, &time_stamp);

	EM_SAFE_FREE(elm);

	EM_DEBUG_LOG("user_type = [%d]", type);
	EM_DEBUG_LOG("incoming_server_user_name = [%s]", (char *)incoming_server_user_name ? (char*)incoming_server_user_name : "NIL");
	EM_DEBUG_LOG("host_addr = [%s]", (char *)host_addr ? (char*)host_addr : "NIL");
	EM_DEBUG_LOG("mbox_name = [%s]", (char *)mbox_name ? (char*)mbox_name : "NIL");
	EM_DEBUG_LOG("auth_type = [%s]", (char *)auth_type ? (char*)auth_type : "NIL");
	EM_DEBUG_LOG("time_stamp= [%s]", (char *)time_stamp? (char*)time_stamp: "NIL");

	if (!emdaemon_get_account_list(&accounts, &count, &err)) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_DB_FAILURE");
		err = EMAIL_ERROR_DB_FAILURE;
		goto FINISH_OFF;
	}

	for (i = 0; i < count; i++) {
		char* temp_account_name = NULL;
		char *s = NULL;
		/*	EM_DEBUG_LOG(">>>> Account Information UserName [ %s ] Email Addr [ %s], Account ID [ %d] >>> ",accounts[i].incoming_server_user_name,accounts[i].user_email_address, accounts[i].account_id); */
		temp_account_name =(char*) EM_SAFE_STRDUP((char *)accounts[i].incoming_server_user_name);

		if ((s = (char*)strchr((char *)temp_account_name, '@')))  {
			*s = '\0';
			EM_SAFE_FREE(accounts[i].incoming_server_user_name);
			accounts[i].incoming_server_user_name  = (char*)EM_SAFE_STRDUP((char *)temp_account_name);
		}
		EM_SAFE_FREE(temp_account_name);
		if (incoming_server_user_name)	{
			if (strcmp(accounts[i].incoming_server_user_name, (char *)incoming_server_user_name) == 0 &&
				strstr(accounts[i].user_email_address, (char *)host_addr)) {
				EM_DEBUG_LOG(">>>> Account Match >>> ");
				if ((type == 1) ||
					(type == 2 && accounts[i].incoming_server_type == EMAIL_SERVER_TYPE_POP3) ||
					(type == 3 && accounts[i].incoming_server_type == EMAIL_SERVER_TYPE_IMAP4)) {
					/* accounts[i].flag2 = type; */
					EM_DEBUG_LOG("found target account id[%d] name[%s]", accounts[i].account_id, accounts[i].incoming_server_user_name);
					break;
				}
			}
		}
	}

	if (i >= count) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_ACCOUNT_NOT_FOUND");
		err = EMAIL_ERROR_ACCOUNT_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (account) {
		account->account_id = accounts[i].account_id;
		/* account->flag2 = accounts[i].flag2; */
	}

	if (mailbox) {
		*mailbox = mbox_name ? (char *)mbox_name : NULL;
		mbox_name = NULL;
	}
	emdaemon_free_account(&accounts, count, NULL);
	accounts = NULL;

	retr = true;

 FINISH_OFF:
	/*
	  if (wbxml)
	  wbxml_free(wbxml);
	*/

	if (wbxml_parser)
		wbxml_parser_destroy(wbxml_parser);

	EM_SAFE_FREE(elm);

	if (accounts)
		emdaemon_free_account(&accounts, count, NULL);

	EM_SAFE_FREE(incoming_server_user_name);
	EM_SAFE_FREE(mbox_name);
	EM_SAFE_FREE(auth_type);
	EM_SAFE_FREE(time_stamp);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


static int emdaemon_handle_emn_notification(unsigned char* wbxml_b64, int input_body_lenth)
{
	EM_DEBUG_FUNC_BEGIN("wbxml_b64[%p] input_body_lenth[%d]", wbxml_b64, input_body_lenth);

	int err = EMAIL_ERROR_NONE;;
	char* mailbox_name = NULL;
	email_account_t account = { 0 };
	emstorage_mailbox_tbl_t *mailbox_tbl = NULL;

	if (!wbxml_b64) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if ((err = _get_emn_account(wbxml_b64, input_body_lenth, &account, &mailbox_name)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("_get_emn_account failed [%d]", err);
		goto FINISH_OFF;
	}

	if (account.incoming_server_type == EMAIL_SERVER_TYPE_IMAP4 && mailbox_name)  {
		if (!emstorage_get_mailbox_by_name(account.account_id, -1, mailbox_name, &mailbox_tbl, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_name failed [%d", err);
			goto FINISH_OFF;
		}
	}
	else {
		if (!emstorage_get_mailbox_by_mailbox_type(account.account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox_tbl, false, &err))  {
			EM_DEBUG_EXCEPTION("emstorage_get_mailbox_by_mailbox_type failed [%d", err);
			goto FINISH_OFF;
		}
	}

	if (!emdaemon_sync_header(mailbox_tbl->account_id, mailbox_tbl->mailbox_id, NULL, &err))  {
		EM_DEBUG_EXCEPTION("emdaemon_sync_header failed [%d]", err);
		goto FINISH_OFF;
	}


 FINISH_OFF:

	if (mailbox_tbl)
		emstorage_free_mailbox(&mailbox_tbl, 1, NULL);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

void oma_emn_push_cb(msg_handle_t input_handle, const char *input_push_header, const char *input_push_body, int input_push_body_lenth, void *input_user_param)
{
	EM_DEBUG_FUNC_BEGIN("input_handle[%d] input_push_header[%p] input_push_body[%p] input_push_body_lenth[%d] input_user_param[%p]", input_handle, input_push_header, input_push_body, input_push_body_lenth, input_user_param);
	int err = EMAIL_ERROR_NONE;

	EM_DEBUG_EXCEPTION("input_push_header [%s]", input_push_header);
	EM_DEBUG_EXCEPTION("input_push_body [%s]", input_push_body);
	EM_DEBUG_EXCEPTION("input_push_body_lenth [%d]", input_push_body_lenth);

	if((err = emdaemon_handle_emn_notification((unsigned char*)input_push_body, input_push_body_lenth)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emdaemon_handle_emn_notification failed [%d]", err);
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
}

static int emdaemon_register_wap_push_callback(msg_handle_t *input_msg_handle, char *input_app_id)
{
	EM_DEBUG_FUNC_BEGIN("input_msg_handle [%p] input_app_id [%p]", input_msg_handle, input_app_id);
	int err = EMAIL_ERROR_NONE;
	msg_error_t msg_err = MSG_SUCCESS;
	msg_struct_t msg_struct = NULL;
	char *content_type = "application/vnd.wap.emn+wbxml";
	char *pkg_name = "com.samsung.email";
	bool bLaunch = false;

	if(input_msg_handle == NULL || input_app_id == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	msg_struct = msg_create_struct(MSG_STRUCT_PUSH_CONFIG_INFO);

	if(msg_struct == NULL) {
		EM_DEBUG_EXCEPTION("msg_create_struct() failed [%d]", msg_err);
		err = EMAIL_ERROR_INPROPER_RESPONSE_FROM_MSG_SERVICE;
		goto FINISH_OFF;
	}

	msg_set_str_value(msg_struct, MSG_PUSH_CONFIG_CONTENT_TYPE_STR, content_type, MAX_WAPPUSH_CONTENT_TYPE_LEN);
	msg_set_str_value(msg_struct, MSG_PUSH_CONFIG_APPLICATON_ID_STR, input_app_id, MAX_WAPPUSH_ID_LEN);
	msg_set_str_value(msg_struct, MSG_PUSH_CONFIG_PACKAGE_NAME_STR, pkg_name, MSG_FILEPATH_LEN_MAX);
	msg_set_bool_value(msg_struct, MSG_PUSH_CONFIG_LAUNCH_BOOL, bLaunch);

	msg_add_push_event(*input_msg_handle, msg_struct);

	if ((msg_err = msg_reg_push_message_callback(*input_msg_handle, &oma_emn_push_cb, input_app_id, NULL)) != MSG_SUCCESS) {
		EM_DEBUG_EXCEPTION("msg_reg_push_message_callback() for %s failed [%d]", msg_err, input_app_id);
		err = EMAIL_ERROR_INPROPER_RESPONSE_FROM_MSG_SERVICE;
		goto FINISH_OFF;
	}

 FINISH_OFF:

	if(msg_struct)
		msg_release_struct(&msg_struct);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


int emdaemon_initialize_emn(void)
{
	EM_DEBUG_FUNC_BEGIN();

	msg_error_t msg_err = MSG_SUCCESS;
	msg_handle_t msg_handle = NULL;
	int err = EMAIL_ERROR_NONE;
	char *oma_docomo_app_id = "x-oma-docomo:xmd.mail.ua";
	char *oma_emn_app_id = "x-wap-application:emn.ua";

	if ((msg_err = msg_open_msg_handle(&msg_handle)) != MSG_SUCCESS) {
		EM_DEBUG_EXCEPTION("msg_open_msg_handle() failed [%d]", msg_err);
		err = EMAIL_ERROR_INPROPER_RESPONSE_FROM_MSG_SERVICE;
		goto FINISH_OFF;
	}

	if ((err = emdaemon_register_wap_push_callback(&msg_handle, oma_docomo_app_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emdaemon_register_wap_push_callback() failed [%d]", err);
		goto FINISH_OFF;
	}

	if ((err = emdaemon_register_wap_push_callback(&msg_handle, oma_emn_app_id)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emdaemon_register_wap_push_callback() failed [%d]", err);
		goto FINISH_OFF;
	}


 FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

int emdaemon_finalize_emn(gboolean bExtDest)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_FUNC_END();
	return 0;
}

#endif /*  __FEATURE_OMA_EMN__ */

/* EOF */
