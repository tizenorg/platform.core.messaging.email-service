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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>	/*  Needed for the definition of va_list */
#include <glib.h>

#include "emflib.h"

/* #include "emf-emn-storage.h" */
#include "emf-emn.h"

/* --------------------------------------------------------------------------------*/
/* ----- WBXML Parsing (2007.06.09) --------------------------------------------------*/
/* --------------------------------------------------------------------------------*/
#ifdef USE_OMA_EMN

#include <wbxml.h>
#include <wbxml_errors.h>
#include "emf-dbglog.h"
#include <vconf.h>
#include "emf-global.h"

typedef struct
{
	int account_id;
	emf_emn_noti_cb callback;
	void* user_data;
} 
emf_emn_noti_pack_t;


char*
__emn_get_username(char *account_username);
static void 
_cb_parser_start_document(void* ctx, WB_LONG charset, const WBXMLLangEntry* lang)
{
    EM_DEBUG_LOG("_cb_parser_start_document\n");
    EM_DEBUG_LOG("Parsing Document:\n\tRoot Element: %s\n\tPublic ID: %s\n\tDTD: %s\n",
                 lang->publicID->xmlRootElt,
                 lang->publicID->xmlPublicID,
                 lang->publicID->xmlDTD);
}

static void 
_cb_parser_end_document(void* ctx)
{
    EM_DEBUG_LOG("_cb_parser_end_document\n");
}

static void 
_cb_parser_start_element(void* ctx, WBXMLTag* element, WBXMLAttribute** atts, WB_BOOL empty)
{
    WB_UTINY* p, **pp = ctx;
    WB_ULONG j = 0, len = 0;
    EM_DEBUG_LOG("parse_clb_start_element\n");

    if (strcasecmp((char *)wbxml_tag_get_xml_name(element), "emn") != 0)
    {
        EM_DEBUG_LOG("not email notification\n");
        return ;
    }

    if (atts == NULL)
    {
        EM_DEBUG_LOG("no emn attributes\n");
        return ;
    }

    len = strlen((char *)wbxml_tag_get_xml_name(element)) + 1;
    while (atts[j] != NULL)
    {
        len += (strlen((char *)wbxml_attribute_get_xml_name(atts[j])) + strlen((char *)wbxml_attribute_get_xml_value(atts[j])) + 7);
        j++;
    }
    len += 3;

    if (!(p = malloc(sizeof(WB_UTINY) * len + 1)))
    {
        return ;
    }

    EM_DEBUG_LOG("<%s", (char *)wbxml_tag_get_xml_name(element));
    sprintf((char *)p, "<%s", (char *)wbxml_tag_get_xml_name(element));

    j = 0;
    while (atts[j] != NULL)
    {
        EM_DEBUG_LOG(" %s=\"%s\"", (char *)wbxml_attribute_get_xml_name(atts[j]), (char *)wbxml_attribute_get_xml_value(atts[j]));
        strcat((char *)p, " ");
        strcat((char *)p, (char *)wbxml_attribute_get_xml_name(atts[j]));
        strcat((char *)p, "=");
        strcat((char *)p, "\"");
        strcat((char *)p, (char *)wbxml_attribute_get_xml_value(atts[j]));
        strcat((char *)p, "\"");
        j++;
    }

    if (empty)
    {
        EM_DEBUG_LOG("/>\n");
        strcat((char *)p, "/>");
    }
    else
    {
        EM_DEBUG_LOG(">\n");
        strcat((char *)p, ">");
    }

    *pp = p;
}

static void 
_cb_parser_end_element(void* ctx, WBXMLTag* element, WB_BOOL empty)
{
    EM_DEBUG_LOG("parse_clb_end_element\n");
}

static void 
_cb_parser_characters(void* ctx, WB_UTINY* ch, WB_ULONG start, WB_ULONG length)
{
    EM_DEBUG_LOG("_cb_parser_characters\n");
}

static int _get_addr_from_element(unsigned char* elm,
						int* type,
						unsigned char** user_name,
						unsigned char** host_addr,
						unsigned char** mbox_name,
						unsigned char** auth_type)
{
    unsigned char*	p;
    unsigned char*	s;
    unsigned char*	user = NULL;
    unsigned char*	host = NULL;
    unsigned char*	mbox = NULL;
    unsigned char*	auth = NULL;
    
     EM_DEBUG_FUNC_BEGIN();
	 
    if (!(p = (unsigned char*)strstr((char *)elm, "mailbox")))	/*  add acetrack.20080331.K8.4043 */
    {
        EM_DEBUG_EXCEPTION("invalid notification data\n");
        return 0;
    }

    p += 9;
    s = (unsigned char*)strchr((char *)p, '\"');
    /*  2007-05-08 by acetrack */
    if (s)
      *s = '\0';
	
    switch (p[0])
    {
    case 'm':/*  mailat (RFC2234) */
        *type = 1;
        p += 7;
        if (!(s = (unsigned char*)strchr((char *)p, '@'))) return 0;
        *s = '\0';
        user = (unsigned char*)EM_SAFE_STRDUP((char *)p);
        s++;
        host = (unsigned char*)EM_SAFE_STRDUP((char *)s);
	  mbox = NULL;
        auth = NULL;
        break;

    case 'p':/*  pop3 (RFC2384) */
        *type = 2;
        p += 6;
        if ((s = (unsigned char*)strchr((char *)p, ';')))
        {
            *s = '\0';
            if (strlen((char *)p)) user = (unsigned char*)EM_SAFE_STRDUP((char *)p);
            p = s + 1;
        }
        if ((s = (unsigned char*)strchr((char *)p, '@')))
        {
            *s = '\0';
            if (user || *(p - 3) == '/')
            {
                if (strncmp((char *)p, "AUTH=", 5) == 0) auth = (unsigned char*)EM_SAFE_STRDUP((char *)p + 5);
            }
            else
                user = (unsigned char*)EM_SAFE_STRDUP((char *)p);
            p = s + 1;
        }
        if ((s = (unsigned char*)strchr((char *)p, ':')))
        {
            *s = '\0';
            s++;
            EM_DEBUG_LOG("PORT:%s\n", s);
        }
        host = (unsigned char*)EM_SAFE_STRDUP((char *)p);
        mbox = NULL;
        break;

    case 'i':/*  imap (RFC2192) */
        *type = 3;
        p += 7;
        if ((s = (unsigned char*)strchr((char *)p, ';')))
        {
            *s = '\0';
            if (strlen((char *)p)) user = (unsigned char*)EM_SAFE_STRDUP((char *)p);
            p = s + 1;
        }
        if ((s = (unsigned char*)strchr((char *)p, '@')))
        {
            *s = '\0';
            if (user || *(p - 3) == '/')
            {
                if (strncmp((char *)p, "AUTH=", 5) == 0) auth = (unsigned char*)EM_SAFE_STRDUP((char *)p + 5);
            }
            else
                user = (unsigned char*)EM_SAFE_STRDUP((char *)p);
            p = s + 1;
        }

        if ((s = (unsigned char *)strchr((char *)p, '/'))) * s = '\0';
        host = (unsigned char *)EM_SAFE_STRDUP((char *)p);
        p = s + 1;

        if ((s = (unsigned char*)strchr((char *)p, '?'))) * s = '\0';
        else if ((s = (unsigned char*)strchr((char *)p, ';'))) * s = '\0';
        else s = p + strlen((char *)p);
        if (*(s - 1) == '/') *(s - 1) = '\0';

        if (strlen((char *)p)) mbox =(unsigned char*) EM_SAFE_STRDUP((char *)p);
        break;

    case 'h': /*  not supported */
        EM_DEBUG_LOG("Http URI is not yet supported\n");
        return 0;
        
    default:
        return 0;
    }

    *user_name = user;
    *host_addr = host;
    *mbox_name = mbox;
    *auth_type = auth;

    return 1;
}

static int	
_get_time_from_element(unsigned char* elm,
                          unsigned char** time_stamp)
{
    EM_DEBUG_FUNC_BEGIN();

    unsigned char* p, *s, *stamp = NULL;
    if (!(p = (unsigned char*)strstr((char *)elm, "timestamp")))
    {
        EM_DEBUG_EXCEPTION("invalid notification data\n");
        return 0;
    }

    p += 11;
    s = (unsigned char*)strchr((char *)p, '\"');
    /*  2007-05-08 by acetrack */
    if (s)
        *s = '\0';

    stamp = malloc(15);
    if (!stamp)	/*  added acetrack.20080331.K8.4045 */
    {
    	EM_DEBUG_EXCEPTION("malloc failed");
    	return 0;
    }
    memcpy(stamp, p, 4);
    memcpy(stamp + 4, p + 5, 2);
    memcpy(stamp + 6, p + 8, 2);
    memcpy(stamp + 8, p + 11, 2);
    memcpy(stamp + 10, p + 14, 2);
    memcpy(stamp + 12, p + 17, 2);
    stamp[14] = '\0';

    *time_stamp = stamp;

    return 1;
}

/*  <emn mailbox="mailat:user@wapforum.org" timestamp="2002-04-16T06:40:00Z"/> */
static int _get_data_from_element(unsigned char* elm,
                          int* type,
                          unsigned char** user_name,
                          unsigned char** host_addr,
                          unsigned char** mbox_name,
                          unsigned char** auth_type,
                          unsigned char** time_stamp)
{

    EM_DEBUG_FUNC_BEGIN();
    if (!_get_time_from_element(elm, time_stamp))
    {
    	return 0;
	}

    /*  must call get_addr_from_element after calling _get_time_from_element */
    if (!_get_addr_from_element(elm, type, user_name, host_addr, mbox_name, auth_type))
    {
	    EM_SAFE_FREE*time_stamp)	/*  added acetrack.20080331.K8.4046 */
   	    return 0;
    }

    return 1;
}

/* Parse the Email address to Get the user Name for the account [deepam.p@samsung.com] */
char*
__emn_get_username(char *account_username)
{

	EM_IF_NULL_RETURN_VALUE(account_username, NULL);

	int index = 0;
	char **token_list = NULL;
	char *username = NULL, *name = NULL;

	username = g_strdup(account_username);
	token_list = g_strsplit_set(username, "@", -1);

	if (username) {
		g_free(username);
	}

	if (token_list[index] != NULL)
    {
		name = g_strdup(token_list[index]);
		g_strfreev(token_list); /*  MUST BE. */
		EM_DEBUG_LOG("ACCOUNT USER NAME [%s] \n", name);
		return name;
	}
	else
		return NULL;
	
}

/* description
 *    get account from OMA EMN data
 * arguments
 *    wbxml_b64 : BASE64 encoded data
 *    account_id: id of account to be retrieved from EMN data
 *    mailbox   : if mail-box information exists in EMN data, mailbox holds the name of mailbox.
 *    err_code  : hold error code
 * return
 *    succeed : 1
 *    fail : 0
 */
static int
_get_emn_account(unsigned char* wbxml_b64, emf_account_t* account, char** mailbox, int* err_code)
{
	EM_DEBUG_LOG("_get_emn_account Enter");
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
    emf_account_t*	accounts = NULL;
    unsigned char*	user_name = NULL;
    unsigned char*	host_addr = NULL;
    unsigned char*	mbox_name = NULL;
    unsigned char*	auth_type = NULL;
    unsigned char*	time_stamp = NULL;
    int				type = 0;
    int				i = 0;
    int				count = 0;
    int				retr = false;
    int				err = EMF_ERROR_NONE;

    EM_DEBUG_LOG("");

    if (!wbxml_b64 || !account)
    {
    	 EM_DEBUG_EXCEPTION(">>>> Invalid Parameter >>>> \n");
        err = EMF_ERROR_INVALID_PARAM;
        goto FINISH_OFF;
    }

    wbxml = g_base64_decode((const char*)wbxml_b64, (unsigned int*)&wbxml_len);
    EM_DEBUG_LOG("wbxml = %p\n", wbxml);
    EM_DEBUG_LOG("wbxml_len = %d\n", wbxml_len);

    /*  create wbxml parser */
    if (!(wbxml_parser = wbxml_parser_create()))
    {
        err = EMF_ERROR_OUT_OF_MEMORY;		
        goto FINISH_OFF;
    }

    /*  initialize wbxml parser */
    wbxml_parser_set_user_data(wbxml_parser, (void*)&elm);
    wbxml_parser_set_content_handler(wbxml_parser, &parse_handler);

    /*  parsing wbxml */
    if ((ret = wbxml_parser_parse(wbxml_parser, wbxml, wbxml_len)) != WBXML_OK)
    {
        err_idx = wbxml_parser_get_current_byte_index(wbxml_parser);
        EM_DEBUG_LOG("Parsing failed at %u - Token %x - %s", err_idx, wbxml[err_idx], wbxml_errors_string(ret));
		
        err = EMF_ERROR_XML_PARSER_FAILURE;		
        goto FINISH_OFF;
    }
    else
    {
        EM_DEBUG_LOG("Parsing OK !\n");
    }

    /*  destroy parser */
    wbxml_parser_destroy(wbxml_parser);
    wbxml_parser = NULL;

    /*  free buffer */
    wbxml_free(wbxml);
    wbxml = NULL;

    if (!elm)
    {
        EM_DEBUG_EXCEPTION("invalid elements\n");
		
        err = EMF_ERROR_XML_PARSER_FAILURE;		
        goto FINISH_OFF;
    }

    EM_DEBUG_LOG("elements = [%s]\n", elm);
    _get_data_from_element(elm, &type, &user_name, &host_addr, &mbox_name, &auth_type, &time_stamp);

    EM_SAFE_FREE(elm);

    EM_DEBUG_LOG("user_type = [%d]\n", type);
    EM_DEBUG_LOG("user_name = [%s]\n", (char *)user_name ? (char*)user_name : "NIL");
    EM_DEBUG_LOG("host_addr = [%s]\n", (char *)host_addr ? (char*)host_addr : "NIL");
    EM_DEBUG_LOG("mbox_name = [%s]\n", (char *)mbox_name ? (char*)mbox_name : "NIL");
    EM_DEBUG_LOG("auth_type = [%s]\n", (char *)auth_type ? (char*)auth_type : "NIL");
    EM_DEBUG_LOG("time_stamp= [%s]\n", (char *)time_stamp? (char*)time_stamp: "NIL");

     if (!emf_account_get_list(&accounts, &count, &err))
    {
        EM_DEBUG_EXCEPTION("   emf_account_get_list error");
        err = EMF_ERROR_DB_FAILURE;
        goto FINISH_OFF;
    }
	
    for (i = 0; i < count; i++)
    {
        /* sowmya.kr, 201009, Fix for EMN */
        char* temp_account_name = NULL;
		char *s = NULL;
	    /* 	EM_DEBUG_LOG(">>>> Account Information UserName [ %s ] Email Addr [ %s], Account ID [ %d] >>> \n",accounts[i].user_name,accounts[i].email_addr, accounts[i].account_id); */
	        temp_account_name =(char*) EM_SAFE_STRDUP((char *)accounts[i].user_name);

		if ((s = (char*)strchr((char *)temp_account_name, '@')))  {
			*s = '\0';
			EM_SAFE_FREE(accounts[i].user_name);			
			accounts[i].user_name  = (char*)EM_SAFE_STRDUP((char *)temp_account_name);			
		}
		EM_SAFE_FREE(temp_account_name);
		if (user_name)  {	
			if (strcmp(accounts[i].user_name, (char *)user_name) == 0 &&
				strstr(accounts[i].email_addr, (char *)host_addr)) {
		        	EM_DEBUG_LOG(">>>> Account Match >>> \n");
				if ((type == 1) ||
					(type == 2 && accounts[i].receiving_server_type == EMF_SERVER_TYPE_POP3) ||
					(type == 3 && accounts[i].receiving_server_type == EMF_SERVER_TYPE_IMAP4)) {
					accounts[i].flag2 = type;
					EM_DEBUG_LOG("found target account id[%d] name[%s]", accounts[i].account_id, accounts[i].user_name);
					break;
				}
			}
		}
	}

    	if (i >= count) {
	        EM_DEBUG_EXCEPTION("no account was found");
        	err = EMF_ERROR_ACCOUNT_NOT_FOUND;
	        goto FINISH_OFF;
	}
	if (account) {
		account->account_bind_type = accounts[i].account_bind_type;
		account->account_id = accounts[i].account_id;
		account->flag2 = accounts[i].flag2;
	}
    
    if (mailbox)
    {
        *mailbox = mbox_name ? (char *)mbox_name : NULL;
        mbox_name = NULL;
    }
    emf_account_free(&accounts, count, NULL);
    accounts = NULL;

    retr = true;

FINISH_OFF:
    if (wbxml) wbxml_free(wbxml);
    if (wbxml_parser) wbxml_parser_destroy(wbxml_parser);
    EM_SAFE_FREE(elm);
    if (accounts) emf_account_free(&accounts, count, NULL);
    EM_SAFE_FREE(user_name);
    EM_SAFE_FREE(mbox_name);
    EM_SAFE_FREE(auth_type);
    EM_SAFE_FREE(time_stamp);
    if (err_code) *err_code = err;
    return retr;
}

EXPORT_API int emf_emn_handler(unsigned char* wbxml_b64, emf_emn_noti_cb callback, int* err_code)
{
	EM_DEBUG_FUNC_BEGIN("wbxml_b64[%p], callback[%p], err_code[%p]", wbxml_b64, callback, err_code);
	
	int ret = false;
	int err = EMF_ERROR_NONE;;
	char* mailbox_name = NULL;
	emf_mailbox_t mailbox = { 0 };
	emf_account_t account = { 0 };
	emf_emn_noti_pack_t* pack = NULL;
	char* pmailbox = NULL;
	
	if (!wbxml_b64) {
		EM_DEBUG_EXCEPTION("EMF_ERROR_INVALID_PARAM");
		err = EMF_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	pack = (emf_emn_noti_pack_t*)em_core_malloc(sizeof(emf_emn_noti_pack_t));
	if (!pack) {
		EM_DEBUG_EXCEPTION("em_core_malloc failed");
		err = EMF_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	
	if (!_get_emn_account(wbxml_b64, &account, &mailbox_name, &err)) {
		EM_DEBUG_EXCEPTION("_get_emn_account failed [%d]", err);
		goto FINISH_OFF;
	}

	mailbox.account_id = account.account_id;

	if (!em_storage_get_mailboxname_by_mailbox_type(mailbox.account_id,EMF_MAILBOX_TYPE_INBOX,&pmailbox, false, &err))  {
		EM_DEBUG_EXCEPTION("em_storage_get_mailboxname_by_mailbox_type failed [%d", err);		
		err = em_storage_get_emf_error_from_em_storage_error(err);
		goto FINISH_OFF;
	}

	if ((account.receiving_server_type == EMF_SERVER_TYPE_IMAP4) && (account.flag2 == 3))  {

		if (!mailbox_name || strncmp(pmailbox, mailbox_name, strlen(pmailbox)) != 0)  {
			EM_DEBUG_EXCEPTION("invalid inbox name [%p]", mailbox_name);
			err = EMF_ERROR_INVALID_MAILBOX;
			goto FINISH_OFF;
		}
	}
	
	/* sync header with mail server */
	mailbox.name = mailbox_tbl->mailbox_name;
	
	if (!emf_mailbox_sync_header(&mailbox, NULL, &err))  {
		EM_DEBUG_EXCEPTION("emf_mailbox_sync_header falied [%d]", err);
		goto FINISH_OFF;
	}
	
	ret = true;
	
FINISH_OFF:
	if (ret == false) 
		EM_SAFE_FREE(pack);

	EM_SAFE_FREE(pmailbox);

	if (err_code != NULL)
		*err_code = err;
	
	return ret;
}


#endif /*  USE_OMA_EMN */

/* EOF */
