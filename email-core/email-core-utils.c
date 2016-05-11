/*
*  email-service
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


/******************************************************************************
 * File: email-core-utils.c
 * Desc: Mail Utils
 *
 * Auth:
 *
 * History:
 *	2006.08.16 : created
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dlfcn.h>
#include <libintl.h>
#include <locale.h>
#include <glib.h>
#include <glib-object.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <vconf.h>
#include <regex.h>
#include <pthread.h>
#include <notification.h>
#include <notification_type.h>
#include <notification_internal.h>
#include <notification_text_domain.h>
#include <badge.h>
#include <badge_internal.h>
#ifdef __FEATURE_USE_DRM_API__
#include <drm_client_types.h>
#include <drm_client.h>
#endif /* __FEATURE_USE_DRM_API__ */
#include <storage.h>
#include <bundle.h>
#include <curl/curl.h>
#include <contacts.h>
#include <contacts_internal.h>

#include "email-types.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-debug-log.h"
#include "email-core-mail.h"
#include "email-core-event.h"
#include "email-core-mailbox.h"
#include "email-core-account.h"
#include "email-core-mailbox-sync.h"
#include "email-core-mime.h"
#include "email-core-gmime.h"
#include "email-core-signal.h"
#include "email-daemon.h"
#include "email-utilities.h"
#include "email-convert.h"
#include "email-internal-types.h"
#include "email-device.h"
#include "email-core-container.h"

#include <gio/gio.h>
#include "email-dbus-activation.h"


#include <app_control.h>
#include <app_control_internal.h>

#define LED_TIMEOUT_SECS          12
#define G_DISPLAY_LENGTH          256

#define DIR_SEPERATOR_CH          '/'
#define EMAIL_CH_QUOT             '"'
#define EMAIL_CH_BRACKET_S        '<'
#define EMAIL_CH_BRACKET_E        '>'
#define EMAIL_CH_COMMA            ','
#define EMAIL_CH_SEMICOLON        ';'
#define EMAIL_CH_ROUND_BRACKET_S  '('
#define EMAIL_CH_ROUND_BRACKET_E  ')'
#define EMAIL_CH_SQUARE_BRACKET_S '['
#define EMAIL_CH_SQUARE_BRACKET_E ']'
#define EMAIL_CH_SPACE            ' '
#define EMAIL_NOTI_ICON_PATH                tzplatform_mkpath(TZ_SYS_RO_APP,  "org.tizen.quickpanel/shared/res/noti_icons/E-mail/noti_email.png")
#define EMAIL_NOTI_INDICATOR_ICON_PATH      tzplatform_mkpath(TZ_SYS_RO_APP,  "org.tizen.indicator/res/icons/Event/B03_Event_email.png")
#define EMAIL_NOTI_MAX_MAIL_ID   100
//#define TEST 			TZ_SYS_RO_APPS

typedef struct  _em_transaction_info_type_t {
	int mail_id;
	int	handle;
	struct _em_transaction_info_type_t *next;

} em_transaction_info_type_t;

em_transaction_info_type_t  *g_transaction_info_list;

__thread email_session_t g_session = {0};
static notification_h g_sending_noti_handle = NULL;
typedef struct emcore_account_list_t emcore_account_list_t;
struct emcore_account_list_t {
	email_account_t *account;
	emcore_account_list_t *next;
};

#include <gmime/gmime.h>

INTERNAL_FUNC char *emcore_convert_mutf7_to_utf8(char *mailbox_name)
{
	EM_DEBUG_FUNC_BEGIN("mailbox_name[%p]", mailbox_name);

	iconv_t cd;
	char *result_mailbox_name = NULL;
	char *mutf7_text = NULL;
	char *cursor = NULL;
	int  is_base64 = 0;

	if (mailbox_name == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return NULL;
	}

	EM_DEBUG_LOG_SEC("mailbox_name[%s]", mailbox_name);

	cursor = mutf7_text = EM_SAFE_STRDUP(mailbox_name);

	if (mutf7_text == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		return NULL;
	}

	for (; *cursor; ++cursor)
		switch (*cursor) {
			case '+':
				if (!is_base64) *cursor = '&';
				break;

			case '&':
				*cursor = '+';
				is_base64 = 1;
				break;

			case '-':
				is_base64 = 0;
				break;

			case ',':
				if (is_base64)
					*cursor = '/';
				break;
		}

	cd = g_mime_iconv_open("UTF-8", "UTF-7");

	if (cd)
		result_mailbox_name = g_mime_iconv_strdup(cd, mutf7_text);

	EM_DEBUG_LOG_SEC("result_mailbox_name[%s]", result_mailbox_name);

	if (result_mailbox_name) {
		cursor = result_mailbox_name;
		for (; *cursor; ++cursor) {
			switch (*cursor) {
			  case '&':
				*cursor = '+';
				break;

			  case '+':
				*cursor = '&';
				break;
			}
		}
	} else {
		result_mailbox_name = (char *)utf8_from_mutf7((unsigned char *)mailbox_name);
		EM_DEBUG_LOG_SEC("result_mailbox_name[%s]", result_mailbox_name);
		if (result_mailbox_name == NULL)
			result_mailbox_name = EM_SAFE_STRDUP(mutf7_text);
	}

	EM_DEBUG_LOG_SEC("result_mailbox_name[%s]", result_mailbox_name);

	if (cd)
		g_mime_iconv_close(cd);

	EM_SAFE_FREE(mutf7_text);

	EM_DEBUG_FUNC_END("result_mailbox_name[%p]", result_mailbox_name);
	return result_mailbox_name;
}

/*  in smtp case, path argument must be ENCODED_PATH_SMTP */
int emcore_get_long_encoded_path_with_account_info(char *multi_user_name, email_account_t *account, char *path, int delimiter, char **long_enc_path, int *err_code)
{
	EM_PROFILE_BEGIN(emCorelongEncodedpath);
	EM_DEBUG_FUNC_BEGIN_SEC("account[%p], path[%s], delimiter[%d], long_enc_path[%p], err_code[%p]", account, path, delimiter, long_enc_path, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	char *p = NULL;
	email_authentication_method_t authentication_method = 0;

	size_t long_enc_path_len = 0;

	if (path == NULL || (path && strncmp(path, ENCODED_PATH_SMTP, EM_SAFE_STRLEN(ENCODED_PATH_SMTP)) != 0)) {		/*  imap or pop3 */
		EM_DEBUG_LOG_SEC("account->incoming_server_address[%p]", account->incoming_server_address);
		EM_DEBUG_LOG_SEC("account->incoming_server_address[%s]", account->incoming_server_address);

		if (!account->incoming_server_address) {
			EM_DEBUG_EXCEPTION("account->incoming_server_address is null");
			error = EMAIL_ERROR_INVALID_ACCOUNT;
			goto FINISH_OFF;
		}

		long_enc_path_len = EM_SAFE_STRLEN(account->incoming_server_address) + EM_SAFE_STRLEN(path) + 256; /*prevent 34357*/

		*long_enc_path = em_malloc(long_enc_path_len);
		if (!*long_enc_path)  {
			EM_DEBUG_EXCEPTION("malloc failed...");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		p = *long_enc_path;

		/*  ex:"{mai.test.com:143/imap} or {mai.test.com:143/imap/tls}my-mailbox" */

        if (multi_user_name) {
            SNPRINTF(p, long_enc_path_len, "{%s:%d/%s/user=%d%s%s",
                account->incoming_server_address,
                account->incoming_server_port_number,
                account->incoming_server_type == EMAIL_SERVER_TYPE_POP3 ? "pop3" : "imap",
                account->account_id,
                TOKEN_FOR_MULTI_USER,
                multi_user_name);
        } else {
            SNPRINTF(p, long_enc_path_len, "{%s:%d/%s/user=%d",
                account->incoming_server_address,
                account->incoming_server_port_number,
                account->incoming_server_type == EMAIL_SERVER_TYPE_POP3 ? "pop3" : "imap",
                account->account_id);
        }

		if (account->incoming_server_secure_connection & 0x01)  {
			strncat(p, "/ssl", long_enc_path_len-(EM_SAFE_STRLEN(p)+1));
			/* strcat(p, "/tryssl"); */
		} else if (account->incoming_server_secure_connection & 0x02) {
			strncat(p, "/tls", long_enc_path_len-(EM_SAFE_STRLEN(p)+1));
		}

		authentication_method = account->incoming_server_authentication_method;

		if (account->incoming_server_requires_apop) {
			strncat(p, "/apop", long_enc_path_len-(EM_SAFE_STRLEN(p)+1));
		}
	} else  {		/*  smtp */
		long_enc_path_len = EM_SAFE_STRLEN(account->outgoing_server_address) + 64;

		*long_enc_path = em_malloc(EM_SAFE_STRLEN(account->outgoing_server_address) + 64);
		if (!*long_enc_path) {
			EM_DEBUG_EXCEPTION("\t malloc failed...\n");
			error = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		p = *long_enc_path;

		/*  ex:"mail.test.com:25/smtp" */

		SNPRINTF(p, long_enc_path_len, "%s:%d/%s",
			account->outgoing_server_address,
			account->outgoing_server_port_number,
			"smtp");

		if (account->outgoing_server_need_authentication > 0) {
            if (multi_user_name) {
			    SNPRINTF(p + EM_SAFE_STRLEN(p), long_enc_path_len-(EM_SAFE_STRLEN(p)), "/user=%d%s%s",
                                                                                        account->account_id,
                                                                                        TOKEN_FOR_MULTI_USER,
                                                                                        multi_user_name);
            } else {
			    SNPRINTF(p + EM_SAFE_STRLEN(p), long_enc_path_len-(EM_SAFE_STRLEN(p)), "/user=%d",
                                                                                        account->account_id);
            }

			if (account->outgoing_server_need_authentication == EMAIL_AUTHENTICATION_METHOD_XOAUTH2)  {
				strncat(p, "/xoauth2", long_enc_path_len-(EM_SAFE_STRLEN(p)+1));
			}
		}

		if (account->outgoing_server_secure_connection & 0x01) {
			strncat(p, "/ssl/force_tls_v1_0", long_enc_path_len-(EM_SAFE_STRLEN(p)+1));
			/* strcat(p, "/tryssl"); */
		} else if (account->outgoing_server_secure_connection & 0x02)
			strncat(p, "/tls/force_tls_v1_0", long_enc_path_len-(EM_SAFE_STRLEN(p)+1));
		else
			strncat(p, "/notls", long_enc_path_len-(EM_SAFE_STRLEN(p)+1));

		authentication_method = account->outgoing_server_need_authentication;
	}

#ifdef FEATURE_CORE_DEBUG
	strncat(p, "/debug", long_enc_path_len-(strlen("/debug")+1));
#endif

	/* Authentication method */
	switch (authentication_method) {
	case EMAIL_AUTHENTICATION_METHOD_DEFAULT:
		strncat(p, "/needauth", long_enc_path_len-(EM_SAFE_STRLEN(p)+1));
		break;
	case EMAIL_AUTHENTICATION_METHOD_XOAUTH2:
		strncat(p, "/xoauth2", long_enc_path_len-(EM_SAFE_STRLEN(p)+1));
		break;
	case EMAIL_AUTHENTICATION_METHOD_NO_AUTH:
	default:
		break;
	}

	if (path == NULL || (path && strncmp(path, ENCODED_PATH_SMTP, EM_SAFE_STRLEN(ENCODED_PATH_SMTP)) != 0)) {
		strncat(p, "}", long_enc_path_len-(EM_SAFE_STRLEN(p)+1));

		if (path != NULL) {
			strncat(p, path, long_enc_path_len-(EM_SAFE_STRLEN(p)+1));
		}
	}

	if (*long_enc_path)
	    EM_DEBUG_LOG_SEC("long_enc_path [%s]", *long_enc_path);

	ret = true;

FINISH_OFF:
	if (ret != true)
		EM_SAFE_FREE(p);

	if (err_code != NULL)
		*err_code = error;
	EM_PROFILE_END(emCorelongEncodedpath);
	return ret;
}

int emcore_get_long_encoded_path(char *multi_user_name, int account_id, char *path, int delimiter, char **long_enc_path, int *err_code)
{
	EM_PROFILE_BEGIN(emCorelongEncodedpath);
	EM_DEBUG_FUNC_BEGIN("account_id[%d], delimiter[%d], long_enc_path[%p], err_code[%p]", account_id, delimiter, long_enc_path, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	email_account_t *ref_account = NULL;

	ref_account = emcore_get_account_reference(multi_user_name, account_id, false);
	if (!ref_account)  {
		EM_DEBUG_EXCEPTION("emcore_get_account_reference failed [%d]", account_id);
		error = EMAIL_ERROR_INVALID_ACCOUNT;
		goto FINISH_OFF;
	}

	if (emcore_get_long_encoded_path_with_account_info(multi_user_name, ref_account, path, delimiter, long_enc_path, &error) == false) {
		EM_DEBUG_EXCEPTION("emcore_get_long_encoded_path_with_account_info failed [%d]", error);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	if (err_code != NULL)
		*err_code = error;
	EM_PROFILE_END(emCorelongEncodedpath);
	return ret;
}

int emcore_get_encoded_mailbox_name(char *name, char **enc_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("name[%s], enc_name[%p], err_code[%p]", name, enc_name, err_code);

	if (!name || !enc_name)  {
		if (err_code != NULL)
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_FUNC_END();
		return false;
	}

	/* encoding mailbox name (Charset->UTF8->UTF7) */
	char *last_slash = NULL;
	char *last_word = NULL;
	char *last_enc_word = NULL;
	char *ret_enc_name = NULL;
	int err = EMAIL_ERROR_NONE;
	int ret = true;

	last_slash = rindex(name, '/');
	last_word = (last_slash) ? last_slash+1 : name;
	if (!last_word) {
		EM_DEBUG_EXCEPTION("Wrong mailbox [%s]", name);
		err = EMAIL_ERROR_INVALID_PARAM;
		ret = false;
		goto FINISH_OFF;
	}

	/* convert the last mailbox folder to utf7 char */
	last_enc_word = (char*)utf8_to_mutf7((unsigned char*)last_word);
	if (!last_enc_word) {
		EM_DEBUG_EXCEPTION("utf8_to_mutf7 failed");
		err = EMAIL_ERROR_INVALID_PARAM;
		ret = false;
		goto FINISH_OFF;
	}

	/* if last word of mailbox name is ASCII, use input */
	if (!strcmp(last_enc_word, last_word)) {
		EM_DEBUG_LOG_SEC("NOT UTF-8 [%s]", last_word);
		ret_enc_name = strdup(name);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_SEC("UTF-7 [%s](%d)->[%s](%d)", last_word, strlen(last_word), last_enc_word, strlen(last_enc_word));

	/* if last word of mailbox name is UTF-8, replace it */
	/* it is not a subfolder */
	if (!last_slash) {
		ret_enc_name = strdup(last_enc_word);
	} else { /* it is a subfolder */
		/*temprarily NULL assigned*/
		*last_slash = '\0';
		int len = strlen(name) + 1 + strlen(last_enc_word); /* including '/' */
		ret_enc_name = em_malloc(len + 1); /* NULL */
		if (!ret_enc_name) {
			EM_DEBUG_EXCEPTION("malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			ret = false;
			goto FINISH_OFF;
		}

		snprintf(ret_enc_name, len+1, "%s/%s", name, last_enc_word);
		*last_slash = '/';
	}

	EM_DEBUG_LOG_SEC("utf-7 encoding complete!! name[%s], enc_name[%s]", name, ret_enc_name);

FINISH_OFF:
	EM_SAFE_FREE(last_enc_word);
	*enc_name = ret_enc_name;
	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("result [%s]", *enc_name);
	return ret;
}

int emcore_get_temp_file_name(char **filename, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("filename[%p], err_code[%p]", filename, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;
	if (filename == NULL) {
		EM_DEBUG_EXCEPTION("\t filename[%p]\n", filename);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	char tempname[512] = {0x00, };
	struct timeval tv;


	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);

	/* Create Directory If deleted by user*/
	emstorage_create_dir_if_delete();

	SNPRINTF(tempname, sizeof(tempname), "%s%c%d", MAILTEMP, DIR_SEPERATOR_CH, rand());

	char *p = EM_SAFE_STRDUP(tempname);
	if (p == NULL) {
		EM_DEBUG_EXCEPTION("\t strdup failed...\n");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	*filename = p;

	ret = true;

FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

int emcore_get_file_name(char *path, char **filename, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("path[%s], filename[%p], err_code[%p]", path, filename, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;

	if (!path || !filename) {
		EM_DEBUG_EXCEPTION("path[%p], filename[%p]", path, filename);

		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	int i = (int)EM_SAFE_STRLEN(path);

	/*  get filename */
	for (; i >= 0; i--)
		if (path[i] == DIR_SEPERATOR_CH)
			break;

	*filename = path + i + 1;

	ret = true;

FINISH_OFF:
		if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_get_file_size(char *path, int *size, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("path[%s], size[%p], err_code[%p]", path, size, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;

	if ((path == NULL) || (size == NULL)) {
		EM_DEBUG_EXCEPTION("\t path[%p], size[%p]\n", path, size);

		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	struct stat st_buf;

	if (stat(path, &st_buf) < 0)  {
		EM_DEBUG_EXCEPTION("\t stat failed - %s\n", path);

		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	*size = st_buf.st_size;

	ret = true;

FINISH_OFF:
	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_check_drm_file(char *path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("path[%s], err_code[%p]", path, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;

	if (path == NULL) {
		EM_DEBUG_EXCEPTION("path[%p]", path);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

#ifdef __FEATURE_USE_DRM_API__
	int drm_ret = false;
	drm_bool_type_e isdrm;
	drm_ret = drm_is_drm_file(path, &isdrm);
	if (drm_ret != DRM_RETURN_SUCCESS || isdrm != DRM_TRUE) {
		EM_DEBUG_LOG("file is not drm file");
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ret = true;
#endif /* __FEATURE_USE_DRM_API__ */

FINISH_OFF:

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_check_drm_is_ringtone(char *ringtone_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("ringtone_path[%s], err_code[%p]", ringtone_path, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;

	if (ringtone_path == NULL) {
		EM_DEBUG_EXCEPTION("path[%p]", ringtone_path);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

#ifdef __FEATURE_USE_DRM_API__
	int drm_ret = false;
	drm_bool_type_e allowed = DRM_UNKNOWN;
	drm_action_allowed_data_s data;
	memset(&data, 0x00, sizeof(drm_action_allowed_data_s));
	strncpy(data.file_path, ringtone_path, strlen(ringtone_path));
	data.data = (int)DRM_SETAS_RINGTONE;

	drm_ret = drm_is_action_allowed(DRM_HAS_VALID_SETAS_STATUS, &data, &allowed);

	if (drm_ret != DRM_RETURN_SUCCESS || allowed != DRM_TRUE) {
		EM_DEBUG_LOG("fail to drm_is_action_allowed [0x%x]", drm_ret);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("allowed [DRM_TRUE]");
	ret = true;
#endif /* __FEATURE_USE_DRM_API__ */

FINISH_OFF:

	if (err_code != NULL)
		*err_code = error;

	EM_DEBUG_FUNC_END();
	return ret;
}

static int _emcore_check_host(char *host)
{
	if (!host)
		return 0;
	return strncmp(host, ".SYNTAX-ERROR.", strlen(".SYNTAX-ERROR."));
}



int emcore_get_address_count(char *addr_str, int *count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("addr_str[%s], count[%p], err_code[%p]", addr_str, count, err_code);

	int ret = false;
	int error = EMAIL_ERROR_NONE;

	ADDRESS *addr = NULL;
	ADDRESS *p_addr = NULL;
	int i = 0, j;
	char *p = NULL;


	if (!count)  {
		EM_DEBUG_EXCEPTION("addr_str[%s], count[%p]", addr_str, count);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (addr_str != NULL)  {
		em_skip_whitespace(addr_str, &p);
		EM_DEBUG_LOG("em_skip_whitespace[p][%s]", p);


		for (i = 0, j = EM_SAFE_STRLEN(p); i < j; i++)
			if (p[i] == ';') p[i] = ',';
		rfc822_parse_adrlist(&addr, p, NULL);
		EM_SAFE_FREE(p);


		for (p_addr = addr, i = 0; p_addr; p_addr = p_addr->next, i++)  {
			if (p_addr->mailbox && p_addr->host) {
				if (!strncmp(p_addr->mailbox, "UNEXPECTED_DATA_AFTER_ADDRESS", strlen("UNEXPECTED_DATA_AFTER_ADDRESS"))
				|| !strncmp(p_addr->mailbox, "INVALID_ADDRESS", strlen("INVALID_ADDRESS"))
				|| !strncmp(p_addr->host, ".SYNTAX-ERROR.", strlen(".SYNTAX-ERROR."))) { /*prevent 34356*/
					EM_DEBUG_LOG("Invalid address ");
					continue;
				}
			}
			if ((!p_addr->mailbox) || (_emcore_check_host(p_addr->host) == 0)) {
				EM_DEBUG_EXCEPTION_SEC("\t invalid address : mailbox[%s], host[%s]\n", p_addr->mailbox, p_addr->host);

				error = EMAIL_ERROR_INVALID_ADDRESS;
				/* goto FINISH_OFF; */
			}
		}
	}

	*count = i;
	if (error != EMAIL_ERROR_INVALID_ADDRESS)
	ret = true;

FINISH_OFF:
	if (addr)
		mail_free_address(&addr);

	if (err_code != NULL)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_set_network_error(int err_code)
{
	email_session_t *session = NULL;

	EM_DEBUG_FUNC_BEGIN();

	emcore_get_current_session(&session);

	if (!session)
		return false;

	session->network = err_code;
	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_get_empty_session(email_session_t **session)
{
	EM_DEBUG_FUNC_BEGIN("session[%p]", session);

	*session = &g_session;

	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_clear_session(email_session_t *session)
{
	EM_DEBUG_FUNC_BEGIN();

	if (session)
		memset(session, 0x00, sizeof(email_session_t));

	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_get_current_session(email_session_t **session)
{
	EM_DEBUG_FUNC_BEGIN("session[%p]", session);

	*session = &g_session;

	EM_DEBUG_FUNC_END();
	return true;
}

INTERNAL_FUNC int emcore_get_mail_count_by_query(char *multi_user_name, int account_id, int mailbox_type, int priority_sender, int *total_mail, int *unread_mail, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("account_id : [%d], mailbox_type : [%d], priority_sender : [%d]", account_id, mailbox_type, priority_sender);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	int type = EMAIL_PRIORITY_SENDER;
	int unread_count = 0;
	int total_count = 0;
	char *conditional_clause_string = NULL;

	int rule_count = 0;
	int is_completed = 0;
	emstorage_rule_tbl_t *rule = NULL;

	int filter_count = 0;
	email_list_filter_t *filter_list = NULL;

	/* Get rule list */
	if (priority_sender) {
		if (!emstorage_get_rule(multi_user_name, ALL_ACCOUNT, type, 0, &rule_count, &is_completed, &rule, true, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_rule failed");
			goto FINISH_OFF;
		}
		if (err == EMAIL_ERROR_FILTER_NOT_FOUND) {
			EM_DEBUG_LOG("no rule found");
			ret = true;		/*it is not an error*/
			goto FINISH_OFF;
		}
	}

	EM_DEBUG_LOG_DEV("rule count [%d]", rule_count);

	/* Make query for searching unread mail */
	filter_count = 3;   /* unseen field requires one filter, "flags_seen_field = 0"
				and "deleted_flag = 0" */

	if (rule_count > 0)
		filter_count += (rule_count * 2) + 2; /* one rule requires two filters,"A" "OR", plus two more operator "(" and ")" */


	if (account_id != ALL_ACCOUNT) {
		filter_count += 2; /* two filters, "AND" "account_id = x" */
	}

	if (mailbox_type)
		filter_count += 2; /* two filters, "AND" "mailbox_type = x" */

	filter_list = em_malloc(sizeof(email_list_filter_t) * filter_count);
	if (filter_list == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	int k = 0;

	/* priority sender only, convert one rule to two filters which is composed of from_address and or/and */
	/* example: (from_A OR from_B) AND ... */
	if (rule_count > 0) {
		/* first, add left parenthesis to from address clause, "(" */
		filter_list[0].list_filter_item_type                   = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[0].list_filter_item.operator_type          = EMAIL_LIST_FILTER_OPERATOR_LEFT_PARENTHESIS;

		for (i = 0; i < rule_count; i++) {
			/*array at odd index(1, 3, 5,..) has rule values, "from_A" */
			k = i*2+1;
			filter_list[k].list_filter_item_type                             = EMAIL_LIST_FILTER_ITEM_RULE;
			filter_list[k].list_filter_item.rule.rule_type                   = EMAIL_LIST_FILTER_RULE_INCLUDE;
			filter_list[k].list_filter_item.rule.target_attribute            = EMAIL_MAIL_ATTRIBUTE_FROM;
			filter_list[k].list_filter_item.rule.key_value.string_type_value = EM_SAFE_STRDUP(rule[i].value2);

			/*array at even index(2, 4, 6,..) has either AND ,or OR*/
			if (i != (rule_count - 1)) { /*if it is not the last rule, "OR" */
				filter_list[++k].list_filter_item_type                 = EMAIL_LIST_FILTER_ITEM_OPERATOR;
				filter_list[k].list_filter_item.operator_type          = EMAIL_LIST_FILTER_OPERATOR_OR;

			} else { /* ")" "AND" */
				filter_list[++k].list_filter_item_type         = EMAIL_LIST_FILTER_ITEM_OPERATOR;
				filter_list[k].list_filter_item.operator_type  = EMAIL_LIST_FILTER_OPERATOR_RIGHT_PARENTHESIS;

				filter_list[++k].list_filter_item_type         = EMAIL_LIST_FILTER_ITEM_OPERATOR;
				filter_list[k].list_filter_item.operator_type  = EMAIL_LIST_FILTER_OPERATOR_AND;
				k++;
			}
		}
	}

	/*rule value, unseen +1*/
	filter_list[k].list_filter_item_type                                = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[k].list_filter_item.rule.rule_type                      = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[k].list_filter_item.rule.target_attribute               = EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD;
	filter_list[k++].list_filter_item.rule.key_value.integer_type_value = 0;

	filter_list[k].list_filter_item_type                                = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[k++].list_filter_item.operator_type                     = EMAIL_LIST_FILTER_OPERATOR_AND;

	filter_list[k].list_filter_item_type                                = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[k].list_filter_item.rule.rule_type                      = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[k].list_filter_item.rule.target_attribute               = EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS;
	filter_list[k++].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAIL_STATUS_SAVED_OFFLINE;

	/*account_id requires two filters*/
	if (account_id != ALL_ACCOUNT) {
		/* odd index, logical operator */
		filter_list[k].list_filter_item_type                              = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[k++].list_filter_item.operator_type                   = EMAIL_LIST_FILTER_OPERATOR_AND;

		/* even index, rule value */
		filter_list[k].list_filter_item_type                              = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[k].list_filter_item.rule.rule_type                    = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[k].list_filter_item.rule.target_attribute             = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
		filter_list[k++].list_filter_item.rule.key_value.integer_type_value = account_id;
	}

	/*mailbox_type requires two filters*/
	if (mailbox_type) {
		/* odd index, logical operator */
		filter_list[k].list_filter_item_type                              = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[k++].list_filter_item.operator_type                   = EMAIL_LIST_FILTER_OPERATOR_AND;

		/* even index, rule value */
		filter_list[k].list_filter_item_type                              = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[k].list_filter_item.rule.rule_type                    = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[k].list_filter_item.rule.target_attribute             = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
		filter_list[k++].list_filter_item.rule.key_value.integer_type_value = mailbox_type;
	}

	if ((err = emstorage_write_conditional_clause_for_getting_mail_list(multi_user_name, filter_list, filter_count, NULL, 0, -1, -1, &conditional_clause_string)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_write_conditional_clause_for_getting_mail_list failed[%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_DEV("conditional_clause_string[%s]", conditional_clause_string);

	/* Search the mail of priority sender in DB */
	if ((err = emstorage_query_mail_count(multi_user_name, conditional_clause_string, true, &total_count, &unread_count)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emstorage_query_mail_count failed:[%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (rule)
		emstorage_free_rule(&rule, rule_count, NULL);

	if (filter_list)
		emstorage_free_list_filter(&filter_list, filter_count);

	EM_SAFE_FREE(conditional_clause_string);

	if (total_mail)
		*total_mail = total_count;

	if (unread_mail)
		*unread_mail = unread_count;

	if (err_code)
		*err_code = err;

	return ret;
}

INTERNAL_FUNC int emcore_display_badge_count(char *multi_user_name, int count)
{
	EM_DEBUG_FUNC_BEGIN();
	/* Use badge API */
	int err = EMAIL_ERROR_NONE;
	badge_error_e badge_err = BADGE_ERROR_NONE;
	bool exist;
	void *join_zone = NULL;

	if ((err = emcore_set_join_zone(multi_user_name, &join_zone)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_set_join_zone failed : [%d]", err);
		goto FINISH_OFF;
	}

	if ((badge_err = badge_is_existing("org.tizen.email", &exist)) != BADGE_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("badge_is_existing failed [%d]", badge_err);
		err = EMAIL_ERROR_BADGE_API_FAILED;
		goto FINISH_OFF;
	}

	if (!exist) {
		/* create badge */
		if ((badge_err = badge_create("org.tizen.email", "/usr/bin/email-service")) != BADGE_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("badge_create failed [%d]", badge_err);
			err = EMAIL_ERROR_BADGE_API_FAILED;
			goto FINISH_OFF;
		}
	}

	if ((badge_err = badge_set_count("org.tizen.email", count)) != BADGE_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("badge_set_count failed [%d]", badge_err);
		if (badge_err != BADGE_ERROR_SERVICE_NOT_READY) {
			err = EMAIL_ERROR_BADGE_API_FAILED;
			goto FINISH_OFF;
		}

		/* Badge callback function : When the badge service ready, call the callback function */
		badge_err = badge_add_deferred_task(emcore_display_unread_in_badge, NULL);
		if (badge_err != BADGE_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("badge_add_deferred_task failed : [%d]", badge_err);
			err = EMAIL_ERROR_BADGE_API_FAILED;
			goto FINISH_OFF;
		}
	}

FINISH_OFF:

	if (join_zone)
		emcore_unset_join_zone(join_zone);

	EM_DEBUG_FUNC_END();
	return err;
}

void emcore_display_unread_in_badge(void *data)
{
	EM_DEBUG_FUNC_BEGIN();

	int err = EMAIL_ERROR_NONE;
	int total_unread_count = 0;
	int total_mail_count = 0;
	int unseen = 0;

	if (!emcore_get_mail_count_by_query((char *)data, ALL_ACCOUNT,
										EMAIL_MAILBOX_TYPE_INBOX, 0,
										&total_mail_count, &total_unread_count,
										&err)) {
		EM_DEBUG_EXCEPTION("emcore_get_mail_count_by_query failed");
		goto FINISH_OFF;
	}

	unseen = total_unread_count;

FINISH_OFF:

	if (unseen <= 0) {
		if ((err = emcore_display_badge_count((char *)data, 0)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_display_badge_count failed : [%d]", err);
		}
	} else {
		if ((err = emcore_display_badge_count((char *)data, unseen)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_display_badge_count failed : [%d]", err);
		}
	}

	EM_DEBUG_FUNC_END();
}

static int emcore_layout_multi_noti(notification_h noti, int unread_mail, char *email_address, char *account_name)
{
	EM_DEBUG_FUNC_BEGIN("unread_count %d", unread_mail);
	char temp_buffer[512] = {0};
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err = notification_set_layout(noti, NOTIFICATION_LY_NOTI_EVENT_MULTIPLE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_layout NOTI_EVENT_MULTIPLE failed [%d]", noti_err);
		goto FINISH_OFF;
	}

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, "New emails", dgettext(NATIVE_EMAIL_DOMAIN, "IDS_EMAIL_MBODY_NEW_EMAILS_ABB"), NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text TEXT_TYPE_TITLE failed");
		goto FINISH_OFF;
	}

	SNPRINTF(temp_buffer, sizeof(temp_buffer), "%d", unread_mail);
	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, temp_buffer, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text TEXT_TYPE_EVENT_COUNT failed");
		goto FINISH_OFF;
	}

	if (account_name)
		SNPRINTF(temp_buffer, sizeof(temp_buffer), "%s, %s", email_address, account_name);
	else
		SNPRINTF(temp_buffer, sizeof(temp_buffer), "%s", email_address);

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, temp_buffer, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text TEXT_TYPE_CONTENT failed");
		goto FINISH_OFF;
	}

	noti_err = notification_set_time(noti, time(NULL));
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_time failed");
		goto FINISH_OFF;
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, EMAIL_NOTI_ICON_PATH);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_image TYPE_ICON failed");
		goto FINISH_OFF;
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, EMAIL_NOTI_INDICATOR_ICON_PATH);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_image TYPE_ICON failed");
		goto FINISH_OFF;
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_LOCK, EMAIL_NOTI_ICON_PATH);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_image TYPE_ICON failed");
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("noti_err : [%d]", noti_err);
	return noti_err;
}

static int emcore_layout_single_noti(notification_h noti, char *account_name, int ids, char *display_sender, time_t time, char *subject, int display_status)
{
	EM_DEBUG_FUNC_BEGIN();
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err = notification_set_layout(noti, NOTIFICATION_LY_NOTI_EVENT_SINGLE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_layout NOTI_EVENT_SINGLE failed [%d]", noti_err);
		goto FINISH_OFF;
	}

	if (ids)
		noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, "Email", dgettext(NATIVE_EMAIL_DOMAIN, "IDS_ST_HEADER_EMAIL"), NOTIFICATION_VARIABLE_TYPE_NONE);
	else
		noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, display_sender, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);

	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text TEXT_TYPE_TITLE failed");
		goto FINISH_OFF;
	}
/*
	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, account_name, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text TEXT_TYPE_CONTENT failed");
		goto FINISH_OFF;
	}

	if (ids)
		noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, NULL, display_sender, NOTIFICATION_VARIABLE_TYPE_NONE);
	else
		noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, display_sender, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);

	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text TEXT_TYPE_CONTENT failed");
		goto FINISH_OFF;
	}
*/
	if (display_status)
		noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, subject, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
	else
		noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_INFO_1, "New email", dgettext(NATIVE_EMAIL_DOMAIN, "IDS_EMAIL_TPOP_NEW_EMAIL_RECEIVED_ABB"), NOTIFICATION_VARIABLE_TYPE_NONE);

	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text TEXT_TYPE_INFO_1 failed");
		goto FINISH_OFF;
	}

	noti_err = notification_set_time(noti, time);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_time failed");
		goto FINISH_OFF;
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, EMAIL_NOTI_ICON_PATH);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_image TYPE_ICON failed");
		goto FINISH_OFF;
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, EMAIL_NOTI_INDICATOR_ICON_PATH);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_image TYPE_ICON failed");
		goto FINISH_OFF;
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_LOCK, EMAIL_NOTI_ICON_PATH);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_image TYPE_ICON failed");
		goto FINISH_OFF;
	}
FINISH_OFF:

	EM_DEBUG_FUNC_END("noti_err : [%d]", noti_err);
	return noti_err;
}

static int emcore_get_alert_type(int vibrate_status)
{
	EM_DEBUG_FUNC_BEGIN();
	int global_sound_status = 0;
	int global_vibe_status = 0;
	int email_vibe_status = 0;
	int call_state = 0;
	int alert_type = EMAIL_ALERT_TYPE_MUTE;
#ifdef __FEATURE_VOICERECORDER_STATUS_FOR_NOTI__
	int voicerecoder_state = 0;
#endif /* __FEATURE_VOICERECORDER_STATUS_FOR_NOTI__ */

	if (vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &global_sound_status) != 0) {
		EM_DEBUG_LOG("vconf_get_bool for VCONFKEY_SETAPPL_SOUND_STATUS_BOOL failed");
		goto FINISH_OFF;
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &global_vibe_status) != 0) {
		EM_DEBUG_LOG("vconf_get_bool for VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOLfailed");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("global_sound_status [%d] global_vibe_status [%d]", global_sound_status, global_vibe_status);

	if (global_sound_status || global_vibe_status) {
#ifdef __FEATURE_VOICERECORDER_STATUS_FOR_NOTI__
		if (vconf_get_int(VCONFKEY_VOICERECORDER_STATE, &voicerecoder_state) != 0) {
			EM_DEBUG_LOG("vconf_get_int for VCONFKEY_VOICERECORDER_STATE failed");
		}
		EM_DEBUG_LOG("voicerecoder_state [%d]", voicerecoder_state);
#endif /* __FEATURE_VOICERECORDER_STATUS_FOR_NOTI__ */

		if (vconf_get_int(VCONFKEY_CALL_STATE, &call_state) != 0) {
			EM_DEBUG_LOG("vconf_get_int for VCONFKEY_CALL_STATE failed");
		}
		EM_DEBUG_LOG("call_state [%d] ", call_state);

		email_vibe_status = vibrate_status;

		EM_DEBUG_LOG("email_vibe_status [%d] ", email_vibe_status);
#ifdef __FEATURE_VOICERECORDER_STATUS_FOR_NOTI__
		if (voicerecoder_state == VCONFKEY_VOICERECORDER_RECORDING) {
			alert_type = EMAIL_ALERT_TYPE_VIB;
			EM_DEBUG_LOG("voice recorder is on recording...");
		}
/*		else if (call_state > VCONFKEY_CALL_OFF && call_state < VCONFKEY_CALL_STATE_MAX) { */
#endif /* __FEATURE_VOICERECORDER_STATUS_FOR_NOTI__ */

		if (call_state > VCONFKEY_CALL_OFF && call_state < VCONFKEY_CALL_STATE_MAX) {
			EM_DEBUG_LOG("Calling");
			if (global_sound_status)
				alert_type = EMAIL_ALERT_TYPE_MELODY;
			else
				alert_type = EMAIL_ALERT_TYPE_NONE;
		} else if (global_sound_status && email_vibe_status) {
			alert_type = EMAIL_ALERT_TYPE_MELODY_AND_VIB;
		} else if (global_sound_status) {
			alert_type = EMAIL_ALERT_TYPE_MELODY;
		} else if (global_vibe_status) {
			alert_type = EMAIL_ALERT_TYPE_VIB;
		}
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("alert_type [%d]", alert_type);
	return alert_type;
}

static char *emcore_get_sound_file_path(int default_ringtone_status, char *alert_ringtone_path)
{
	EM_DEBUG_FUNC_BEGIN();
	char *ret = NULL;
	int   use_default_ring_tone = 0;

	use_default_ring_tone = default_ringtone_status;

	EM_DEBUG_LOG("use_default_ring_tone [%d]", use_default_ring_tone);

	if (use_default_ring_tone)
		ret = vconf_get_str(VCONFKEY_SETAPPL_NOTI_MSG_RINGTONE_PATH_STR);
	else
		ret = EM_SAFE_STRDUP(alert_ringtone_path);

	if ((emcore_check_drm_file(ret, NULL)) && (!emcore_check_drm_is_ringtone(ret, NULL))) {
		EM_DEBUG_LOG_DEV("The ringtone is not allowed DRM. : [%s]", ret);
		EM_SAFE_FREE(ret);
		ret = EM_SAFE_STRDUP(vconf_get_str(VCONFKEY_SETAPPL_NOTI_MSG_RINGTONE_PATH_STR));
	}

	EM_DEBUG_FUNC_END("ret [%s]", ret);
	return ret;
}

#if 0 //using notification_status_message_post instead of calling email-app toast - change date: 30/9/2014
INTERNAL_FUNC int emcore_show_toast_popup(char *input_popup_string)
{
	int err = EMAIL_ERROR_NONE;
	int ret = APP_CONTROL_ERROR_NONE;
	app_control_h svc_handle = NULL;

	ret = app_control_create(&svc_handle);

	if (ret != APP_CONTROL_ERROR_NONE || !svc_handle) {
		EM_DEBUG_LOG("app_control_create() failed! ret:[%d]", ret);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ret = app_control_set_app_id(svc_handle, "org.tizen.email-tts-play");
	if (ret != APP_CONTROL_ERROR_NONE) {
		EM_DEBUG_LOG("app_control_set_app_id() failed! ret:[%d]", ret);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ret = app_control_add_extra_data(svc_handle, "email_misc_work_type", "1003");
	if (ret != APP_CONTROL_ERROR_NONE) {
		EM_DEBUG_LOG("app_control_add_extra_data() failed! ret:[%d]", ret);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ret = app_control_add_extra_data(svc_handle, "popup_string", input_popup_string);
	if (ret != APP_CONTROL_ERROR_NONE) {
		EM_DEBUG_LOG("app_control_add_extra_data() failed! ret:[%d]", ret);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ret = app_control_send_launch_request(svc_handle, NULL, NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		EM_DEBUG_LOG("app_control_send_launch_request() failed! ret:[%d]", ret);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

FINISH_OFF:

    if (svc_handle) {
        ret = app_control_destroy(svc_handle);
        if (ret != APP_CONTROL_ERROR_NONE) {
            EM_DEBUG_LOG("app_control_destroy() failed! ret:[%d]", ret);
            err = EMAIL_ERROR_SYSTEM_FAILURE;
        }
    }

	return err;
}
#endif


INTERNAL_FUNC int emcore_add_notification(char *multi_user_name, int account_id, int mail_id, int unread_mail_count, int vip_unread_mail_count, int input_play_alert_tone, int sending_error, unsigned long display)
{
	EM_DEBUG_FUNC_BEGIN("account_id[%d] mail_id[%d] unread_mail_count[%d] input_play_alert_tone[%d]", account_id, mail_id, unread_mail_count, input_play_alert_tone);
	int err = EMAIL_ERROR_NONE;
#ifdef __FEATURE_NOTIFICATION_ENABLE__
	int i = 0;
	int private_id = 0;
	int *p_mail_id = 0;
	int unseen = 0;
	int new_noti = 0;
	int unread_mail_count_t = 0;
	char *mailbox_name = NULL;
	char vconf_private_id[MAX_PATH] = {0, };
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	notification_vibration_type_e noti_vibe_type = NOTIFICATION_VIBRATION_TYPE_NONE;
	emstorage_account_tbl_t *account_tbl = NULL;
	emstorage_mail_tbl_t *p_mail_data = NULL;
	int display_status = false;
	EMAIL_ALERT_TYPE alert_type = EMAIL_ALERT_TYPE_NONE;
	char *alert_tone_path = NULL;
	void *join_zone = NULL;

	/* For service bundle */
	char buf[256] = {0,};
	char **value = NULL;
	app_control_h service = NULL;
	bundle *b = NULL;

	if (!emstorage_get_account_by_id(multi_user_name, account_id, GET_FULL_DATA_WITHOUT_PASSWORD, &account_tbl, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!account_tbl->options.notification_status) {
		EM_DEBUG_LOG("Notification disabled");
		goto FINISH_OFF;
	}

	/* Get the unread mail id */
	if (!emstorage_get_unread_mailid(multi_user_name, account_id, 0, &p_mail_id, &unread_mail_count_t, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_unread_mailid failed [%d]", err);
		goto FINISH_OFF;
	}

	unseen = unread_mail_count_t;
	display_status = account_tbl->options.display_content_status;

	/* Load the previous noti */
	SNPRINTF(vconf_private_id, sizeof(vconf_private_id), "%s/%d", VCONF_KEY_NOTI_PRIVATE_ID, account_id);
	if (vconf_get_int(vconf_private_id, &private_id) != 0) {
	      EM_DEBUG_EXCEPTION("vconf_get_int failed");
	}

	/* Turn display on to blink LED */
//	emdevice_change_display_state(DISPLAY_STATE_ON);

	if ((err = emcore_set_join_zone(multi_user_name, &join_zone)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_set_join_zone failed : [%d]", err);
		goto FINISH_OFF;
	}

	noti = notification_load(NULL, private_id);
	if (noti == NULL) {
		EM_DEBUG_EXCEPTION("notification_load failed");
		noti = notification_new(NOTIFICATION_TYPE_NOTI, NOTIFICATION_GROUP_ID_NONE, NOTIFICATION_PRIV_ID_NONE);
		if (noti == NULL) {
			EM_DEBUG_EXCEPTION("notification_new failed");
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		if ((noti_err = notification_set_pkgname(noti, NATIVE_EMAIL_APPLICATION_PKG)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_pkgname failed [%d]", noti_err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		new_noti = true;
	}

	if (input_play_alert_tone) {
		alert_type = emcore_get_alert_type(account_tbl->options.vibrate_status);

		if (alert_type == EMAIL_ALERT_TYPE_MELODY_AND_VIB || alert_type == EMAIL_ALERT_TYPE_MELODY)
			alert_tone_path = emcore_get_sound_file_path(account_tbl->options.default_ringtone_status, account_tbl->options.alert_ringtone_path);
	}

	/* blocking mode : */
#ifdef __FEATURE_BLOCKING_MODE__
	if (!emcore_get_blocking_mode_status()) {
#endif /* __FEATURE_BLOCKING_MODE__ */
		if ((noti_err = notification_set_led_time_period(noti, 250, 2500)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_led_time_period failed [%d]", noti_err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		if ((noti_err = notification_set_led(noti, NOTIFICATION_LED_OP_ON, 0x00)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_led failed [%d]", noti_err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		if ((alert_type == EMAIL_ALERT_TYPE_MELODY_AND_VIB || alert_type == EMAIL_ALERT_TYPE_MELODY) && EM_SAFE_STRLEN(alert_tone_path)) {
			if (strcmp(alert_tone_path, "silent") == 0) {
				if ((noti_err = notification_set_sound(noti, NOTIFICATION_SOUND_TYPE_DEFAULT, NULL)) != NOTIFICATION_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("notification_set_sound failed [%d]", noti_err);
					err = EMAIL_ERROR_SYSTEM_FAILURE;
					goto FINISH_OFF;
				}
			} else {
				if ((noti_err = notification_set_sound(noti, NOTIFICATION_SOUND_TYPE_USER_DATA, alert_tone_path)) != NOTIFICATION_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("notification_set_sound failed [%d]", noti_err);
					err = EMAIL_ERROR_SYSTEM_FAILURE;
					goto FINISH_OFF;
				}
			}
		}
#ifdef __FEATURE_BLOCKING_MODE__
	}
#endif /* __FEATURE_BLOCKING_MODE__ */

	if (alert_type == EMAIL_ALERT_TYPE_MELODY_AND_VIB || alert_type == EMAIL_ALERT_TYPE_VIB)
		noti_vibe_type = NOTIFICATION_VIBRATION_TYPE_DEFAULT;

	if ((noti_err = notification_set_vibration(noti, noti_vibe_type, NULL)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_vibration failed [%d]", noti_err);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if ((noti_err = notification_set_text_domain(noti, NATIVE_EMAIL_DOMAIN, tzplatform_mkpath(TZ_SYS_RO_APP,"org.tizen.email/res/locale"))) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_text_domain failed [%d]", noti_err);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mailbox_name_by_mailbox_type(multi_user_name, account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox_name, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mailbox_name_by_mailbox_type failed [%d]", err);
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("unseen [%d] display_status [%d]", unseen, display_status);

	/* set execution option to launch email-viewer when single incoming mail*/
	app_control_create(&service);
	if (!service) {
		EM_DEBUG_EXCEPTION("service create failed");
	}

	if (unseen == 1) {
		if (!emstorage_get_mail_by_id(multi_user_name, p_mail_id[0], &p_mail_data, false, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_SEC("single layout:account_name[%s], alias_sender[%s], time[%d], subject[%s]", account_tbl->account_name, p_mail_data->alias_sender, p_mail_data->date_time, p_mail_data->subject);
		if ((noti_err = emcore_layout_single_noti(noti, account_tbl->account_name, 0, p_mail_data->alias_sender, p_mail_data->date_time, p_mail_data->subject, display_status)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_layout_single_noti failed [%d]", noti_err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		snprintf(buf, sizeof(buf), "%d", 12);
		app_control_add_extra_data(service, "RUN_TYPE", buf);
		snprintf(buf, sizeof(buf), "%d", account_id);
		app_control_add_extra_data(service, "ACCOUNT_ID", buf);
		snprintf(buf, sizeof(buf), "%d", p_mail_data->mail_id);
		app_control_add_extra_data(service, "MAIL_ID", buf);
		snprintf(buf, sizeof(buf), "%d", p_mail_data->mailbox_id);
		app_control_add_extra_data(service, "MAILBOX_ID", buf);
		app_control_set_app_id(service, NATIVE_EMAIL_APPLICATION_PKG);

		app_control_to_bundle(service, &b);
		noti_err = notification_set_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH, NULL, NULL, b);
		if (noti_err != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_execute_option failed [%d]", noti_err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}
	} else if (unseen > 1) {
	    EM_DEBUG_LOG_SEC("multi layout: unseen[%d], address[%s], account_name[%s]", unseen, account_tbl->user_email_address, account_tbl->account_name);
		if ((noti_err = emcore_layout_multi_noti(noti, unseen, account_tbl->user_email_address, account_tbl->account_name)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_layout_multi_noti failed [%d]", noti_err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		snprintf(buf, sizeof(buf), "%d", 13);
		app_control_add_extra_data(service, "RUN_TYPE", buf);
		snprintf(buf, sizeof(buf), "%d", account_id);
		app_control_add_extra_data(service, "ACCOUNT_ID", buf);

		if (unseen > EMAIL_NOTI_MAX_MAIL_ID)
		    unseen = EMAIL_NOTI_MAX_MAIL_ID;
		value = (char **)em_malloc(unseen * sizeof(char *));
		if (value == NULL) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		for (i = 0; i < unseen; i++) {
			memset(buf, 0x00, sizeof(buf));
			snprintf(buf, sizeof(buf), "%d", p_mail_id[i]);
			value[i] = EM_SAFE_STRDUP(buf);
		}
		app_control_add_extra_data_array(service, "MAIL_ID", (const char **)value, unseen);
		app_control_set_app_id(service, NATIVE_EMAIL_APPLICATION_PKG);

		app_control_to_bundle(service, &b);
		noti_err = notification_set_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_MULTI_LAUNCH, NULL, NULL, b);
		if (noti_err != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_execute_option failed [%d]", noti_err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}
	} else { /* if priority sender exists and no mail of priority sender, unseen will be 0*/
		EM_DEBUG_LOG("-unseen: [%d]", unseen);
		goto FINISH_OFF;
		/* err = EMAIL_ERROR_PRIORITY_SENDER_MAIL_NOT_FOUND;*/ /*there is no mail for priority sender*/
	}

	if ((noti_err = notification_set_display_applist(noti, display)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_insert failed [%d]", noti_err);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (new_noti) {
		if ((noti_err = notification_insert(noti, &private_id)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_insert failed [%d]", noti_err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("Private_id = [%d]", private_id);

		SNPRINTF(vconf_private_id, sizeof(vconf_private_id), "%s/%d", VCONF_KEY_NOTI_PRIVATE_ID, account_id);
		if (vconf_set_int(vconf_private_id, private_id) != 0) {
			EM_DEBUG_EXCEPTION("vconf_set_int failed");
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}
	} else {
		if ((noti_err = notification_update(noti)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_update failed [%d]", noti_err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}
	}

#ifdef __FEATURE_DRIVING_MODE__
	if (unseen) {
		err = emcore_start_driving_mode(multi_user_name, p_mail_id[unseen - 1]);
		if (err != EMAIL_ERROR_NONE)
			EM_DEBUG_EXCEPTION("emcore_start_driving_mode failed : [%d]", err);
	}
#endif /* __FEATURE_DRIVING_MODE__ */

FINISH_OFF:

	if (service)
		app_control_destroy(service);

	if (noti)
		notification_free(noti);

	if (join_zone)
		emcore_unset_join_zone(join_zone);

	EM_SAFE_FREE(alert_tone_path);
	EM_SAFE_FREE(p_mail_id); /* prevent */

	if (p_mail_data)
		emstorage_free_mail(&p_mail_data, 1, NULL);

	if (account_tbl)
		emstorage_free_account(&account_tbl, 1, NULL);

	for (i = 0; i < unseen; i++) {
		if (value)
			EM_SAFE_FREE(value[i]);
	}
	EM_SAFE_FREE(value);

	EM_SAFE_FREE(mailbox_name);

#endif  /* __FEATURE_NOTIFICATION_ENABLE__ */

	EM_DEBUG_FUNC_END("ret [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_add_notification_for_send(char *multi_user_name, int account_id, int mail_id, email_action_t action, int sending_error, unsigned long display)
{
	EM_DEBUG_FUNC_BEGIN("account_id: %d, mail_id: %d, action:%d", account_id, mail_id, action);
	int err = EMAIL_ERROR_NONE;
#ifdef __FEATURE_NOTIFICATION_ENABLE__
	int private_id = 0;
	void *join_zone = NULL;
	char *mailbox_name = NULL;
	char *dgettext_string = NULL;
	char vconf_private_id[MAX_PATH] = {0, };
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	emstorage_mail_tbl_t *p_mail_data = NULL;
	emstorage_account_tbl_t *account_tbl = NULL;

	/* For service bundle */
	char buf[256] = {0,};
	app_control_h service = NULL;
	bundle *b = NULL;

	/* get general noti status */
	if (!emstorage_get_account_by_id(multi_user_name, account_id, GET_FULL_DATA_WITHOUT_PASSWORD, &account_tbl, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (!account_tbl->options.notification_status) {
		EM_DEBUG_LOG("notification disabled");
		goto FINISH_OFF;
	}

	if (!account_tbl->options.display_content_status) {
		EM_DEBUG_LOG("display content disabled");
		goto FINISH_OFF;
	}

	/* Delete the previous noti */
	emcore_delete_notification_by_account(multi_user_name, account_id, true);
	if (g_sending_noti_handle) {
		if ((noti_err = notification_free(g_sending_noti_handle)) != NOTIFICATION_ERROR_NONE)
			err = EMAIL_ERROR_NOTI;
		g_sending_noti_handle = NULL;
	}

	if ((err = emcore_set_join_zone(multi_user_name, &join_zone)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_set_join_zone failed : [%d]", err);
		goto FINISH_OFF;
	}

	if (action == EMAIL_ACTION_SENDING_MAIL)
		noti = notification_create(NOTIFICATION_TYPE_ONGOING);
	else
		noti = notification_new(NOTIFICATION_TYPE_NOTI, NOTIFICATION_GROUP_ID_NONE, NOTIFICATION_PRIV_ID_NONE);

	if (noti == NULL) {
		EM_DEBUG_EXCEPTION("notification_new failed");
		err = EMAIL_ERROR_NOTI;
		goto FINISH_OFF;
	}

	if ((noti_err = notification_set_pkgname(noti, NATIVE_EMAIL_APPLICATION_PKG)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_pkgname failed [%d]", noti_err);
		err = EMAIL_ERROR_NOTI;
		goto FINISH_OFF;
	}

	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &p_mail_data, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	/* Create service bundle */
	app_control_create(&service);
	if (!service) {
		EM_DEBUG_EXCEPTION("service create failed");
	}

	snprintf(buf, sizeof(buf), "%d", 12);
	app_control_add_extra_data(service, "RUN_TYPE", buf);
	snprintf(buf, sizeof(buf), "%d", account_id);
	app_control_add_extra_data(service, "ACCOUNT_ID", buf);
	snprintf(buf, sizeof(buf), "%d", p_mail_data->mail_id);
	app_control_add_extra_data(service, "MAIL_ID", buf);
	snprintf(buf, sizeof(buf), "%d", p_mail_data->mailbox_id);
	app_control_add_extra_data(service, "MAILBOX_ID", buf);
	app_control_set_app_id(service, NATIVE_EMAIL_APPLICATION_PKG);

	app_control_to_bundle(service, &b);
	noti_err = notification_set_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH, NULL, NULL, b);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_set_execute_option failed [%d]", noti_err);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	switch (action) {
	case EMAIL_ACTION_SEND_MAIL:
/*
		setlocale(LC_MESSAGES, vconf_get_str(VCONFKEY_LANGSET));
		bindtextdomain("sys_string", "/usr/share/locale");
		textdomain("sys_string");
*/
		setlocale(LC_MESSAGES, vconf_get_str(VCONFKEY_LANGSET));
		bindtextdomain(NATIVE_EMAIL_DOMAIN, tzplatform_mkpath(TZ_SYS_RO_APP,"org.tizen.email/res/locale"));
		textdomain(NATIVE_EMAIL_DOMAIN);

		switch (sending_error) {
		case EMAIL_ERROR_NONE:
			dgettext_string = dgettext(NATIVE_EMAIL_DOMAIN, "IDS_EMAIL_TPOP_EMAIL_SENT");
			break;
		case EMAIL_ERROR_FLIGHT_MODE_ENABLE:
			dgettext_string = dgettext(NATIVE_EMAIL_DOMAIN, "IDS_EMAIL_TPOP_FAILED_TO_CONNECT_TO_NETWORK");
			break;
		case EMAIL_ERROR_NETWORK_NOT_AVAILABLE:
		case EMAIL_ERROR_NO_SIM_INSERTED:
			dgettext_string = dgettext(NATIVE_EMAIL_DOMAIN, "IDS_EMAIL_TPOP_NETWORK_NOT_AVAILABLE_EMAIL_WILL_BE_SENT_WHEN_CONNECTED_TO_NETWORK");
			break;
		case EMAIL_ERROR_SERVER_STORAGE_FULL:
			dgettext_string = dgettext(NATIVE_EMAIL_DOMAIN, "IDS_EMAIL_TPOP_SENDING_FAILED_SERVER_STORAGE_FULL_ABB");
			break;
		default:
			dgettext_string = dgettext(NATIVE_EMAIL_DOMAIN, "IDS_EMAIL_TPOP_SENDING_FAILED");
			break;
		}

		if ((noti_err = notification_set_layout(noti,
												NOTIFICATION_LY_NOTI_EVENT_SINGLE)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_layout failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		if ((noti_err = notification_set_image(noti,
												NOTIFICATION_IMAGE_TYPE_ICON,
												EMAIL_NOTI_ICON_PATH)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_image failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		if ((noti_err = notification_set_text(noti,
											NOTIFICATION_TEXT_TYPE_TITLE,
											p_mail_data->alias_recipient,
											NULL,
											NOTIFICATION_VARIABLE_TYPE_NONE)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_text failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		switch (sending_error) {
		case EMAIL_ERROR_NONE:
			if ((noti_err = notification_set_text(noti,
												NOTIFICATION_TEXT_TYPE_CONTENT,
												"Email Sent",
												dgettext_string,
												NOTIFICATION_VARIABLE_TYPE_NONE)) != NOTIFICATION_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("notification_set_text failed [%d]", noti_err);
				err = EMAIL_ERROR_NOTI;
				goto FINISH_OFF;
			}
			break;
		case EMAIL_ERROR_SERVER_STORAGE_FULL:
			if ((noti_err = notification_set_text(noti,
												NOTIFICATION_TEXT_TYPE_CONTENT,
												"Sending failed. Server storage full.",
												dgettext_string,
												NOTIFICATION_VARIABLE_TYPE_NONE)) != NOTIFICATION_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("notification_set_text failed [%d]", noti_err);
				err = EMAIL_ERROR_NOTI;
				goto FINISH_OFF;
			}
			break;
		default:
			if ((noti_err = notification_set_text(noti,
												NOTIFICATION_TEXT_TYPE_CONTENT,
												"Sending failed",
												dgettext_string,
												NOTIFICATION_VARIABLE_TYPE_NONE)) != NOTIFICATION_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("notification_set_text failed [%d]", noti_err);
				err = EMAIL_ERROR_NOTI;
				goto FINISH_OFF;
			}
			break;
		}

		if (sending_error == EMAIL_ERROR_SMTP_SEND_FAILURE_BY_OVERSIZE) {
			if ((noti_err = notification_set_text(noti,
												NOTIFICATION_TEXT_TYPE_INFO_1,
												dgettext(NATIVE_EMAIL_DOMAIN, "IDS_EMAIL_POP_THE_EMAIL_IS_TOO_LARGE"),
												NULL,
												NOTIFICATION_VARIABLE_TYPE_NONE)) != NOTIFICATION_ERROR_NONE) {
				EM_DEBUG_EXCEPTION("notification_set_text TEXT_TYPE_INFO_1 failed");
				err = EMAIL_ERROR_NOTI;
				goto FINISH_OFF;
			}
		}

		if ((noti_err = notification_set_display_applist(noti,
												NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY | NOTIFICATION_DISPLAY_APP_TICKER)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_insert failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		if ((noti_err = notification_insert(noti, &private_id)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_insert failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("Private_id = [%d]", private_id);

		SNPRINTF(vconf_private_id, sizeof(vconf_private_id), "%s/%d", VCONF_KEY_NOTI_PRIVATE_ID, account_id);
		if (vconf_set_int(vconf_private_id, private_id) != 0) {
			EM_DEBUG_EXCEPTION("vconf_set_int failed");
			err = EMAIL_ERROR_GCONF_FAILURE;
			goto FINISH_OFF;
		}

		break;

	case EMAIL_ACTION_SENDING_MAIL:

		if ((noti_err = notification_set_text_domain(noti, NATIVE_EMAIL_DOMAIN, tzplatform_mkpath(TZ_SYS_RO_APP,"org.tizen.email/res/locale"))) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_text_domain failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		if ((noti_err = notification_set_layout(noti, NOTIFICATION_LY_ONGOING_PROGRESS)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_layout failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		if ((noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, EMAIL_NOTI_ICON_PATH)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_image failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		if ((noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, "Sending email...", dgettext(NATIVE_EMAIL_DOMAIN, "IDS_EMAIL_TPOP_SENDING_EMAIL_ING"), NOTIFICATION_VARIABLE_TYPE_NONE)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_text failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		if ((noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, p_mail_data->subject, NULL, NOTIFICATION_VARIABLE_TYPE_NONE)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_set_text failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		/*notification_set_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH, NULL, NULL, args);*/

		if ((noti_err = notification_set_display_applist(noti, NOTIFICATION_DISPLAY_APP_ALL)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_insert failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		if ((noti_err = notification_insert(noti, &private_id)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_insert failed [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
			goto FINISH_OFF;
		}

		g_sending_noti_handle = noti;
		EM_DEBUG_LOG("Private_id = [%d]", private_id);

		SNPRINTF(vconf_private_id, sizeof(vconf_private_id), "%s/%d", VCONF_KEY_NOTI_PRIVATE_ID, account_id);
		if (vconf_set_int (vconf_private_id, private_id) != 0) {
			EM_DEBUG_EXCEPTION("vconf_set_int failed");
			err = EMAIL_ERROR_GCONF_FAILURE;
			goto FINISH_OFF;
		}

		break;

	default:

		if ((noti_err = notification_free(noti)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_free error [%d]", noti_err);
			err = EMAIL_ERROR_NOTI;
		}

		break;
	}

FINISH_OFF:

	if (service)
		app_control_destroy(service);

    if ((action == EMAIL_ACTION_SEND_MAIL) && noti) {
        if ((noti_err = notification_free(noti)) != NOTIFICATION_ERROR_NONE) {
            err = EMAIL_ERROR_NOTI;
		}
    }

	if ((action != EMAIL_ACTION_SEND_MAIL) && (err != EMAIL_ERROR_NONE)) {
		if (noti) {
			notification_free(noti);
			g_sending_noti_handle = NULL;
		}
	}

	if (join_zone)
		emcore_unset_join_zone(join_zone);

	if (p_mail_data)
		emstorage_free_mail(&p_mail_data, 1, NULL);

	if (account_tbl)
		emstorage_free_account(&account_tbl, 1, NULL);

	EM_SAFE_FREE(mailbox_name);

#endif  /* __FEATURE_NOTIFICATION_ENABLE__ */

	EM_DEBUG_FUNC_END("ret [%d]", err);
	return err;
}

INTERNAL_FUNC void emcore_update_notification_for_send(int account_id, int mail_id, double progress)
{
	EM_DEBUG_FUNC_BEGIN("account_id: %d, mail_id: %d, progress: %f", account_id, mail_id, progress);

	int private_id = 0;
	char vconf_private_id[MAX_PATH] = {0, };
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	SNPRINTF(vconf_private_id, sizeof(vconf_private_id), "%s/%d", VCONF_KEY_NOTI_PRIVATE_ID, account_id);
	if (vconf_get_int(vconf_private_id, &private_id) != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
	}

	EM_DEBUG_LOG("Private_id = [%d]", private_id);
	noti_err = notification_update_progress(g_sending_noti_handle, private_id, progress);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_LOG("notification_update_progress failed [0x%x]", noti_err);
	}

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emcore_show_user_message(char *multi_user_name, int id, email_action_t action, int error)
{
	EM_DEBUG_FUNC_BEGIN("id[%d], action[%d], error[%d]", id, action, error);

	int ret = false;
	int display = 0;
	emstorage_mail_tbl_t *mail_table_data = NULL;

	if ((action == EMAIL_ACTION_SEND_MAIL || action == EMAIL_ACTION_SENDING_MAIL) && error != EMAIL_ERROR_CANCELLED) {
	/*  In case email is cancelled using cancel button in Outbox there is no need to show Cancel/Retry Pop up */

		if (id <= 0) {
			EM_DEBUG_LOG("Invalid mail_id");
			return false;
		}

		if (!emstorage_get_mail_by_id(multi_user_name, id, &mail_table_data, true, NULL)) {
			EM_DEBUG_LOG("Mail not found");
			return false;
		}

		EM_DEBUG_LOG("smtp_stream : sent_end");
		if (emcore_add_notification_for_send(multi_user_name, mail_table_data->account_id, id, action, error, display) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_notification_set error");
			goto FINISH_OFF;
		}
		ret = true;
	}

FINISH_OFF:

	if (mail_table_data && !emstorage_free_mail(&mail_table_data, 1, NULL))
		EM_DEBUG_EXCEPTION("emstorage_free_mail Failed");

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}


/* storage space handling - 210709 */
int emcore_get_storage_status(void)
{
	EM_DEBUG_FUNC_BEGIN();
	int storage_status = 0, nError = 0;

#if !GLIB_CHECK_VERSION(2, 36, 0)
	g_type_init();
#endif

#ifdef STORAGE_STATUS
	nError = vconf_get_int(PS_KEY_SYSTEM_STORAGE_MOVI_STATUS,
							&storage_status);
#endif /*  STORAGE_STATUS */

	if (nError == -1) {
		EM_DEBUG_EXCEPTION("vconf_get_int Failed");
		return false;
	}
	EM_DEBUG_FUNC_END();
	return storage_status;
}



INTERNAL_FUNC int emcore_is_storage_full()
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int ret_from_storage_lib = 0;
	struct statvfs stat_result;

	ret_from_storage_lib = storage_get_internal_memory_size(&stat_result);
	if (ret_from_storage_lib < 0) {
		EM_DEBUG_EXCEPTION("ret_from_storage_lib [%d]", ret_from_storage_lib);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	} else {
		unsigned long i_free = (stat_result.f_bsize*stat_result.f_bavail) / (1024 * 1024);
		if (i_free < EMAIL_LIMITATION_FREE_SPACE) {
			EM_DEBUG_EXCEPTION("total[%lf] avail[%lf]", (double)stat_result.f_frsize*stat_result.f_blocks, (double)stat_result.f_bsize*stat_result.f_bavail);
			EM_DEBUG_EXCEPTION("Not enough free space of storage [%ld] MB.", i_free);
			err = EMAIL_ERROR_MAIL_MEMORY_FULL;
		}
	}

FINISH_OFF:
	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

int emcore_calc_mail_size(char *multi_user_name, email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, int *output_size)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list[%p], input_attachment_count[%d], output_size[%p]", input_mail_data, input_attachment_data_list, input_attachment_count, output_size);

	struct stat            st_buf;
	int                    mail_size = 0; /*  size of the plain text body and attachments */
	int                    err       = EMAIL_ERROR_NONE;
	int                    i         = 0;
    char                   *prefix_path = NULL;
    char                   real_file_path[255] = {0};

	if (!input_mail_data || (input_attachment_count && !input_attachment_data_list) || (!input_attachment_count && input_attachment_data_list) || !output_size)  {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		err = emcore_get_container_path(multi_user_name, &prefix_path);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed :[%d]", err);
			goto FINISH_OFF;
		}
	} else {
		prefix_path = strdup("");
	}

	if (input_mail_data->file_path_plain != NULL) {
        memset(real_file_path, 0x00, sizeof(real_file_path));
        SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_data->file_path_plain);

		if (stat(real_file_path, &st_buf) < 0)  {
			EM_DEBUG_EXCEPTION_SEC("input_mail_data->file_path_plain : stat(\"%s\") failed...", input_mail_data->file_path_plain);
			err = EMAIL_ERROR_FILE_NOT_FOUND;
			goto FINISH_OFF;
		}

		mail_size += st_buf.st_size;

	}

	if (input_mail_data->file_path_html != NULL) {
        memset(real_file_path, 0x00, sizeof(real_file_path));
        SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_mail_data->file_path_html);

		if (stat(real_file_path, &st_buf) < 0) {
			EM_DEBUG_EXCEPTION_SEC("input_mail_data->file_path_html : stat(\"%s\") failed...", input_mail_data->file_path_html);
			err = EMAIL_ERROR_FILE_NOT_FOUND;
			goto FINISH_OFF;
		}

		mail_size += st_buf.st_size;
	}

	for (i = 0; i < input_attachment_count; i++)  {
        memset(real_file_path, 0x00, sizeof(real_file_path));
        SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_attachment_data_list[i].attachment_path);

		if (stat(real_file_path, &st_buf) < 0)  {
			EM_DEBUG_EXCEPTION_SEC("stat(\"%s\") failed...", input_attachment_data_list[i].attachment_path);
			err = EMAIL_ERROR_FILE_NOT_FOUND;
			goto FINISH_OFF;
		}
		mail_size += st_buf.st_size;
	}

	*output_size = mail_size;

FINISH_OFF:

	EM_SAFE_FREE(prefix_path);

	EM_DEBUG_FUNC_END("mail_size [%d]", mail_size);
	return err;
}


/* parse the Full mailbox Path and Get the Alias Name of the Mailbox */
char *emcore_get_alias_of_mailbox(const char *mailbox_path)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_IF_NULL_RETURN_VALUE(mailbox_path, NULL);

	guint index = 0;
	gchar **token_list = NULL;
	gchar *mailbox = NULL, *name = NULL;
	char *converted_name;


	mailbox = g_strdup(mailbox_path);
	token_list = g_strsplit_set(mailbox, "/\\", -1);

	if (token_list == NULL) {
		EM_DEBUG_LOG("g_strsplit_set failed.");
   	     if (mailbox)
        	    g_free(mailbox);

		return NULL;
	}

	if (mailbox)
		g_free(mailbox);

	while (token_list[index] != NULL)
		index++;

	name = g_strdup(token_list[index - 1]);
	if (!name) /* prevent 27459 */ {
		return NULL;
	 }

	g_strfreev(token_list);

	converted_name = emcore_convert_mutf7_to_utf8(name);
	EM_DEBUG_LOG_DEV("converted_name:%s", converted_name);

	if (!converted_name || EM_SAFE_STRLEN(converted_name) == 0) {
        EM_SAFE_FREE(converted_name);
		converted_name = g_strdup(name);
	}

	if (name)
		g_free(name);

	EM_DEBUG_FUNC_END();
	return converted_name;
}


INTERNAL_FUNC int emcore_get_first_address(const char *input_full_address, char **output_display_name, char **output_angle_addr)
{
	EM_DEBUG_FUNC_BEGIN("input_full_address[%p], output_display_name[%p], output_angle_addr[%p]", input_full_address, output_display_name, output_angle_addr);

	int err = EMAIL_ERROR_NONE;
	char *full_address = NULL;
	char angle_addr[MAX_EMAIL_ADDRESS_LENGTH] = { 0, };
	ADDRESS *address_list = NULL;

	if (input_full_address == NULL || output_display_name == NULL || output_angle_addr == NULL) {
		err = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		goto FINISH_OFF;
	}

	/* full_address will be tainted by rfc822_parse_adrlist */
	/* So, input_full_address should be duplicated */
	full_address = EM_SAFE_STRDUP(input_full_address);

	if (full_address == NULL) {
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		goto FINISH_OFF;
	}

	rfc822_parse_adrlist(&address_list, (char*)full_address, NULL);

	if (address_list) {
		if (address_list->host)
			SNPRINTF(angle_addr, MAX_EMAIL_ADDRESS_LENGTH, "%s@%s", address_list->mailbox, address_list->host);
		else /* For SMS of eas-engine  : (example : 01012345678) */
			SNPRINTF(angle_addr, MAX_EMAIL_ADDRESS_LENGTH, "%s", address_list->mailbox);

		*output_display_name  = EM_SAFE_STRDUP(address_list->personal);
		*output_angle_addr    = EM_SAFE_STRDUP(angle_addr);

		mail_free_address(&address_list);
	}

FINISH_OFF:
	EM_SAFE_FREE(full_address);

	EM_DEBUG_FUNC_END();
	return err;
}

void emcore_fill_address_information_of_mail_tbl(char *multi_user_name, emstorage_mail_tbl_t *mail_data)
{
	EM_DEBUG_FUNC_BEGIN("mail_data [%p]", mail_data);

	char *first_alias   = NULL;
	char *first_address = NULL;
	char *recipient     = NULL;

	int i = 0;
	int vector_len = 0;
	char **vector;
	char *recipient_self = NULL;

	email_account_t *ref_account = NULL;

	ref_account = emcore_get_account_reference(multi_user_name, mail_data->account_id, false);
	if (!ref_account)  {
		EM_DEBUG_LOG("emcore_get_account_reference failed [%d]", mail_data->account_id);
	}

	EM_DEBUG_LOG_SEC("full_address_from [%s]", mail_data->full_address_from);

	/*  sender alias & address */
	if (emcore_get_first_address(mail_data->full_address_from, &first_alias, &first_address) == true) {
		if (first_alias == NULL) {
			mail_data->alias_sender = EM_SAFE_STRDUP(first_address);
		} else {
			mail_data->alias_sender = first_alias;
			first_alias = NULL;
		}
		mail_data->email_address_sender = first_address;
		first_address = NULL;
	}

	EM_DEBUG_LOG_SEC("full_address_from [%s]", mail_data->full_address_from);

	/*  recipient alias & address */
	if (mail_data->full_address_to != NULL)
		recipient = mail_data->full_address_to;
	else if (mail_data->full_address_cc != NULL)
		recipient = mail_data->full_address_cc;
	else if (mail_data->full_address_bcc != NULL)
		recipient = mail_data->full_address_bcc;

	/* if full_address_to contains account address, set account address to email_address_recipient */
	if (ref_account && mail_data->full_address_to &&
			g_strrstr(mail_data->full_address_to, ref_account->user_email_address)) {
		vector = g_strsplit_set(mail_data->full_address_to, ";", -1);
		vector_len = g_strv_length(vector);

		for (i = 0; i < vector_len; i++) {
			if (g_strrstr(vector[i], ref_account->user_email_address)) {
				recipient_self = EM_SAFE_STRDUP(vector[i]);
				break;
			}
		}
		g_strfreev(vector);

		if (recipient_self) {
			recipient = recipient_self;
		}
	}

	if (emcore_get_first_address(recipient, &first_alias, &first_address) == true) {
		if (first_alias == NULL)
			mail_data->alias_recipient = EM_SAFE_STRDUP(first_address);
		else
			mail_data->alias_recipient = first_alias;

		mail_data->email_address_recipient = first_address;
	}

	if (ref_account) {
		emcore_free_account(ref_account);
		EM_SAFE_FREE(ref_account);
	}

	EM_SAFE_FREE(recipient_self);
	EM_DEBUG_FUNC_END();
}

struct email_attribute_info {
	email_mail_attribute_type        attribute_type;
	char                            *attribute_name;
	email_mail_attribute_value_type  attribute_value_type;
};

static struct email_attribute_info _mail_attribute_info_array[] = {
		{EMAIL_MAIL_ATTRIBUTE_MAIL_ID, "mail_id", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID, "account_id", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_MAILBOX_ID, "mailbox_id", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME, "mailbox_name", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE, "mailbox_type", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_SUBJECT, "subject", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_DATE_TIME, "date_time", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_TIME},
		{EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_STATUS, "server_mail_status", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_SERVER_MAILBOX_NAME, "server_mailbox_name", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_SERVER_MAIL_ID, "server_mail_id", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_MESSAGE_ID, "message_id", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_REFERENCE_MAIL_ID, "reference_mail_id", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_FROM, "full_address_from", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_TO, "full_address_to", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_CC, "full_address_cc", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_BCC, "full_address_bcc", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_BODY_DOWNLOAD_STATUS, "body_download_status", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_MAIL_SIZE, "mail_size", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_FILE_PATH_PLAIN, "file_path_plain", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_FILE_PATH_HTML, "file_path_html", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_FILE_SIZE, "file_size", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD, "flags_seen_field", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD, "flags_deleted_field", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER },
		{EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD, "flags_flagged_field", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_FLAGS_ANSWERED_FIELD, "flags_answered_field", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_FLAGS_RECENT_FIELD, "flags_recent_field", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_FLAGS_DRAFT_FIELD, "flags_draft_field", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_FLAGS_FORWARDED_FIELD, "flags_forwarded_field", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_DRM_STATUS, "drm_status", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_PRIORITY, "priority", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS, "save_status", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_LOCK_STATUS, "lock_status", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_REPORT_STATUS, "report_status", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_ATTACHMENT_COUNT, "attachment_count", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_INLINE_CONTENT_COUNT, "inline_content_count", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_THREAD_ID, "thread_id", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_THREAD_ITEM_COUNT, "thread_item_count", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_PREVIEW_TEXT, "preview_text", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_MEETING_REQUEST_STATUS, "meeting_request_status", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_MESSAGE_CLASS, "message_class", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_DIGEST_TYPE, "digest_type", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_SMIME_TYPE, "smime_type", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_SCHEDULED_SENDING_TIME, "scheduled_sending_time", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_TIME},
		{EMAIL_MAIL_ATTRIBUTE_REMAINING_RESEND_TIMES, "remaining_resend_times", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_TAG_ID, "tag_id", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_REPLIED_TIME, "replied_time", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_TIME},
		{EMAIL_MAIL_ATTRIBUTE_FORWARDED_TIME, "forwarded_time", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_TIME},
		{EMAIL_MAIL_ATTRIBUTE_RECIPIENT_ADDRESS, "email_recipient_address", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_STRING},
		{EMAIL_MAIL_ATTRIBUTE_EAS_DATA_LENGTH_TYPE, "eas_data_length", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_INTEGER},
		{EMAIL_MAIL_ATTRIBUTE_EAS_DATA_TYPE, "eas_data", EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_BINARY}
};

INTERNAL_FUNC char* emcore_get_mail_field_name_by_attribute_type(email_mail_attribute_type input_attribute_type)
{
	EM_DEBUG_FUNC_BEGIN("input_attribute_type [%d]", input_attribute_type);

	if (input_attribute_type > EMAIL_MAIL_ATTRIBUTE_EAS_DATA_TYPE || input_attribute_type < 0) {
		EM_DEBUG_EXCEPTION("Invalid input_attribute_type [%d]", input_attribute_type);
		return NULL;
	}

	EM_DEBUG_LOG_DEV("return [%s]", _mail_attribute_info_array[input_attribute_type].attribute_name);
	return _mail_attribute_info_array[input_attribute_type].attribute_name;
}

INTERNAL_FUNC int emcore_get_attribute_type_by_mail_field_name(char *input_mail_field_name, email_mail_attribute_type *output_mail_attribute_type)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_field_name [%p], output_mail_attribute_type [%p]", input_mail_field_name, output_mail_attribute_type);
	int i = 0;
	int err = EMAIL_ERROR_DATA_NOT_FOUND;

	if (!input_mail_field_name || !output_mail_attribute_type) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		return EMAIL_ERROR_INVALID_PARAM;
	}

	for (i = 0; i < EMAIL_MAIL_ATTRIBUTE_END; i++) {
		if (EM_SAFE_STRCMP(input_mail_field_name, _mail_attribute_info_array[i].attribute_name) == 0) {
			*output_mail_attribute_type = i;
			err = EMAIL_ERROR_NONE;
			break;
		}
	}

	EM_DEBUG_FUNC_END("found mail attribute type [%d]", (int)*output_mail_attribute_type);
	return err;
}

INTERNAL_FUNC int emcore_get_mail_attribute_value_type(email_mail_attribute_type input_attribute_type, email_mail_attribute_value_type *output_value_type)
{
	EM_DEBUG_FUNC_BEGIN("input_attribute_type[%d] output_value_type[%p]", input_attribute_type, output_value_type);
	int err = EMAIL_ERROR_NONE;
	int i = 0;
	email_mail_attribute_value_type value_type = EMAIL_MAIL_ATTRIBUTE_VALUE_TYPE_NONE;

	if (output_value_type == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	for (i = 0; i < EMAIL_MAIL_ATTRIBUTE_END; i++) {
		if (input_attribute_type == _mail_attribute_info_array[i].attribute_type) {
			value_type = _mail_attribute_info_array[i].attribute_value_type;
			break;
		}
	}

	*output_value_type = value_type;

	EM_DEBUG_LOG("value_type [%d]", value_type);

FINISH_OFF:
	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

#ifdef __FEATURE_BODY_SEARCH__
int emcore_strip_mail_body_from_file(char *multi_user_name, emstorage_mail_tbl_t *mail, char **stripped_text, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail[%p]", mail);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	char *buf = NULL;
	char *encoding_type = NULL;
	char *utf8_encoded_string = NULL;
    char *prefix_path = NULL;
    char real_html_path[255] = {0};
    char real_plain_path[255] = {0};
	gsize byte_read = 0;
	gsize byte_written = 0;
	GError *glib_error = NULL;
	FILE *fp_plain = NULL;
	struct stat  st_buf;

	if (!mail) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		err = emcore_get_container_path(multi_user_name, &prefix_path);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed :[%d]", err);
			goto FINISH_OFF;
		}
	} else {
		prefix_path = strdup("");
	}

	/* read file to buf & strip if html text */
	if (mail->file_path_html) {
		char result_buffer[MAX_PREVIEW_TEXT_LENGTH] = { 0, };

		if (mail->preview_text) {
			*stripped_text = EM_SAFE_STRDUP(mail->preview_text);
			ret = true;
			goto FINISH_OFF;
		}

        SNPRINTF(real_html_path, sizeof(real_html_path), "%s%s", prefix_path, mail->file_path_html);

		if ((err = em_get_encoding_type_from_file_path(mail->file_path_plain, &encoding_type)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_get_encoding_type_from_file_path failed [%d]", err);
			goto FINISH_OFF;
		}

		if ((err = emcore_strip_HTML_tag(real_html_path, encoding_type, result_buffer, MAX_PREVIEW_TEXT_LENGTH)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_strip_HTML_tag failed");
			goto FINISH_OFF;
		}

		*stripped_text = EM_SAFE_STRDUP(result_buffer);
	}

	if (!buf && mail->file_path_plain) {
        SNPRINTF(real_plain_path, sizeof(real_plain_path), "%s%s", prefix_path, mail->file_path_plain);

		if ((err = em_get_encoding_type_from_file_path(mail->file_path_plain, &encoding_type)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_get_encoding_type_from_file_path failed [%d]", err);
			goto FINISH_OFF;
		}

		memset(&st_buf, 0, sizeof(struct stat));
		if (stat(real_plain_path, &st_buf) < 0) {
			EM_DEBUG_EXCEPTION_SEC("stat(\"%s\") failed...", real_plain_path);
			err = EMAIL_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}

		err = em_fopen(real_plain_path, "r", &fp_plain);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION_SEC("em_fopen failed [%s]", real_plain_path);
			goto FINISH_OFF;
		}

		if (S_ISREG(st_buf.st_mode) && st_buf.st_size == 0) {
			EM_DEBUG_LOG("text_file is empty size");
			err = EMAIL_ERROR_EMPTY_FILE;
			goto FINISH_OFF;
		}

		if (!(buf = (char*)em_malloc(sizeof(char)*(st_buf.st_size + 1)))) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			goto FINISH_OFF;
		}

		byte_read = fread(buf, sizeof(char), st_buf.st_size, fp_plain);

		if (byte_read <= 0) {
			EM_SAFE_FREE(buf);
			err = EMAIL_ERROR_NULL_VALUE;
			if (ferror(fp_plain)) {
				EM_DEBUG_EXCEPTION_SEC("fread failed [%s]", real_plain_path);
				err = EMAIL_ERROR_SYSTEM_FAILURE;
			}
			goto FINISH_OFF;
		}
		reg_replace(buf, CR_STRING, " ");
		reg_replace(buf, LF_STRING, " ");
		reg_replace(buf, TAB_STRING, " ");
	}

	if (buf) {
		em_trim_left(buf);
		if (encoding_type && strcasecmp(encoding_type, "UTF-8") != 0) {
			EM_DEBUG_LOG("encoding_type [%s]", encoding_type);

			if (strcasecmp(encoding_type, "ks_c_5601-1987") == 0 ||
					strcasecmp(encoding_type, "ksc5601") == 0 ||
					strcasecmp(encoding_type, "cp949") == 0) {
				EM_SAFE_FREE(encoding_type);
				encoding_type = EM_SAFE_STRDUP("EUC-KR");
			}

			utf8_encoded_string = (char*)g_convert(buf, -1, "UTF-8", encoding_type, &byte_read, &byte_written, &glib_error);

			if (utf8_encoded_string == NULL) {
				if (!g_error_matches(glib_error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE)) {
		            EM_SAFE_FREE(*stripped_text);
					*stripped_text = EM_SAFE_STRDUP(buf);
					goto FINISH_OFF;
				}
				EM_DEBUG_LOG("Extract the preview text, again");

				utf8_encoded_string = (char *)g_convert(buf, -1, "UTF-8", encoding_type, &byte_read, &byte_written, &glib_error);
				if (utf8_encoded_string == NULL) {
					EM_SAFE_FREE(*stripped_text);
					*stripped_text = EM_SAFE_STRDUP(buf);
					goto FINISH_OFF;
				}
			}
			EM_SAFE_FREE(buf);

            EM_SAFE_FREE(*stripped_text);
			*stripped_text = EM_SAFE_STRDUP(utf8_encoded_string);
		} else {
            EM_SAFE_FREE(*stripped_text);
			*stripped_text = EM_SAFE_STRDUP(buf);
		}
	}

	ret = true;

FINISH_OFF:

	if (err_code)
		*err_code = err;

	EM_SAFE_FREE(buf);
	EM_SAFE_FREE(prefix_path);
	EM_SAFE_FREE(encoding_type);
	EM_SAFE_FREE(utf8_encoded_string);

	if (fp_plain != NULL)
		fclose(fp_plain);

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
#endif


int emcore_get_preview_text_from_file(char *multi_user_name, const char *input_plain_path, const char *input_html_path, int input_preview_buffer_length, char **output_preview_buffer)
{
	EM_DEBUG_FUNC_BEGIN("input_plain_path[%p], input_html_path[%p], input_preview_buffer_length [%d], output_preview_buffer[%p]", input_plain_path, input_html_path, input_preview_buffer_length, output_preview_buffer);

	int          err = EMAIL_ERROR_NONE;
	gsize        byte_read = 0;
	gsize        byte_written = 0;
	int          local_preview_buffer_length = 0;
	char        *local_preview_text = NULL;
	char        *encoding_type = NULL;
	char        *utf8_encoded_string = NULL;
	FILE        *fp_plain = NULL;
	GError      *glib_error = NULL;
	struct stat  st_buf;
    char        *prefix_path = NULL;
    char         real_file_path[255] = {0};

	if (!output_preview_buffer) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	local_preview_buffer_length = input_preview_buffer_length * 2;

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		err = emcore_get_container_path(multi_user_name, &prefix_path);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_container_path failed :[%d]", err);
			goto FINISH_OFF;
		}
	} else {
		prefix_path = strdup("");
	}

	if (input_html_path) { /*prevent 26249*/
		/*	get preview text from html file */
		char result_buffer[MAX_PREVIEW_TEXT_LENGTH] = { 0, };

        memset(real_file_path, 0x00, sizeof(real_file_path));
        SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_html_path);

		if ((err = em_get_encoding_type_from_file_path(input_html_path, &encoding_type)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_get_encoding_type_from_file_path failed [%d]", err);
			goto FINISH_OFF;
		}

		if ((err = emcore_strip_HTML_tag(real_file_path, encoding_type, result_buffer, MAX_PREVIEW_TEXT_LENGTH)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_strip_HTML_tag failed");
			goto FINISH_OFF;
		}
		local_preview_text = EM_SAFE_STRDUP(result_buffer);
	}

	if (!local_preview_text && input_plain_path) { /*prevent 26249*/

        memset(real_file_path, 0x00, sizeof(real_file_path));
        SNPRINTF(real_file_path, sizeof(real_file_path), "%s%s", prefix_path, input_plain_path);

		/*  get preview text from plain text file */
		if ((err = em_get_encoding_type_from_file_path(input_plain_path, &encoding_type)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_get_encoding_type_from_file_path failed [%d]", err);
			goto FINISH_OFF;
		}

		memset(&st_buf, 0, sizeof(struct stat));
		if (stat(real_file_path, &st_buf) < 0)  {
			EM_DEBUG_EXCEPTION("stat(\"%s\") failed...", input_plain_path);
			err = EMAIL_ERROR_INVALID_MAIL;
			goto FINISH_OFF;
		}

		err = em_fopen(real_file_path, "r", &fp_plain);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("em_fopen failed [%s]", input_plain_path);
			goto FINISH_OFF;
		}

		if (S_ISREG(st_buf.st_mode) && st_buf.st_size == 0) {
			EM_DEBUG_LOG("input_text_file is empty size");
			err = EMAIL_ERROR_EMPTY_FILE;
			goto FINISH_OFF;
		}

		if (!(local_preview_text = (char*)em_malloc(sizeof(char) * local_preview_buffer_length))) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			goto FINISH_OFF;
		}

		byte_read = fread(local_preview_text, sizeof(char), local_preview_buffer_length - 1, fp_plain);

		if (byte_read <= 0) { /*prevent 26249*/
			EM_SAFE_FREE(local_preview_text);
			err = EMAIL_ERROR_NULL_VALUE;
			if (ferror(fp_plain)) {
				EM_DEBUG_EXCEPTION("fread failed [%s]", input_plain_path);
				err = EMAIL_ERROR_SYSTEM_FAILURE;
			}
			goto FINISH_OFF;
		}
		reg_replace(local_preview_text, CR_STRING, " ");
		reg_replace(local_preview_text, LF_STRING, " ");
		reg_replace(local_preview_text, TAB_STRING, " ");

		em_trim_left(local_preview_text);

		if (encoding_type && strcasecmp(encoding_type, "UTF-8") != 0) {
			EM_DEBUG_LOG("encoding_type [%s]", encoding_type);

			if (strcasecmp(encoding_type, "ks_c_5601-1987") == 0 ||
					strcasecmp(encoding_type, "ksc5601") == 0 ||
					strcasecmp(encoding_type, "cp949") == 0) {
				EM_SAFE_FREE(encoding_type);
				encoding_type = EM_SAFE_STRDUP("EUC-KR");
			}

			utf8_encoded_string = (char *)g_convert(local_preview_text, -1, "UTF-8", encoding_type, &byte_read, &byte_written, &glib_error);

			if (utf8_encoded_string == NULL) {
				if (!g_error_matches(glib_error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE)) {
					goto FINISH_OFF;
				}
				EM_DEBUG_LOG("Extract the preview text, again");

				utf8_encoded_string = (char *)g_convert(local_preview_text, -1, "UTF-8", encoding_type, &byte_read, &byte_written, &glib_error);
				if (utf8_encoded_string == NULL) {
					EM_DEBUG_LOG("g_convert fail, again");
					goto FINISH_OFF;
				}
			}
			EM_SAFE_FREE(local_preview_text);
			local_preview_text = utf8_encoded_string;
		}
	}

FINISH_OFF:
/*
	if (local_preview_text != NULL)
		*output_preview_buffer = EM_SAFE_STRDUP(local_preview_text);
*/
	if (local_preview_text != NULL) {
		EM_SAFE_FREE(*output_preview_buffer); /* valgrind */
		*output_preview_buffer = EM_SAFE_STRDUP(local_preview_text);
	}

	EM_SAFE_FREE(local_preview_text);
	EM_SAFE_FREE(encoding_type);
	EM_SAFE_FREE(prefix_path);

	if (fp_plain != NULL)
		fclose(fp_plain);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_add_transaction_info(int mail_id, int handle , int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], handle[%d]", mail_id, handle);

	int ret = false;
	int err = EMAIL_ERROR_NONE ;
	em_transaction_info_type_t  *pTransinfo = NULL ;
	em_transaction_info_type_t	*pTemp = NULL;

	EM_DEBUG_LOG("g_transaction_info_list[%p]", g_transaction_info_list);
	pTransinfo = g_transaction_info_list ;

	if (!(pTemp = em_malloc(sizeof(em_transaction_info_type_t))))  {
		EM_DEBUG_EXCEPTION("malloc failed...");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	pTemp->mail_id = mail_id ;
	pTemp->handle = handle;

	if (!pTransinfo) {
		pTransinfo = pTemp ;
		g_transaction_info_list = pTransinfo ;
	} else {
		while (pTransinfo->next)
			pTransinfo = pTransinfo->next;
		pTransinfo->next = pTemp;
	}
	ret = true ;

FINISH_OFF:

	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END("g_transaction_info_list[%p]", g_transaction_info_list);
	return ret;
}

INTERNAL_FUNC int emcore_get_handle_by_mailId_from_transaction_info(int mail_id, int *pHandle)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d], handle[%p]", mail_id, pHandle);

	int ret = false;
	em_transaction_info_type_t  *pTransinfo = NULL ;

	if (g_transaction_info_list == NULL) {
		EM_DEBUG_EXCEPTION("g_transaction_info_list NULL");
		return false;
	}
	pTransinfo = g_transaction_info_list;

	do {
		EM_DEBUG_LOG("pTransinfo->mail_id[%d]", pTransinfo->mail_id);
		if (pTransinfo->mail_id == mail_id) {
			*pHandle = pTransinfo->handle;
			ret = true;
			EM_DEBUG_LOG("*pHandle[%d]", *pHandle);
			break;
		} else
			pTransinfo = pTransinfo->next ;
	} while (pTransinfo);
	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_delete_transaction_info_by_mailId(int mail_id)
{
	EM_DEBUG_FUNC_BEGIN("mail_id[%d]", mail_id);

	em_transaction_info_type_t  *pTransinfo ;
	em_transaction_info_type_t *pTemp = NULL;

	if (g_transaction_info_list == NULL) {
		EM_DEBUG_EXCEPTION("g_transaction_info_list NULL");
		return false;
	}
	pTransinfo = g_transaction_info_list;

	EM_DEBUG_LOG("pTransinfo[%p]", pTransinfo);

	do {
		EM_DEBUG_LOG("pTransinfo->mail_id[%d]", pTransinfo->mail_id);
		if (pTransinfo->mail_id == mail_id) {
			pTemp = pTransinfo->next ;
			if (!pTemp) {
				EM_SAFE_FREE(pTransinfo) ;
				g_transaction_info_list = NULL;
			} else {
				pTransinfo->mail_id = pTransinfo->next->mail_id;
				pTransinfo->handle = pTransinfo->next->handle;
				pTransinfo->next = pTransinfo->next->next;

				EM_SAFE_FREE(pTemp);
			}
			break;
		} else {
			pTransinfo = pTransinfo->next ;
		}

	} while (pTransinfo);
	EM_DEBUG_FUNC_END();
	return true;
}


#include <regex.h>

INTERNAL_FUNC int reg_replace(char *input_source_text, char *input_old_pattern_string, char *input_new_string)
{
	int         error_code = EMAIL_ERROR_NONE;
	char       *pos = NULL;
	int         so, n, nmatch, source_text_length, n_count = 1;
	regmatch_t *pmatch = NULL;
	regex_t     reg_pattern;

	if (!input_source_text || !input_old_pattern_string || !input_new_string) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		error_code = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	source_text_length = EM_SAFE_STRLEN(input_source_text);

	if (regcomp(&reg_pattern, input_old_pattern_string, REG_EXTENDED | REG_ICASE) != 0) {
		EM_DEBUG_EXCEPTION("regcomp failed");
		goto FINISH_OFF;
	}

	nmatch = reg_pattern.re_nsub + 1;

	EM_DEBUG_LOG("nmatch [%d]", nmatch);

	if (nmatch < 1) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_DATA");
		error_code = EMAIL_ERROR_INVALID_DATA;
		goto FINISH_OFF;
	}

	pmatch = (regmatch_t *)em_malloc(sizeof(regmatch_t) * nmatch);

	if (pmatch == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		error_code = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	for (pos = input_new_string; *pos ; pos++) {
		if (*pos == '\\' && *(pos + 1) > '0' && *(pos + 1) <= '9') {

			so = pmatch[*(pos + 1) - 48].rm_so;
			n  = pmatch[*(pos + 1) - 48].rm_eo - so;

			EM_DEBUG_LOG("so [%d], n [%d]", so, n);

			if (so < 0 || EM_SAFE_STRLEN(input_new_string) + n - 1 > source_text_length)
				break;

			memmove(pos + n, pos + 2, EM_SAFE_STRLEN(pos) - 1);
			memmove(pos, input_source_text + so, n);
			pos = pos + n - 2;
		}
	}

	for (pos = input_source_text; !regexec(&reg_pattern, pos, 1, pmatch, 0); ) {
		n = pmatch[0].rm_eo - pmatch[0].rm_so;
		pos += pmatch[0].rm_so;
		memmove(pos + EM_SAFE_STRLEN(input_new_string), pos + n, EM_SAFE_STRLEN(pos) - n + 1);
		memmove(pos, input_new_string, EM_SAFE_STRLEN(input_new_string));
		pos += EM_SAFE_STRLEN(input_new_string);
		n_count++;
	}

FINISH_OFF:

	EM_SAFE_FREE(pmatch);
	regfree(&reg_pattern);

	EM_DEBUG_FUNC_END("error_code [%d]", error_code);
	return error_code;
}


INTERNAL_FUNC char *reg_replace_new(char *input_source_text, char *input_old_pattern_string, char *input_new_string)
{
	char *replaced_str = NULL;
	GRegex *regex = NULL;
	GError *error = NULL;

	if (input_source_text == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		return NULL;
	}

	regex = g_regex_new(input_old_pattern_string, G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, &error);
	if (!regex) {
		EM_DEBUG_LOG("g_regex_new failed");
		goto FINISH_OFF;
	}

	replaced_str = g_regex_replace_literal(regex,
											input_source_text,
											strlen(input_source_text),
											0,
											input_new_string,
											0,
											&error);
	if (replaced_str == NULL) {
		EM_DEBUG_EXCEPTION("g_regex_replace_literal failed : [%s][%d]", error->message, error->code);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (regex) g_regex_unref(regex);

	return replaced_str;
}


#include <libxml/HTMLparser.h>

static void emcore_get_content_string(xmlNode *input_node, char *input_result_buffer, int input_result_buffer_length, int *input_exit_flag)
{
    xmlNode *cur_node = NULL;
    char    *temp_content_string = NULL;

    if (input_node == NULL || input_result_buffer == NULL || input_exit_flag == NULL) {
    	EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
    	goto FINISH_OFF;
    }

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->name && (strcasecmp((const char *)cur_node->name, "head") == 0 || strcasecmp((const char *)cur_node->name, "title") == 0))
			continue;

		if (cur_node->name && (strcasecmp((const char *)cur_node->name, "BR") == 0)) {
			if (EM_SAFE_STRLEN(input_result_buffer) + 1 >= input_result_buffer_length) {
				*input_exit_flag = 1;
				break;
			} else {
				EM_SAFE_STRCAT(input_result_buffer, " ");
				continue;
			}
		}

		if (cur_node->type == XML_TEXT_NODE && cur_node->content) {
			if ((EM_SAFE_STRLEN(cur_node->content) + EM_SAFE_STRLEN(input_result_buffer)) >= input_result_buffer_length) {
				int len1 = EM_SAFE_STRLEN(input_result_buffer);

				if (len1 >= 0 && len1 < input_result_buffer_length) {
					int remain_size = input_result_buffer_length - len1 - 2;
					EM_DEBUG_LOG("remain_size : [%d], len1 : [%d]", remain_size, len1);
					char *remain_str = NULL;

					if (remain_size > 0)
						remain_str = g_strndup((char *)(cur_node->content), remain_size);

					if (remain_str) {
						char *replaced_string = NULL;
						replaced_string = reg_replace_new(remain_str, "[ \t\r\n\v\f]+", " ");
						EM_SAFE_STRCAT(input_result_buffer, replaced_string);
						EM_SAFE_FREE(replaced_string);
						free(remain_str);
					}
				}

				*input_exit_flag = 1;
				break;
			}

			temp_content_string = EM_SAFE_STRDUP((const char *)cur_node->content);
			if (temp_content_string) {
				char *replaced_string = NULL;
				replaced_string = reg_replace_new(temp_content_string, "[ \t\r\n\v\f]+", " ");
				EM_SAFE_STRCAT(input_result_buffer, replaced_string);
				EM_SAFE_FREE(replaced_string);
				free(temp_content_string);
			}
		}

		if (cur_node->children)
			emcore_get_content_string(cur_node->children, input_result_buffer, input_result_buffer_length, input_exit_flag);

		if (*input_exit_flag == 1)
			break;
    }

FINISH_OFF:

	return;
}

static void emcore_default_xml_error_handler(void *ctx, const char *msg, ...)
{
	EM_DEBUG_EXCEPTION("htmlReadFile failed");
}

int emcore_strip_HTML_tag(const char *input_html_file_path, char *input_encoding_type, char *output_result_buffer, int input_result_buffer_legnth)
{
	EM_DEBUG_FUNC_BEGIN("input_html_file_path[%p] input_encoding_type[%s] output_result_buffer[%p] input_result_buffer_legnth[%d]", input_html_file_path, input_encoding_type, output_result_buffer, input_result_buffer_legnth);
	int err = EMAIL_ERROR_NONE;
	int exit_flag = 0;
	char *result_string = NULL;
	xmlNode *root_element = NULL;
	htmlDocPtr result_html_document = NULL;
	char *encoding_type = NULL;
	char *utf_charset = "UTF-8";
	char *euc_kr_charset = "euc-kr";

	if (input_html_file_path == NULL || output_result_buffer == NULL || input_result_buffer_legnth == 0) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	result_string = em_malloc(sizeof(char) * input_result_buffer_legnth);
	if (result_string == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_OUT_OF_MEMORY");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (EM_SAFE_STRCMP(input_encoding_type, "unknown") == 0)
		encoding_type = utf_charset;
	else if (input_encoding_type && (strcasecmp(input_encoding_type, "ks_c_5601-1987") == 0 ||
		strcasecmp(input_encoding_type, "ksc5601") == 0 ||
		strcasecmp(input_encoding_type, "cp949") == 0))
		encoding_type = euc_kr_charset;
	else
		encoding_type = input_encoding_type;

	EM_DEBUG_LOG("encoding_type [%s]", encoding_type);

	LIBXML_TEST_VERSION
	xmlSetGenericErrorFunc(NULL, emcore_default_xml_error_handler);
	result_html_document = htmlReadFile(input_html_file_path, encoding_type, HTML_PARSE_RECOVER | HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR | HTML_PARSE_NONET | HTML_PARSE_NOBLANKS);
	if (result_html_document == NULL) {
		EM_DEBUG_EXCEPTION("htmlReadFile failed");
		err = EMAIL_ERROR_ON_PARSING;
		goto FINISH_OFF;
	}

	root_element = xmlDocGetRootElement(result_html_document);

	emcore_get_content_string(root_element, result_string, input_result_buffer_legnth, &exit_flag);
	char *replaced_string = reg_replace_new(result_string, "[ \t\r\n\v\f]+", " ");
	EM_SAFE_STRCPY(output_result_buffer, replaced_string);
	EM_SAFE_FREE(replaced_string);
	em_trim_left(output_result_buffer);

FINISH_OFF:
	if (result_html_document) {
		xmlFreeDoc(result_html_document);
	}

	EM_SAFE_FREE(result_string);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_send_noti_for_new_mail(int account_id, char *mailbox_name, char *subject, char *from, char *uid, char *datetime)
{
	EM_DEBUG_FUNC_BEGIN_SEC("mailbox_name(%s) subject(%s), from(%s), uid(%s), datetime(%s)", mailbox_name, subject, from, uid, datetime);
	int error_code = EMAIL_ERROR_NONE;
	int param_length = 0;
	char *param_string = NULL;

	if (mailbox_name == NULL || subject == NULL || from == NULL || uid == NULL || datetime == NULL) {
		error_code = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("Invalid parameter, mailbox_name(%p), subject(%p), from(%p), uid(%p), datetime(%p)", mailbox_name, subject, from, uid, datetime);
		goto FINISH_OFF;
	}

	param_length = strlen(mailbox_name) + strlen(subject) + strlen(from) + strlen(uid) + strlen(datetime) + 5; /*prevent 34358*/

	param_string = em_malloc(param_length);
	if (param_string == NULL) {
		error_code = EMAIL_ERROR_OUT_OF_MEMORY;
		EM_DEBUG_EXCEPTION("Memory allocation for 'param_string' is failed");
		goto FINISH_OFF;
	}

	memset(param_string, 0x00, param_length);

	SNPRINTF(param_string, param_length, "%s%c%s%c%s%c%s%c%s", mailbox_name, 0x01, subject, 0x01, from, 0x01, uid, 0x01, datetime);

	if (emcore_notify_network_event(NOTI_DOWNLOAD_NEW_MAIL, account_id, param_string, 0, 0) == 0) {	/*  failed */
		error_code = EMAIL_ERROR_UNKNOWN;
		EM_DEBUG_EXCEPTION("emcore_notify_network_eventis failed");
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_SAFE_FREE(param_string);
	EM_DEBUG_FUNC_END();
	return error_code;
}

#define MAX_TITLE_LENGTH 1024

#ifdef __FEATURE_DRIVING_MODE__
int convert_app_err_to_email_err(int app_error)
{
	int err = EMAIL_ERROR_NONE;

	switch (app_error) {
	case APP_CONTROL_ERROR_NONE:
		err = EMAIL_ERROR_NONE;
		break;
	case APP_CONTROL_ERROR_INVALID_PARAMETER:
		err = EMAIL_ERROR_INVALID_PARAM;
		break;
	case APP_CONTROL_ERROR_OUT_OF_MEMORY:
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		break;
	default:
		err = EMAIL_ERROR_UNKNOWN;
		break;
	}

	return err;
}

INTERNAL_FUNC int emcore_start_driving_mode(char *multi_user_name, int mail_id)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = APP_CONTROL_ERROR_NONE;
	int tts_enable = 0;
	char string_mail[10] = { 0 };
	emstorage_mail_tbl_t *p_mail_data = NULL;
	app_control_h service = NULL;

	if (vconf_get_bool(VCONFKEY_SETAPPL_DRIVINGMODE_DRIVINGMODE, &tts_enable) != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("Driving Mode : [%d]", tts_enable);

	if (!tts_enable)
		goto FINISH_OFF;


	if (vconf_get_bool(VCONFKEY_SETAPPL_DRIVINGMODE_NEWEMAILS, &tts_enable) != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("New emails of driving Mode : [%d]", tts_enable);

	if (!tts_enable)
		goto FINISH_OFF;

	if (!emstorage_get_mail_by_id(multi_user_name, mail_id, &p_mail_data, false, NULL)) {
		EM_DEBUG_EXCEPTION("emstorage_get_mail_by_id failed");
		goto FINISH_OFF;
	}

	if (p_mail_data->tag_id >= 0)
		goto FINISH_OFF;

	SNPRINTF(string_mail, sizeof(string_mail), "%d", mail_id);

	err = app_control_create(&service);
	if (err != APP_CONTROL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("app_control_create failed : [%d]", err);
		goto FINISH_OFF;
	}

	err = app_control_set_app_id(service, "org.tizen.email-tts-play");
	if (err != APP_CONTROL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("app_control_set_app_id failed : [%d]", err);
		goto FINISH_OFF;
	}

	err = app_control_add_extra_data(service, "tts_email_id", string_mail);
	if (err != APP_CONTROL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("app_control_add_extra_data failed : [%d]", err);
		goto FINISH_OFF;
	}

	err = app_control_send_launch_request(service, NULL, NULL);
	if (err != APP_CONTROL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("app_control_send_launch_request failed : [%d]", err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (service)
		app_control_destroy(service);

	if (p_mail_data)
		emstorage_free_mail(&p_mail_data, 1, NULL);

	EM_DEBUG_FUNC_END();
	return convert_app_err_to_email_err(err);
}
#endif /* __FEATURE_DRIVING_MODE__ */

INTERNAL_FUNC int emcore_clear_notifications(char *multi_user_name, int account_id)
{
	int account_count = 0, i;
	emstorage_account_tbl_t *account_list;
	int error_code = EMAIL_ERROR_NONE;

	if (account_id == ALL_ACCOUNT) {
		if (!emstorage_get_account_list(multi_user_name, &account_count, &account_list, true, false, &error_code)) {
			EM_DEBUG_EXCEPTION("emstorage_get_account_list failed");
			goto FINISH_OFF;
		}

		for (i = 0; i < account_count; i++) {
			error_code = emcore_delete_notification_by_account(multi_user_name, account_list[i].account_id, true);
			if (error_code != EMAIL_ERROR_NONE)
				EM_DEBUG_EXCEPTION("emcore_delete_notification_by_account failed");
		}
	} else {
		error_code = emcore_delete_notification_by_account(multi_user_name, account_id, true);
		if (error_code != EMAIL_ERROR_NONE)
			EM_DEBUG_EXCEPTION("emcore_delete_notification_by_account failed");
	}

	if (!emstorage_update_save_status(multi_user_name, account_id, &error_code)) {
	       EM_DEBUG_EXCEPTION("emstorage_update_save_status failed : [%d]", error_code);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (account_count) {
		emstorage_free_account(&account_list, account_count, NULL);
	}

	EM_DEBUG_FUNC_END("return[%d]", error_code);
	return error_code;
}

#define EAS_EXECUTABLE_PATH "/usr/bin/eas-engine"

INTERNAL_FUNC int emcore_delete_notification_by_account(char *multi_user_name, int account_id, int with_noti_tray)
{
	EM_DEBUG_FUNC_BEGIN("account_id [%d]", account_id);
	int error_code = EMAIL_ERROR_NONE;
	int private_id = 0;
	char vconf_private_id[MAX_PATH] = {0, };

	SNPRINTF(vconf_private_id, sizeof(vconf_private_id), "%s/%d", VCONF_KEY_NOTI_PRIVATE_ID, account_id);
	if (vconf_get_int(vconf_private_id, &private_id) != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
	}
#ifdef __FEATURE_NOTIFICATION_ENABLE__
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	if ((noti_err = notification_delete_by_priv_id(NATIVE_EMAIL_APPLICATION_PKG, NOTIFICATION_TYPE_NOTI, private_id)) != NOTIFICATION_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("notification_delete_by_priv_id failed [%d]", noti_err);
		error_code = EMAIL_ERROR_NOTI;
	}

	if (with_noti_tray) {
		if ((noti_err = notification_delete_by_priv_id(NATIVE_EMAIL_APPLICATION_PKG, NOTIFICATION_TYPE_ONGOING, private_id)) != NOTIFICATION_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("notification_delete_by_priv_id failed [%d]", noti_err);
			error_code = EMAIL_ERROR_NOTI;
		}
	}
#endif /* __FEATURE_NOTIFICATION_ENABLE__ */
	EM_DEBUG_FUNC_END();
	return error_code;
}

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__

/**
 * @fn emcore_convert_to_uid_range_set(email_id_set_t* id_set, int id_set_count, email_uid_range_set **uid_range_set, int range_len, int *err_code)
 * Prepare a linked list of uid ranges with each node having a uid_range and lowest and highest uid in it.
 *
 *@author 					h.gahlaut@samsung.com
 * @param[in] id_set			Specifies the array of mail_id and corresponding server_mail_id sorted by server_mail_ids in ascending order
 * @param[in] id_set_count		Specifies the no. of cells in id_set array i.e. no. of sets of mail_ids and server_mail_ids
 * @param[in] range_len		Specifies the maximum length of string of range allowed.
 * @param[out] uid_range_set	Returns the uid_ranges formed in the form of a linked list with head stored in uid_range_set pointer
 * @param[out] err_code		Returns the error code.
 * @remarks 					An example of a uid_range formed is 2:6,8,10,14:15,89,
 *							While using it the caller should remove the ending, (comma)
 * @return This function returns true on success or false on failure.
 */

INTERNAL_FUNC int emcore_convert_to_uid_range_set(email_id_set_t *id_set, int id_set_count, email_uid_range_set **uid_range_set, int range_len, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int error = EMAIL_ERROR_NONE;

	if (NULL == id_set || id_set_count  <= 0 || NULL == uid_range_set) {
		EM_DEBUG_EXCEPTION(" Invalid Parameter id_set[%p] id_set_count[%d] uid_range_set[%p]", id_set, id_set_count, uid_range_set);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	int i = 0;
	unsigned long current_uid = 0;
	unsigned long first_uid = 0;
	unsigned long last_uid = 0;
	const int max_subset_string_size = MAX_SUBSET_STRING_SIZE;
	char subset_string[MAX_SUBSET_STRING_SIZE] = {0,};
	email_uid_range_set *current_node = NULL;	/* current_node denotes the current node under processing in the linked list of uid_range_set that is to be formed*/

	if (range_len < (max_subset_string_size + 1))		/* 1 for ending NULL character */ {
		EM_DEBUG_EXCEPTION(" Invalid Parameter range_len[%d]", range_len);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("id set count[%d] range_len[%d]", id_set_count, range_len);

	do {
		first_uid = last_uid = current_uid = id_set[i].server_mail_id;
		/* Start subset string by putting first server mail id in it from id_set*/
		memset(subset_string, 0x00, max_subset_string_size);
		SNPRINTF(subset_string, max_subset_string_size, "%lu", first_uid);
		++i;

		/* Check if only one server mail id was left in id_set */
		if (i >= id_set_count) {
			/* No more server mail id left in id_set */
			if (false == emcore_append_subset_string_to_uid_range(subset_string, &current_node, uid_range_set, range_len, first_uid, last_uid)) {
				EM_DEBUG_EXCEPTION("emcore_append_subset_string_to_uid_range failed");
				goto FINISH_OFF;
			}
			break;
		} else {
			/* More server mail id are present in id_set. Find out if first:last_uid is to be formed or only first_uid will be subset string */
			do {
				current_uid = id_set[i].server_mail_id;
				if (current_uid == (last_uid + 1)) {
					last_uid = current_uid;
					++i;
				} else {
					memset(subset_string, 0x00, max_subset_string_size);
					if (first_uid != last_uid)	/* Form subset string by first_uid:last_uid */
						SNPRINTF(subset_string, max_subset_string_size, "%lu:%lu", first_uid, last_uid);
					else	/* Form subset string by first_uid */
						SNPRINTF(subset_string, max_subset_string_size, "%lu", first_uid);

					if (false == emcore_append_subset_string_to_uid_range(subset_string, &current_node, uid_range_set, range_len, first_uid, last_uid)) {
						EM_DEBUG_EXCEPTION("emcore_append_subset_string_to_uid_range failed");
						goto FINISH_OFF;
					}
					/* To Start formation of new subset string break out of inner loop */
					break;
				}

			} while (i < id_set_count);

			/* Flow comes here in two cases :
			1. id_set ended and has continuous numbers at end of id_set so form subset string by first_uid:last_uid . in this  case last_uid == current_uid
			2. due to break statement */

			if (last_uid == current_uid) {
				/* Case 1 */
				memset(subset_string, 0x00, max_subset_string_size);
				SNPRINTF(subset_string, max_subset_string_size, "%lu:%lu", first_uid, last_uid);

				if (false == emcore_append_subset_string_to_uid_range(subset_string, &current_node, uid_range_set, range_len, first_uid, last_uid)) {
					EM_DEBUG_EXCEPTION("emcore_append_subset_string_to_uid_range failed");
					goto FINISH_OFF;
				}
			} else {
				/* Case 2: Do Nothing */
			}
		}
	} while (i < id_set_count);

	ret = true;

FINISH_OFF:
	if (NULL != err_code)
		*err_code = error;
	EM_DEBUG_FUNC_END();
	return ret;

}

/**
 * @fn emcore_append_subset_string_to_uid_range(char *subset_string, email_uid_range_set **uid_range_set, int range_len, unsigned long luid, unsigned long huid)
 * Appends the subset_string to uid range if the uid range has not exceeded maximum length(range_len), otherwise creates a new node in linked list of uid range set
 * and stores the subset_string in its uid_range. Also sets the lowest and highest uids for the corresponsing uid_range
 *
 * @author					h.gahlaut@samsung.com
 * @param[in] subset_string	Specifies the subset string to be appended. A subset string can be like X:Y or X where X and Y are uids.
 * @param[in] range_len		Specifies the maximum length of range string allowed.
 * @param[in] luid			Specifies the lowest uid in subset string
 * @param[in] huid			Specifies the highest uid in subset string
 * @param[out] uid_range_set	Returns the uid_ranges formed in the form of a linked list with head stored in uid_range_set pointer
 * @param[out] err_code		Returns the error code.
 * @remarks
 * @return This function returns true on success or false on failure.
 */

int emcore_append_subset_string_to_uid_range(char *subset_string, email_uid_range_set **current_node_adr, email_uid_range_set **uid_range_set, int range_len, unsigned long luid, unsigned long huid)
{
	EM_DEBUG_FUNC_BEGIN();
	email_uid_range_set *current_node = NULL;

	if (NULL == (*uid_range_set)) {
		/*This happens only once when list  creation starts. Head Node is allocated */
		current_node = (email_uid_range_set *)em_malloc(sizeof(email_uid_range_set));
		if (NULL == current_node) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			return false;
		}

		current_node->uid_range = (char *)em_malloc(range_len);

		if (NULL == current_node->uid_range) {
			EM_DEBUG_EXCEPTION("em_mallocfailed");
			EM_SAFE_FREE(current_node);
			return false;
		}

		SNPRINTF(current_node->uid_range, range_len, "%s,", subset_string);

		current_node->lowest_uid = luid;
		current_node->highest_uid = huid;
		(*uid_range_set) = current_node;

		(*current_node_adr) = current_node;

	} else {
		/* Apart from first call to this function flow will always come here */
		current_node = (*current_node_adr);
		int len_sub_string = EM_SAFE_STRLEN(subset_string);
		int space_left_in_buffer = range_len - EM_SAFE_STRLEN(current_node->uid_range);

		if ((len_sub_string + 1 + 1) <= space_left_in_buffer) 	/* 1 for comma + 1 for ending null character */ {
			SNPRINTF(current_node->uid_range + EM_SAFE_STRLEN(current_node->uid_range), space_left_in_buffer, "%s,", subset_string);
			current_node->highest_uid = huid;
		} else {
			/* No more space left in uid_range string.If continued on it, it will exceeded max size of range_len */
			/* Allocate new node in Uid Range set */
			email_uid_range_set *new_node = NULL;

			new_node = (email_uid_range_set *)em_malloc(sizeof(email_uid_range_set));

			if (NULL == new_node) {
				EM_DEBUG_EXCEPTION("em_mallocfailed");
				return false;
			}

			/* Allocate uid_range of new node */

			new_node->uid_range =  (char *)em_malloc(range_len);

			if (NULL == new_node->uid_range) {
				EM_DEBUG_EXCEPTION("em_mallocfailed");
				EM_SAFE_FREE(new_node);
				return false;
			}

			SNPRINTF(new_node->uid_range, range_len, "%s, ", subset_string);

			new_node->lowest_uid = luid;
			new_node->highest_uid = huid;

			current_node->next = new_node;

			(*current_node_adr) = new_node;
		}
	}
	EM_DEBUG_FUNC_END();
	return true;
}

/**
 * void emcore_free_uid_range_set(email_uid_range_set **uid_range_head)
 * Frees the linked list of uid ranges
 *
 * @author 					h.gahlaut@samsung.com
 * @param[in] uid_range_head	Head pointer of linked list of uid ranges
 * @remarks
 * @return This function does not return anything.
 */

INTERNAL_FUNC
void emcore_free_uid_range_set(email_uid_range_set **uid_range_set)
{
	EM_DEBUG_FUNC_BEGIN();

	email_uid_range_set *current_node = NULL;
	email_uid_range_set *uid_range_head = NULL;

	current_node = uid_range_head = (*uid_range_set);	/* Make the current node and head ptr point to starting of  uid_range_set */

	while (current_node) {
		uid_range_head = current_node->next;		/* Move the head ptr to next node*/

		EM_SAFE_FREE(current_node->uid_range);
		EM_SAFE_FREE(current_node);				/* Free the current node */

		current_node = uid_range_head;			/* Make the current node point to head ptr */
	}

	(*uid_range_set) = NULL;
	EM_DEBUG_FUNC_END();
}


/**
 * @fn emcore_form_comma_separated_strings(int numbers[], int num_count, int max_string_len, char *** strings, int *string_count, int *err_code)
 * Forms comma separated strings of a give max_string_len from an array of numbers
 *
 * @author 					h.gahlaut@samsung.com
 * @param[in] numbers			Specifies the array of numbers to be converted into comma separated strings.
 * @param[in] num_count		Specifies the count of numbers in numbers array.
 * @param[in] max_string_len	Specifies the maximum length of comma separated strings that are to be formed.
 * @param[out] strings			Returns the base address of a double dimension array which stores the strings.
 * @param[out] string_count		Returns the number of strings formed.
 * @param[out] err_code		Returns the error code.
 * @remarks 					If Input to the function is five numbers like 2755 2754 2748 2749 2750 and a given max_string_len is 20.
 *							Then this function will form two comma separated strings as follows -
 *							"2755, 2754, 2748"
 *							"2749, 2750"
 * @return This function returns true on success or false on failure.
 */

INTERNAL_FUNC int emcore_form_comma_separated_strings(int numbers[], int num_count, int max_string_len, char *** strings, int *string_count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int error = EMAIL_ERROR_NONE;
	int ret = false;

	char **string_list = NULL;
	int num_of_strings = 0;
	int i = 0;
	int j = 0;
	char num[MAX_INTEGER_LENGTH + 1] = {0, };
	int num_len = 0;
	int space_in_buffer = 0;
	int len_of_string_formed = 0;

	if (NULL == numbers || num_count <= 0 ||
		max_string_len < (MAX_INTEGER_LENGTH + 2) || NULL == strings || NULL == string_count)			/*  32767, is the highest integer possible in string.This requires 7 bytes of storage in character type array (1 byte for ending NULL and 1 byte for ending comma) so max_string_len should not be less than worst case possible.  */ {
		EM_DEBUG_EXCEPTION("Invalid Parameter numbers[%p] num_count [%d] max_string_len [%d] strings [%p] string_count[%p]",
			numbers, num_count, max_string_len, strings, string_count);
		error = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("num_count [%d] max_string_len [%d]", num_count, max_string_len);

	string_list = em_malloc(sizeof(char *));

	if (NULL == string_list) {
		EM_DEBUG_EXCEPTION("em_mallocfailed ");
		goto FINISH_OFF;
	}

	string_list[num_of_strings] = em_malloc(max_string_len);

	if (NULL == string_list[num_of_strings]) {
		EM_DEBUG_EXCEPTION("em_mallocfailed ");
		goto FINISH_OFF;
	}

	++num_of_strings;
	space_in_buffer = max_string_len;

	for (j = 0; j < num_count; ++j) {
		memset(num, 0x00, MAX_INTEGER_LENGTH + 1);
		SNPRINTF(num, MAX_INTEGER_LENGTH + 1, "%d", numbers[j]);

		num_len = EM_SAFE_STRLEN(num);

		len_of_string_formed = EM_SAFE_STRLEN(string_list[num_of_strings - 1]);

		space_in_buffer = max_string_len - len_of_string_formed ;

		if (space_in_buffer >= (num_len+1+1))			/*  1 for comma and 1 for ending NULL */ {
			SNPRINTF(string_list[num_of_strings - 1] + len_of_string_formed, max_string_len, "%d,", numbers[j]);
		} else {	/*  Removing comma at end of string  */
			string_list[num_of_strings - 1][len_of_string_formed-1] = '\0';
			char **temp = NULL;
			temp = (char **)realloc(string_list, sizeof(char *) * (num_of_strings + 1));	/*  Allocate new buffer to store a pointer to a new string */

			if (NULL == temp) {
				EM_DEBUG_EXCEPTION("realloc failed");
				goto FINISH_OFF;
			}

			memset(temp + num_of_strings, 0X00, sizeof(char *));

			string_list = temp;
			temp = NULL;
			string_list[num_of_strings] = em_malloc(max_string_len);/*  Allocate new buffer to store the string */

			if (NULL == string_list[num_of_strings]) {
				EM_DEBUG_EXCEPTION(" em_mallocfailed ");
				goto FINISH_OFF;
			}
			++num_of_strings;
			SNPRINTF(string_list[num_of_strings - 1] , max_string_len, "%d,", numbers[j]);/*  Start making new string */
		}
	}

	/*  Removing comma at end of string  */
	len_of_string_formed = EM_SAFE_STRLEN(string_list[num_of_strings - 1]);
	string_list[num_of_strings - 1][len_of_string_formed-1] = '\0';
	ret = true;

FINISH_OFF:

	if (false == ret)
		emcore_free_comma_separated_strings(&string_list, &num_of_strings);

	if (true == ret) {
		for (i = 0; i < num_of_strings; ++i)
			EM_DEBUG_LOG("%s", string_list[i]);
		*strings = string_list;
		*string_count = num_of_strings;
	}


	if (NULL != err_code)
		*err_code = error;

	EM_DEBUG_FUNC_END("ret [%d]", ret);
	return ret;
}
/**
 * @fn emcore_free_comma_separated_strings(char *** string_list, int *string_count)
 * Frees the double dimensional array of strings.
 *
 * @author 					h.gahlaut@samsung.com
 * @param[in] uid_range_head	Address of base address of double dimensional array of strings.
 * @param[in] string_count		Address of variable holding the count of strings.
 * @remarks
 * @return This function does not return anything.
 */
INTERNAL_FUNC void emcore_free_comma_separated_strings(char *** string_list, int *string_count)
{
	EM_DEBUG_FUNC_BEGIN();
	int i = 0;
	char **str_list = NULL;
	int count = 0;

	if (NULL != string_list) {
		str_list = *string_list;

		if (0 != *string_count) {
			count = *string_count;
			for (i = 0; i < count; ++i)
				EM_SAFE_FREE(str_list[i]);
		}

		EM_SAFE_FREE(str_list);
		*string_list = NULL;	/*  This makes the pointer to be freed as NULL in calling function and saves it from being a dangling pointer for sometime in calling function */
		*string_count = 0;
	}
	EM_DEBUG_FUNC_END();
}


#endif




int emcore_make_attachment_file_name_with_extension(char *source_file_name, char *sub_type, char *result_file_name, int result_file_name_buffer_length, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("source_file_name[%s], sub_type[%s], result_file_name_buffer_length[%d] ", source_file_name, sub_type, result_file_name_buffer_length);
	int ret = false, err = EMAIL_ERROR_NONE;
	char *extcheck = NULL;
	char attachment_file_name[MAX_PATH + 1] = { 0, };

	if (!source_file_name || !result_file_name) {
		EM_DEBUG_EXCEPTION("Invalid Parameter");
		err  = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	strncpy(attachment_file_name, source_file_name, MAX_PATH);
	extcheck = strchr(attachment_file_name, '.');

	if (extcheck)
		EM_DEBUG_LOG("Extension Exist in the Attachment [%s] ", extcheck);
	else  {	/* No extension attached, So add the Extension based on the subtype */
		if (sub_type) {
            if ((MAX_PATH + 1) < (strlen(sub_type) + 1)) {
                EM_DEBUG_EXCEPTION("Buffer overflow");
                err = EMAIL_ERROR_OUT_OF_MEMORY;
                goto FINISH_OFF;
            }

			EM_SAFE_STRNCAT(attachment_file_name, ".", MAX_PATH - EM_SAFE_STRLEN(attachment_file_name) - 1);
			EM_SAFE_STRNCAT(attachment_file_name, sub_type, MAX_PATH - EM_SAFE_STRLEN(attachment_file_name) - 1);
			EM_DEBUG_LOG_SEC("attachment_file_name with extension[%s] ", attachment_file_name);
		} else
			EM_DEBUG_LOG("UnKnown Extesnsion");

	}
	memset(result_file_name, 0 , result_file_name_buffer_length);
	EM_SAFE_STRNCPY(result_file_name, attachment_file_name, result_file_name_buffer_length - 1);
	EM_DEBUG_LOG_SEC("*result_file_name[%s]", result_file_name);
	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;
	EM_DEBUG_FUNC_END();
	return ret;
}

#ifdef __FEATURE_LOCAL_ACTIVITY__
INTERNAL_FUNC int emcore_add_activity(emstorage_activity_tbl_t *new_activity, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	EM_DEBUG_LOG("\t new_activity[%p], err_code[%p]", new_activity, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!new_activity) {
		EM_DEBUG_LOG("\t new_activity[%p]\n", new_activity);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	if (!emstorage_add_activity(new_activity, false, &err)) {
		EM_DEBUG_LOG("\t emstorage_add_activity falied - %d\n", err);

		goto FINISH_OFF;
	}
	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;

	return ret;
}

INTERNAL_FUNC int emcore_delete_activity(emstorage_activity_tbl_t *activity, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	EM_DEBUG_LOG("\t new_activity[%p], err_code[%p]", activity, err_code);

	/*  default variable */
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	if (!activity) {
		EM_DEBUG_LOG("\t new_activity[%p]\n", activity);

		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	if (!emstorage_delete_local_activity(activity, true, &err)) {
		EM_DEBUG_LOG("\t emstorage_delete_local_activity falied - %d\n", err);

		goto FINISH_OFF;
	}
	ret = true;

FINISH_OFF:
	if (err_code)
		*err_code = err;

	return ret;
}

INTERNAL_FUNC int emcore_get_next_activity_id(int *activity_id, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (NULL == activity_id) {
		EM_DEBUG_EXCEPTION("\t activity_id[%p]", activity_id);
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (false == emstorage_get_next_activity_id(activity_id, &err)) {
		EM_DEBUG_LOG("\t emstorage_get_next_activity_id failed - %d\n", err);
		goto FINISH_OFF;
	}

	ret = true;

	FINISH_OFF:
	if (NULL != err_code) {
		*err_code = err;
	}

	return ret;

}

#endif /* __FEATURE_LOCAL_ACTIVITY__ */


INTERNAL_FUNC void emcore_free_rule(email_rule_t* rule)
{
	EM_DEBUG_FUNC_BEGIN();

	if (!rule)
		return;

	EM_SAFE_FREE(rule->filter_name);
	EM_SAFE_FREE(rule->value);
	EM_SAFE_FREE(rule->value2);

	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_free_body_sparep(void **p)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_SAFE_FREE(*p);
	EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC int emcore_search_string_from_file(char *file_path, char *search_string, char *new_string, int *result)
{
	EM_DEBUG_FUNC_BEGIN_SEC("file_path : [%s], search_string : [%s]", file_path, search_string);
	int error = EMAIL_ERROR_NONE;
	int file_size = 0;
	int p_result = false;
	FILE *fp = NULL;
	char *buf = NULL;
	char *stripped = NULL;
	char *cid_string = NULL;
	char *modified_string = NULL;

	if (!search_string || !file_path || !result) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		error = EMAIL_ERROR_INVALID_PARAM;
		return error;
	}

	error = em_fopen(file_path, "r", &fp);
	if (error != EMAIL_ERROR_NONE || fp == NULL) {
		EM_DEBUG_EXCEPTION("em_fopen error [%d] [%s]", error, file_path);
		goto FINISH_OFF;
	}

	if (!emcore_get_file_size(file_path, &file_size, &error)) {
		EM_DEBUG_EXCEPTION("emcore_get_file_size error [%s]", file_path);
		goto FINISH_OFF;
	}

	buf = em_malloc(file_size + 1);
	if (buf == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (fread(buf, sizeof(char), file_size, fp) != file_size) {
		EM_DEBUG_EXCEPTION("fread error [%d]", errno);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/*prevent 35586*/
	stripped = em_replace_all_string(buf, CRLF_STRING, "");
	if (stripped) {
		if (new_string) {
			cid_string = em_malloc(strlen(search_string) + strlen("cid:") + 1);
			if (cid_string == NULL) {
				EM_DEBUG_EXCEPTION("em_mallocfailed");
				error = EMAIL_ERROR_OUT_OF_MEMORY;
				goto FINISH_OFF;
			}

			snprintf(cid_string, EM_SAFE_STRLEN(search_string) + EM_SAFE_STRLEN("cid:") + 1, "cid:%s", search_string);

			modified_string = em_replace_string(stripped, cid_string, new_string);
			if (modified_string) {
				if (fp)
					fclose(fp);

				error = em_fopen(file_path, "w", &fp);
				if (error != EMAIL_ERROR_NONE || fp == NULL) {
					EM_DEBUG_EXCEPTION("em_fopen failed");
					goto FINISH_OFF;
				}

				fprintf(fp, "%s", modified_string);

				p_result = true;
			}

		} else {
			if (strstr(stripped, search_string))
				p_result = true;
		}
	}

FINISH_OFF:
	if (!p_result)
		EM_DEBUG_LOG("Search string[%s] not found", search_string);

	*result = p_result;

	if (fp)
		fclose(fp);

	EM_SAFE_FREE(buf);
	EM_SAFE_FREE(stripped);  /*prevent 35586*/
	EM_SAFE_FREE(cid_string);
	EM_SAFE_FREE(modified_string);

	EM_DEBUG_FUNC_END("error:[%d]", error);
	return error;
}

INTERNAL_FUNC int emcore_load_query_from_file(char *file_path, char ***query_array, int *array_len)
{
	EM_DEBUG_FUNC_BEGIN_SEC("file_path : [%s], query_array : [%p]", file_path, *query_array);
	int error = EMAIL_ERROR_NONE;
	int file_size = 0;
	int vector_len = 0;
	int i = 0;
	FILE *fp = NULL;
	char *buf = NULL;
	char **result_vector = NULL;

	if (!file_path) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		error = EMAIL_ERROR_INVALID_PARAM;
		return error;
	}

	error = em_fopen(file_path, "r", &fp);
	if (error != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_fopen failed");
		goto FINISH_OFF;
	}

	if (!emcore_get_file_size(file_path, &file_size, &error)) {
		EM_DEBUG_EXCEPTION("emcore_get_file_size failed");
		goto FINISH_OFF;
	}

	buf = em_malloc(file_size + 1);
	if (buf == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		error = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (fread(buf, sizeof(char), file_size, fp) != file_size) {
		EM_DEBUG_EXCEPTION("Get the data from file : failed");
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	result_vector = g_strsplit(buf, ";", -1);
	if (!result_vector || g_strv_length(result_vector) <= 0) {
		EM_DEBUG_EXCEPTION("Parsing sql file failed");
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	vector_len = g_strv_length(result_vector);
	if (vector_len <= 0) {
		EM_DEBUG_EXCEPTION("vector length : [%d]", vector_len);
		error = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	*query_array = (char **)calloc(vector_len, sizeof(char *));

	for (i = 0; i < vector_len; i++) {
		if (result_vector[i]) {
			char *str = g_strconcat(result_vector[i], ";", NULL);
			if (str) (*query_array)[i] = str;
		}
	}

	if (array_len) {
		*array_len = vector_len;
		EM_DEBUG_LOG("SQL string array length : [%d]", vector_len);
	}

FINISH_OFF:

	if (fp)
		fclose(fp);

	EM_SAFE_FREE(buf);

	if (result_vector) {
		g_strfreev(result_vector);
	}

	EM_DEBUG_FUNC_END("error:[%d]", error);
	return error;
}

/* peak schedule */
static int emcore_get_next_peak_start_time(emstorage_account_tbl_t *input_account_ref, time_t input_current_time, time_t *output_time)
{
	EM_DEBUG_FUNC_BEGIN("input_account_ref [%p] input_time[%d] output_time[%p]", input_account_ref, input_current_time, output_time);
	int err = EMAIL_ERROR_NONE;
	int wday = 1;
	int day_count = 0;
	int start_hour = 0;
	int start_min  = 0;
	struct tm time_buf;
	struct tm *time_info;

	if (output_time == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	time_info = localtime_r(&input_current_time, &time_buf);
	if (time_info == NULL) {
		EM_DEBUG_EXCEPTION("localtime failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("input time and date: %s", asctime(time_info));

	wday = wday << time_info->tm_wday;

	EM_DEBUG_LOG("wday[%d] peak_days[%d]", wday, input_account_ref->peak_days);

	/* find next peak day */

	if (wday & input_account_ref->peak_days) {
		start_hour = input_account_ref->peak_start_time / 100;
		start_min  = input_account_ref->peak_start_time % 100;
		if (start_hour < time_info->tm_hour || (start_hour == time_info->tm_hour && start_min < time_info->tm_min)) {
			if (wday == EMAIL_PEAK_DAYS_SATDAY)
				wday = EMAIL_PEAK_DAYS_SUNDAY;
			else
				wday = wday << 1;
			day_count++;
		}

	}

	while (!(wday & input_account_ref->peak_days) && day_count < 7) {
		EM_DEBUG_LOG("wday[%d] day_count[%d]", wday, day_count);
		if (wday == EMAIL_PEAK_DAYS_SATDAY)
			wday = EMAIL_PEAK_DAYS_SUNDAY;
		else
			wday = wday << 1;
		day_count++;
	}

	EM_DEBUG_LOG("day_count[%d]", day_count);

	if (day_count < 7) {
		time_info->tm_mday += day_count; /* The other members of time_info will be interpreted or set by mktime */

		time_info->tm_hour = input_account_ref->peak_start_time / 100;
		time_info->tm_min  = input_account_ref->peak_start_time % 100;
		time_info->tm_min  = 0;

		*output_time = mktime(time_info);
		EM_DEBUG_LOG("result_time: %s", asctime(time_info));
	}

FINISH_OFF:

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

static int emcore_check_time_in_peak_schedule(emstorage_account_tbl_t *input_account_ref, time_t input_time, int *output_result)
{
	EM_DEBUG_FUNC_BEGIN("input_account_ref [%p] input_time[%d] output_result[%p]", input_account_ref, input_time, output_result);
	int err = EMAIL_ERROR_NONE;
	struct tm time_buf;
	struct tm *time_info;
	int wday = 1;
	int result = 0;
	int start_hour = 0;
	int start_min  = 0;
	int end_hour = 0;
	int end_min  = 0;

	if (input_account_ref == NULL || output_result == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	time_info = localtime_r(&input_time, &time_buf);
	if (time_info == NULL) {
		EM_DEBUG_EXCEPTION("localtime failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("input time and date: %s", asctime(time_info));

	wday = wday << time_info->tm_wday;

	EM_DEBUG_LOG("wday [%d]", wday);

	if (wday & input_account_ref->peak_days) {
		start_hour = input_account_ref->peak_start_time / 100;
		start_min  = input_account_ref->peak_start_time % 100;

		end_hour   = input_account_ref->peak_end_time / 100;
		end_min    = input_account_ref->peak_end_time % 100;

		EM_DEBUG_LOG("start_hour[%d] start_min[%d] end_hour[%d] end_min[%d]", start_hour, start_min, end_hour, end_min);

		if (start_hour <= time_info->tm_hour && time_info->tm_hour <= end_hour) {
			if (time_info->tm_hour == end_hour) {
				if (time_info->tm_min < end_min)
					result = 1;
			} else
				result = 1;
		}
	}

	EM_DEBUG_LOG("result[%d]", result);
	*output_result = result;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_calc_next_time_to_sync(char *multi_user_name, int input_account_id, time_t input_current_time, time_t *output_time)
{
	EM_DEBUG_FUNC_BEGIN("input_account_id[%d] input_current_time[%d] output_time[%p]", input_account_id, input_current_time, output_time);
	int err = EMAIL_ERROR_NONE;
	emstorage_account_tbl_t *account = NULL;
	time_t next_time = 0;
	time_t next_peak_start_time = 0;
	time_t result_time = 0;
	int    is_time_in_peak_schedule = 0;

	emstorage_get_account_by_id(multi_user_name, input_account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account, true, &err);
	if (account == NULL) {
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed [%d]", err);
		goto FINISH_OFF;
	}

	if (account->peak_days < 0) {
		/* peak schedule disabled */
		if (account->check_interval > 0)
			result_time = input_current_time + (account->check_interval * 60);
	} else if (account->peak_days > 0) {
		/* peak schedule enabled */

		/* if current time is in peak schedule */
		emcore_check_time_in_peak_schedule(account, input_current_time, &is_time_in_peak_schedule);
		EM_DEBUG_LOG("is_time_in_peak_schedule [%d]", is_time_in_peak_schedule);
		if (is_time_in_peak_schedule) {
			/* if next time to sync is in peak schedule? */
			next_time = input_current_time + (account->peak_interval * 60);

			emcore_check_time_in_peak_schedule(account, next_time, &is_time_in_peak_schedule);
			if (is_time_in_peak_schedule) {
				/* create an alarm to sync in peak schedule*/
				result_time = next_time;
			}
		}

		if (result_time == 0) {
			/* if current time is not in peak schedule */
			if (next_time == 0)  {
				/* if normal sync schedule is set */
				if (account->check_interval) {
					next_time = input_current_time + (account->check_interval * 60);
				}
			}
			emcore_get_next_peak_start_time(account, input_current_time, &next_peak_start_time);

			EM_DEBUG_LOG("next_time            : %s", ctime(&next_time));
			EM_DEBUG_LOG("next_peak_start_time : %s", ctime(&next_peak_start_time));

			/* if next time to sync is closer than next peak schedule start? */
			if (next_time && (next_time < next_peak_start_time)) {
				/* create an alarm to sync in check_interval */
				result_time = next_time;
			}
			/* else check_interval is zero or next peak schedule start is close than next check_interval */
			else {
				result_time = next_peak_start_time;
			}
		}
	}

	*output_time = result_time;
	EM_DEBUG_LOG("result_time : %s", ctime(&result_time));


FINISH_OFF:
	if (account) {
		emstorage_free_account(&account, 1, NULL);
	}

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

static int convert_contact_err_to_email_err(int contact_err)
{
	int err = EMAIL_ERROR_NONE;

	switch (contact_err) {
	case CONTACTS_ERROR_NONE:
		err = EMAIL_ERROR_NONE;
		break;
	case CONTACTS_ERROR_OUT_OF_MEMORY:
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		break;
	case CONTACTS_ERROR_INVALID_PARAMETER:
		err = EMAIL_ERROR_INVALID_PARAM;
		break;
	case CONTACTS_ERROR_NO_DATA:
		err = EMAIL_ERROR_DATA_NOT_FOUND;
		break;
	case CONTACTS_ERROR_DB:
		err = EMAIL_ERROR_DB_FAILURE;
		break;
	case CONTACTS_ERROR_IPC:
		err = EMAIL_ERROR_IPC_CONNECTION_FAILURE;
		break;
	case CONTACTS_ERROR_SYSTEM:
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		break;
	case CONTACTS_ERROR_INTERNAL:
	default:
		err = EMAIL_ERROR_UNKNOWN;
		break;
	}
	return err;
}

int emcore_get_mail_contact_info(char *multi_user_name, email_mail_contact_info_t *contact_info, char *full_address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("contact_info[%p], full_address[%s], err_code[%p]", contact_info, full_address, err_code);

	int ret = false;
	int err = EMAIL_ERROR_NONE;

	if (!emcore_get_mail_contact_info_with_update(multi_user_name, contact_info, full_address, 0, &err))
		EM_DEBUG_EXCEPTION("emcore_get_mail_contact_info_with_update failed [%d]", err);
	else
		ret = true;

	if (err_code != NULL)
		*err_code = err;

	return ret;
}

int emcore_search_contact_info(char *multi_user_name, const char *contact_uri, int address_property_id, char *address, int favorite_property_id, bool is_favorite, int limit, contacts_record_h *contacts_record)
{
	EM_DEBUG_FUNC_BEGIN();
	int contact_err = CONTACTS_ERROR_NONE;

	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;
	contacts_list_h list = NULL;

	void *join_zone = NULL;

	if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		if (emcore_set_join_zone(multi_user_name, &join_zone) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_set_join_zone failed");
			goto FINISH_OFF;
		}
	}

	if ((contact_err = contacts_query_create(contact_uri, &query)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_query_create failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_filter_create(contact_uri, &filter)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_filter_create failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_filter_add_str(filter, address_property_id, CONTACTS_MATCH_FULLSTRING, address)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_filter_add_str failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_filter_add_operator(filter, CONTACTS_FILTER_OPERATOR_AND)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_filter_add_operator failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_filter_add_bool(filter, favorite_property_id, is_favorite)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_filter_add_bool failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_query_set_filter(query, filter)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_query_set_filter failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_db_get_records_with_query(query, 0, limit, &list)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_db_get_record_with_query failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_list_get_current_record_p(list, contacts_record)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_list_get_current_record_p failed");
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (query != NULL)
		contacts_query_destroy(query);

	if (filter != NULL)
		contacts_filter_destroy(filter);

	if (list != NULL)
		contacts_list_destroy(list, false);

	if (join_zone)
		emcore_unset_join_zone(join_zone);

	return contact_err;
}

int emcore_search_contact_info_by_address(char *multi_user_name, const char *contact_uri, int property_id, char *address, int limit, contacts_record_h *contacts_record)
{
	EM_DEBUG_FUNC_BEGIN();
	int contact_err = CONTACTS_ERROR_NONE;

	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;
	contacts_list_h list = NULL;

	void *join_zone = NULL;

	if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		if (emcore_set_join_zone(multi_user_name, &join_zone) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_set_join_zone failed");
			goto FINISH_OFF;
		}
	}

	if ((contact_err = contacts_query_create(contact_uri, &query)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_query_create failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_filter_create(contact_uri, &filter)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_filter_create failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_filter_add_str(filter, property_id, CONTACTS_MATCH_FULLSTRING, address)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_filter_add_str failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_query_set_filter(query, filter)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_query_set_filter failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_db_get_records_with_query(query, 0, limit, &list)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_db_get_record_with_query failed");
		goto FINISH_OFF;
	}

	if ((contact_err = contacts_list_get_current_record_p(list, contacts_record)) != CONTACTS_ERROR_NONE) {
		if (contact_err != CONTACTS_ERROR_NO_DATA) /* no matching record found */
			EM_DEBUG_EXCEPTION("contacts_list_get_current_record_p failed [%d]", contact_err);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (query != NULL)
		contacts_query_destroy(query);

	if (filter != NULL)
		contacts_filter_destroy(filter);

	if (list != NULL)
		contacts_list_destroy(list, false);

	if (join_zone)
		emcore_unset_join_zone(join_zone);

	return contact_err;
}

typedef struct {
	int   account_id;
	char *email_address;
	char *subject;
    char *multi_user_name;
	time_t date_time;
	email_action_t action;
} set_contacts_log_internal_t;

gboolean emcore_set_contacts_log_internal(void* arg)
{
	if (!arg) {
		EM_DEBUG_EXCEPTION("no contact data to add");
		return FALSE;
	}
	set_contacts_log_internal_t* tmp = (set_contacts_log_internal_t*) arg;
	int   account_id    = tmp->account_id;
	char *email_address = tmp->email_address;
	char *subject       = tmp->subject;
	time_t date_time    = tmp->date_time;
	email_action_t action = tmp->action;
	char *multi_user_name = tmp->multi_user_name;

	int contacts_error = CONTACTS_ERROR_NONE;
	int person_id = 0;
	int action_type = 0;

	contacts_record_h phone_record = NULL;
	contacts_record_h person_record = NULL;

	void *join_zone = NULL;

	if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		if (emcore_set_join_zone(multi_user_name, &join_zone) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_set_join_zone failed");
			goto FINISH_OFF;
		}
	}

	if ((contacts_error = contacts_record_create(_contacts_phone_log._uri, &phone_record)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_query_create failed [%d]", contacts_error);
		goto FINISH_OFF;
	}

	/* Set email address */
	if ((contacts_error = contacts_record_set_str(phone_record, _contacts_phone_log.address, email_address)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_set_str [address] failed [%d]", contacts_error);
		goto FINISH_OFF;
	}

	/* Search contact person info */
	if ((contacts_error = emcore_search_contact_info_by_address(multi_user_name, _contacts_person_email._uri, _contacts_person_email.email, email_address, 1, &person_record)) != CONTACTS_ERROR_NONE) {
		if (contacts_error != CONTACTS_ERROR_NO_DATA) /* no matching record found */
			EM_DEBUG_EXCEPTION("emcore_search_contact_info_by_address failed [%d]", contacts_error);
	} else {
		/* Get person_id in contacts_person_email record  */
		if (person_record  && (contacts_error = contacts_record_get_int(person_record, _contacts_person_email.person_id, &person_id)) != CONTACTS_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("contacts_record_get_str failed [%d]", contacts_error);
			goto FINISH_OFF;
		}

		/* Set the person id */
		if ((contacts_error = contacts_record_set_int(phone_record, _contacts_phone_log.person_id, person_id)) != CONTACTS_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("contacts_record_set_int [person id] failed [%d]", contacts_error);
			goto FINISH_OFF;
		}
	}

	switch (action) {
	case EMAIL_ACTION_SEND_MAIL:
		action_type = CONTACTS_PLOG_TYPE_EMAIL_SENT;
		break;
	case EMAIL_ACTION_SYNC_HEADER:
		action_type = CONTACTS_PLOG_TYPE_EMAIL_RECEIVED;
		break;
	default:
		EM_DEBUG_EXCEPTION("Unknown action type");
		goto FINISH_OFF;
	}

	/* Set log type */
	if ((contacts_error = contacts_record_set_int(phone_record, _contacts_phone_log.log_type, action_type)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_set_int [log_type] failed [%d]", contacts_error);
		goto FINISH_OFF;
	}

	/* Set log time */
	if ((contacts_error = contacts_record_set_int(phone_record, _contacts_phone_log.log_time, (int)date_time)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_set_str [address] failed [%d]", contacts_error);
		goto FINISH_OFF;
	}

	/* Set subject */
	if ((contacts_error = contacts_record_set_str(phone_record, _contacts_phone_log.extra_data2, subject)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_set_str [subject] failed [%d]", contacts_error);
		goto FINISH_OFF;
	}

	/* Set Mail id */
	if ((contacts_error = contacts_record_set_int(phone_record, _contacts_phone_log.extra_data1, account_id)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_set_int [mail id] failed [%d]", contacts_error);
		goto FINISH_OFF;
	}

	/* Insert the record in DB */
	if ((contacts_error = contacts_db_insert_record(phone_record, NULL)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_db_insert_record failed [%d]", contacts_error);
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (phone_record != NULL)
		contacts_record_destroy(phone_record, false);

	if (person_record != NULL)
		contacts_record_destroy(person_record, false);

	if (join_zone)
		emcore_unset_join_zone(join_zone);

	EM_SAFE_FREE(email_address);
	EM_SAFE_FREE(subject);
	EM_SAFE_FREE(tmp);

	EM_DEBUG_FUNC_END("contact err [%d]", convert_contact_err_to_email_err(contacts_error));
	return FALSE;
}

int emcore_set_contacts_log(char *multi_user_name,
                                int   account_id,
                                char *email_address,
                                char *subject,
                                time_t date_time,
                                email_action_t action)
{
	/* arg shall be destroyed in emcore_set_contacts_log_internal */
	set_contacts_log_internal_t *arg = em_malloc(sizeof(set_contacts_log_internal_t));
	if (!arg) {
		EM_DEBUG_EXCEPTION("em_mallocerror");
		return EMAIL_ERROR_OUT_OF_MEMORY;
	}

	/* deep copy */
	arg->account_id      = account_id;
	arg->email_address   = EM_SAFE_STRDUP(email_address);
	arg->subject         = EM_SAFE_STRDUP(subject);
	arg->date_time       = date_time;
	arg->action          = action;
    arg->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

	g_main_context_invoke(NULL, emcore_set_contacts_log_internal, arg);

	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC int emcore_set_sent_contacts_log(char *multi_user_name, emstorage_mail_tbl_t *input_mail_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data : [%p]", input_mail_data);

	int i = 0;
	int err = EMAIL_ERROR_NONE;
	char email_address[MAX_EMAIL_ADDRESS_LENGTH];
	char *address_array[3] = {input_mail_data->full_address_to, input_mail_data->full_address_cc, input_mail_data->full_address_bcc};
	ADDRESS *addr = NULL;
	ADDRESS *p_addr = NULL;

	for (i = 0; i < 3; i++) {
		if (address_array[i] && address_array[i][0] != 0) {
			rfc822_parse_adrlist(&addr, address_array[i], NULL);
			for (p_addr = addr; p_addr; p_addr = p_addr->next) {
				SNPRINTF(email_address, MAX_EMAIL_ADDRESS_LENGTH, "%s@%s", addr->mailbox, addr->host);
				if ((err = emcore_set_contacts_log(multi_user_name,
                                                    input_mail_data->account_id,
                                                    email_address,
                                                    input_mail_data->subject,
                                                    input_mail_data->date_time,
                                                    EMAIL_ACTION_SEND_MAIL)) != EMAIL_ERROR_NONE) {
					EM_DEBUG_EXCEPTION("emcore_set_contacts_log failed : [%d]", err);
					goto FINISH_OFF;
				}
			}

			if (addr) {
				mail_free_address(&addr);
				addr = NULL;
			}
		}
	}

FINISH_OFF:

	if (addr)
		mail_free_address(&addr);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}


INTERNAL_FUNC int emcore_set_received_contacts_log(char *multi_user_name, emstorage_mail_tbl_t *input_mail_data)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data : [%p]", input_mail_data);
	int err = EMAIL_ERROR_NONE;

	if ((err = emcore_set_contacts_log(multi_user_name,
                                        input_mail_data->account_id,
                                        input_mail_data->email_address_sender,
                                        input_mail_data->subject,
                                        input_mail_data->date_time,
                                        EMAIL_ACTION_SYNC_HEADER)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("emcore_set_contacts_log failed [%d]", err);
	}

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

typedef struct _contacts_delete_data {
    int account_id;
    char *multi_user_name;
} contacts_delete_data;

gboolean emcore_delete_contacts_log_internal(void* arg)
{
	EM_DEBUG_FUNC_BEGIN();

    contacts_delete_data *data = (contacts_delete_data *)arg;
	int contacts_error = CONTACTS_ERROR_NONE;

	void *join_zone = NULL;

    if (EM_SAFE_STRLEN(data->multi_user_name) > 0) {
		if (emcore_set_join_zone(data->multi_user_name, &join_zone) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_set_join_zone failed");
			return FALSE;
		}
	}

	/* Delete record of the account id */
	if ((contacts_error = contacts_phone_log_delete(CONTACTS_PHONE_LOG_DELETE_BY_EMAIL_EXTRA_DATA1, data->account_id)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_phone_log_delete failed [%d]", contacts_error);
	}
/*	err = convert_contact_err_to_email_err(contacts_error); */

	if (join_zone)
		emcore_unset_join_zone(join_zone);

    EM_SAFE_FREE(data->multi_user_name);
    EM_SAFE_FREE(data);

	EM_DEBUG_FUNC_END();
	return FALSE;
}

INTERNAL_FUNC int emcore_delete_contacts_log(char *multi_user_name, int account_id)
{
    contacts_delete_data *data = NULL;

    data = (contacts_delete_data *)em_malloc(sizeof(contacts_delete_data));
    if (data == NULL) {
        EM_DEBUG_EXCEPTION("em_mallocfailed");
        return EMAIL_ERROR_OUT_OF_MEMORY;
    }

    data->account_id = account_id;
    data->multi_user_name = EM_SAFE_STRDUP(multi_user_name);

	g_main_context_invoke(NULL, emcore_delete_contacts_log_internal, (void*) data);

	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC int emcore_get_mail_display_name(char *multi_user_name, char *email_address, char **contact_display_name)
{
	if (!email_address || !contact_display_name) {
		EM_DEBUG_EXCEPTION("NULL_PARAM email_address[%p] contact_display_name[%p]",
                                                email_address, contact_display_name);
		return EMAIL_ERROR_INVALID_PARAM;
	}

	GError   *gerror = NULL;
	GVariant *result = NULL;
	int ret = EMAIL_ERROR_NONE;

#if !GLIB_CHECK_VERSION(2, 36, 0)
	g_type_init();
#endif

	GDBusProxy* bproxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
	                            G_DBUS_PROXY_FLAGS_NONE,
	                            NULL,
	                            EMAIL_SERVICE_NAME,
	                            EMAIL_SERVICE_PATH,
	                            EMAIL_SERVICE_NAME,
	                            NULL,
	                            &gerror);
	if (!bproxy) {
		EM_DEBUG_EXCEPTION("g_dbus_proxy_new_for_bus_sync error [%s]",
                                 gerror->message);
		ret = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
		goto FINISH_OFF;
	}

	result = g_dbus_proxy_call_sync(bproxy,
                                      "GetDisplayName",
                                      g_variant_new("(ss)", email_address, multi_user_name),
                                      G_DBUS_CALL_FLAGS_NONE,
                                      -1,
                                      NULL,
                                      &gerror);


	if (!result) {
		EM_DEBUG_EXCEPTION("g_dbus_proxy_call_sync 'GetDisplayName' error [%s]",
                                 gerror->message);
		ret = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
		goto FINISH_OFF;
	}

	g_variant_get(result, "(si)", contact_display_name, &ret);

	/* replace "" to NULL */
	if (!g_strcmp0(*contact_display_name, ""))
		EM_SAFE_FREE(*contact_display_name);

	g_variant_unref(result);

FINISH_OFF:
	EM_DEBUG_LOG("ret [%d]\n", ret);
	if (bproxy)
		g_object_unref(bproxy);

	if (gerror)
		g_error_free(gerror);

	return ret;
}


INTERNAL_FUNC int emcore_get_mail_display_name_internal(char *multi_user_name,
                                                        char *email_address,
                                                        char **contact_display_name)
{
	EM_DEBUG_FUNC_BEGIN_SEC("contact_name_value[%s], contact_display_name[%p]", email_address, contact_display_name);

	int contact_err = 0;
	int err = EMAIL_ERROR_NONE;
	int ret = false;
	char *display = NULL;
	/* Contact variable */
	contacts_record_h record = NULL;

/*
	if ((contact_err = contacts_connect_on_thread()) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("Open connect service failed [%d]", contact_err);
		goto FINISH_OFF;
	}
*/
	if ((contact_err = emcore_search_contact_info_by_address(multi_user_name, _contacts_contact_email._uri, _contacts_contact_email.email, email_address, 1, &record)) != CONTACTS_ERROR_NONE) {
		if (contact_err != CONTACTS_ERROR_NO_DATA) /* no matching record found */
			EM_DEBUG_EXCEPTION("emcore_search_contact_info_by_address failed [%d]", contact_err);
		goto FINISH_OFF;
	}
/*
	if ((contact_err = contacts_list_get_count(list, &list_count)) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_list_get_count failed");
		goto FINISH_OFF;
	}

	if (list_count > 1) {
		EM_DEBUG_EXCEPTION("Duplicated contacts info");
		contact_err = CONTACTS_ERROR_INTERNAL;
		goto FINISH_OFF;
	} else if (list_count == 0) {
		EM_DEBUG_EXCEPTION("Not found contact info");
		contact_err = CONTACTS_ERROR_NO_DATA;
		goto FINISH_OFF;
	}
*/

	if (contacts_record_get_str(record, _contacts_contact_email.display_name, &display) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_record_get_str failed");
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

/*	contacts_disconnect_on_thread(); */

	if (record != NULL)
		contacts_record_destroy(record, false);

	if (ret) {
		*contact_display_name = display;
	} else {
		*contact_display_name = NULL;
		EM_SAFE_FREE(display);
	}

	err = convert_contact_err_to_email_err(contact_err);

	return err;
}

INTERNAL_FUNC int emcore_connect_contacts_service(char *multi_user_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int contact_err = 0;
	int err = EMAIL_ERROR_NONE;

	void *join_zone = NULL;

    if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		if ((err = emcore_set_join_zone(multi_user_name, &join_zone)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_set_join_zone failed : [%d]", err);
			return err;
		}
    }

	if ((contact_err = contacts_connect()) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_connect failed : [%d]", contact_err);
	}

	err = convert_contact_err_to_email_err(contact_err);

	if (join_zone)
		emcore_unset_join_zone(join_zone);

	EM_DEBUG_FUNC_END();
	return err;
}

INTERNAL_FUNC int emcore_disconnect_contacts_service(char *multi_user_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int contact_err = 0;
	int err = EMAIL_ERROR_NONE;

	void *join_zone = NULL;

	if (EM_SAFE_STRLEN(multi_user_name) > 0) {
		if ((err = emcore_set_join_zone(multi_user_name, &join_zone)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_set_join_zone failed : [%d]", err);
			return err;
		}
    }

	if ((contact_err = contacts_disconnect()) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("contacts_disconnect failed : [%d]", contact_err);
	}

	err = convert_contact_err_to_email_err(contact_err);

	if (join_zone)
		emcore_unset_join_zone(join_zone);

	EM_DEBUG_FUNC_END();
	return err;
}

#ifdef __FEATURE_BLOCKING_MODE__

#define ALLOWED_CONTACT_TYPE_NONE      0
#define ALLOWED_CONTACT_TYPE_ALL       1
#define ALLOWED_CONTACT_TYPE_FAVORITES 2
#define ALLOWED_CONTACT_TYPE_CUSTOM    3

static int blocking_mode_status = false;
INTERNAL_FUNC int blocking_mode_of_setting = false;

INTERNAL_FUNC void emcore_set_blocking_mode_of_setting(int input_blocking_mode_of_setting)
{
	blocking_mode_of_setting = input_blocking_mode_of_setting;
}

INTERNAL_FUNC int emcore_get_blocking_mode_status()
{
	EM_DEBUG_FUNC_BEGIN("blocking_mode_status : [%d]", blocking_mode_status);
	EM_DEBUG_FUNC_END();

	return blocking_mode_status;
}

INTERNAL_FUNC void emcore_set_blocking_mode_status(int blocking_mode)
{
	blocking_mode_status = blocking_mode;
}

INTERNAL_FUNC int emcore_check_blocking_mode(char *multi_user_name, char *sender_address, int *blocking_status)
{
	if (!sender_address || !blocking_status) {
		EM_DEBUG_EXCEPTION("NULL_PARAM sender_address[%p] blocking_status[%p]",
                                                   sender_address, blocking_status);
		return EMAIL_ERROR_INVALID_PARAM;
	}


	GError   *gerror = NULL;
	GVariant *result = NULL;
	int ret = EMAIL_ERROR_NONE;

#if !GLIB_CHECK_VERSION(2, 36, 0)
	g_type_init();
#endif

	GDBusProxy* bproxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
	                            G_DBUS_PROXY_FLAGS_NONE,
	                            NULL,
	                            EMAIL_SERVICE_NAME,
	                            EMAIL_SERVICE_PATH,
	                            EMAIL_SERVICE_NAME,
	                            NULL,
	                            &gerror);
	if (!bproxy) {
		EM_DEBUG_EXCEPTION("g_dbus_proxy_new_for_bus_sync error [%s]",
                                 gerror->message);
		ret = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
		goto FINISH_OFF;
	}

	result = g_dbus_proxy_call_sync(bproxy,
                                      "CheckBlockingMode",
                                      g_variant_new("(ss)", sender_address, multi_user_name),
                                      G_DBUS_CALL_FLAGS_NONE,
                                      -1,
                                      NULL,
                                      &gerror);


	if (!result) {
		EM_DEBUG_EXCEPTION("g_dbus_proxy_call_sync 'CheckBlockingMode' error [%s]",
                                 gerror->message);
		ret = EMAIL_ERROR_IPC_PROTOCOL_FAILURE;
		goto FINISH_OFF;
	}

	g_variant_get(result, "(ii)", blocking_status, &ret);
	g_variant_unref(result);

FINISH_OFF:

	if (bproxy)
		g_object_unref(bproxy);

	if (gerror)
		g_error_free(gerror);

	return ret;
}

INTERNAL_FUNC int emcore_check_blocking_mode_internal(char *multi_user_name, char *sender_address, int *blocking_status)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;
	int contact_error = 0;
	int person_id = 0;
	int allowed_contact_type = 0; /* 0 : NONE, 1 : All contacts, 2 : Favorites, 3 : Custom */
	char *person_id_string = NULL;
	char *str = NULL;
	char *token = NULL;
	contacts_record_h record = NULL;

	if (!blocking_mode_of_setting) {
		return err;
	}

	if (vconf_get_int(VCONFKEY_SETAPPL_BLOCKINGMODE_ALLOWED_CONTACT_TYPE, &allowed_contact_type) != 0) {
		EM_DEBUG_EXCEPTION("vconf_get_int failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

/*
	if ((contact_error = contacts_connect_on_thread()) != CONTACTS_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("Open connect service failed [%d]", contact_error);
		goto FINISH_OFF;
	}
*/
	switch (allowed_contact_type) {
	case ALLOWED_CONTACT_TYPE_NONE:
		EM_DEBUG_LOG("allowed_contact_type is none : bloacking all(notification)");
		*blocking_status = true;
		break;

	case ALLOWED_CONTACT_TYPE_ALL:
		EM_DEBUG_LOG("allowed_contact_type is ALL");
		/* Search the allowed contact type in contact DB */
		if ((contact_error = emcore_search_contact_info_by_address(multi_user_name, _contacts_person_email._uri, _contacts_person_email.email, sender_address, 1, &record)) != CONTACTS_ERROR_NONE) {
			if (contact_error != CONTACTS_ERROR_NO_DATA) /* no matching record found */
				EM_DEBUG_EXCEPTION("emcore_search_contact_info_by_address failed [%d]", contact_error);
			goto FINISH_OFF;
		}

		if (record == NULL) {
			EM_DEBUG_LOG("No search contact info");
			goto FINISH_OFF;
		}

		*blocking_status = false;
		break;

	case ALLOWED_CONTACT_TYPE_FAVORITES:
		if ((contact_error = emcore_search_contact_info(multi_user_name, _contacts_person_email._uri, _contacts_person_email.email, sender_address, _contacts_person_email.is_favorite, true, 1, &record)) != CONTACTS_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_search_contact_info failed : [%d]", contact_error);
			goto FINISH_OFF;
		}

		if (record == NULL) {
			EM_DEBUG_LOG("No search contact info");
			goto FINISH_OFF;
		}

		*blocking_status = false;
		break;

	case ALLOWED_CONTACT_TYPE_CUSTOM:
		person_id_string = vconf_get_str(VCONFKEY_SETAPPL_BLOCKINGMODE_ALLOWED_CONTACT_LIST);
		if (person_id_string == NULL) {
			EM_DEBUG_LOG("Custom allowed contact type is NULL");
			goto FINISH_OFF;
		}

		if ((contact_error = emcore_search_contact_info_by_address(multi_user_name, _contacts_person_email._uri, _contacts_person_email.email, sender_address, 1, &record)) != CONTACTS_ERROR_NONE) {
			if (contact_error != CONTACTS_ERROR_NO_DATA) /* no matching record found */
				EM_DEBUG_EXCEPTION("emcore_search_contact_info_by_address failed [%d]", contact_error);
			goto FINISH_OFF;
		}

		if (record == NULL) {
			EM_DEBUG_LOG("No search contact info");
			goto FINISH_OFF;
		}

		if (contacts_record_get_int(record, _contacts_contact_email.person_id, &person_id) != CONTACTS_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("contacts_record_get_int failed");
			goto FINISH_OFF;
		}

		token = strtok_r(person_id_string, ",", &str);
		do {
			if (person_id == atoi(token)) {
				*blocking_status = false;
				break;
			}
		} while ((token = strtok_r(NULL, ",", &str)));

		break;

	default:
		EM_DEBUG_EXCEPTION("Invalid parameter : [%d]", allowed_contact_type);
		*blocking_status = true;
		err = EMAIL_ERROR_INVALID_PARAM;
	}

FINISH_OFF:

/*
	contacts_disconnect_on_thread();
*/
	err = convert_contact_err_to_email_err(contact_error);

	EM_SAFE_FREE(person_id_string);

	EM_DEBUG_FUNC_END();
	return err;
}

#endif     /* __FEATURE_BLOCKING_MODE__ */

INTERNAL_FUNC char *emcore_set_mime_entity(char *mime_path)
{
	EM_DEBUG_FUNC_BEGIN("mime_path : [%s]", mime_path);
	FILE *fp_read = NULL;
	FILE *fp_write = NULL;
	char *mime_entity = NULL;
	char *mime_entity_path = NULL;
	char temp_buffer[255] = {0,};
	int err = EMAIL_ERROR_NONE;
	int searched = 0;

	if (!emcore_get_temp_file_name(&mime_entity_path, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_get_temp_file_name failed[%d]", err);
		goto FINISH_OFF;
	}

	/* get mime entity */
	if (mime_path != NULL) {
		err = em_fopen(mime_path, "r", &fp_read);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION_SEC("File em_fopen(read) is failed : filename [%s]", mime_path);
			goto FINISH_OFF;
		}

		err = em_fopen(mime_entity_path, "w", &fp_write);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION_SEC("File em_fopen(write) is failed : filename [%s]", mime_entity_path);
			goto FINISH_OFF;
		}

		fseek(fp_read, 0, SEEK_SET);
		fseek(fp_write, 0, SEEK_SET);

		while (fgets(temp_buffer, 255, fp_read) != NULL) {
			mime_entity = strcasestr(temp_buffer, "content-type");
			if (mime_entity != NULL && !searched)
				searched = 1;

			if (searched) {
				fprintf(fp_write, "%s", temp_buffer);
			}
		}
	}

FINISH_OFF:
	if (fp_read)
		fclose(fp_read);

	if (fp_write)
		fclose(fp_write);

	EM_SAFE_FREE(mime_entity);

	EM_DEBUG_FUNC_END();
	return mime_entity_path;
}

INTERNAL_FUNC int emcore_get_content_from_file(char *filename, char **contents, int *length)
{
	EM_DEBUG_FUNC_BEGIN("filename[%s], contents[%p], length[%p]", filename, contents, length);

	struct stat stat_buf;
	int fd = 0;
	int bytes_read = 0;
	int size = 0;
	int alloc_size = 0;
	int err = EMAIL_ERROR_NONE;
	char *buf = NULL;
	char errno_buf[ERRNO_BUF_SIZE] = {0};

	if (filename == NULL || contents == NULL || length == NULL) {
		err = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("Invalid parameter");
		goto FINISH_OFF;
	}

	err = em_open(filename, O_RDONLY, 0, &fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open error [%s][%d]", filename, err);
		goto FINISH_OFF;
	}

	if (fstat(fd, &stat_buf) < 0) {
		EM_DEBUG_EXCEPTION("fstat failed for fd [%d]", fd);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (stat_buf.st_size > 0 && S_ISREG(stat_buf.st_mode)) {
		size = stat_buf.st_size;
		alloc_size = size + 1;
		buf = em_malloc(alloc_size);
		if (buf == NULL) {
			EM_DEBUG_EXCEPTION("malloc failed...");
			err = EMAIL_ERROR_OUT_OF_MEMORY;
			goto FINISH_OFF;
		}

		bytes_read = 0;
		errno = 0;

		while (bytes_read < size) {
			ssize_t rd_size = 0;
			rd_size = read(fd, buf + bytes_read, size - bytes_read);

			if (rd_size < 0) {
				if (errno != EINTR) {
					EM_DEBUG_EXCEPTION("read failed: %s", EM_STRERROR(errno_buf));
					err = EMAIL_ERROR_SYSTEM_FAILURE;
					goto FINISH_OFF;
				}
			} else if (rd_size == 0)
				break;
			else
				bytes_read += rd_size;
		}

		buf[bytes_read] = '\0';

		if (length)
			*length = bytes_read;

		*contents = buf;
	}
	EM_SAFE_CLOSE(fd); /* prevent 39093 */

	EM_DEBUG_FUNC_END();
	return err;

FINISH_OFF:

	EM_SAFE_FREE(buf);
	EM_SAFE_CLOSE(fd); /* prevent 39093 */
	EM_DEBUG_FUNC_END();

	return err;
}

INTERNAL_FUNC int emcore_set_content_to_file(const char *contents, char *dest_path, int length)
{
	EM_DEBUG_FUNC_BEGIN("contents[%p], dest_path[%s], length[%d]", contents, dest_path, length);
	int fd = 0;
	int err = EMAIL_ERROR_NONE;
	char *tmp_path = NULL;
	char errno_buf[ERRNO_BUF_SIZE] = {0};
	int byte_written = 0; /* 39063 */

	if (contents == NULL || dest_path == NULL || length <= 0) {
		err = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("Invalid parameter");
		goto FINISH_OFF;
	}

	tmp_path = emcore_mime_get_save_file_name(&err);
	if (!tmp_path) { /* prevent 39114 */
		EM_DEBUG_EXCEPTION("tmp_path NULL");
		goto FINISH_OFF;
	}

	err = em_open(tmp_path, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH, &fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open error [%s][%d]", tmp_path, err);
		goto FINISH_OFF;
	}

	errno = 0;
	while (length > 0 && contents && errno == 0) {
		byte_written = write(fd, (void *)contents, length);
		if (byte_written < 0) {
			/* interrupted by a signal */
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			EM_DEBUG_EXCEPTION("write failed: %s", EM_STRERROR(errno_buf));
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("NWRITTEN [%d]", byte_written);
		length -= byte_written;
		contents += byte_written;
	}

	errno = 0;
	if (fsync(fd) != 0) {
		EM_DEBUG_EXCEPTION("fsync failed: %s", EM_STRERROR(errno_buf));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
	}

	errno = 0;
	if (tmp_path && dest_path && rename(tmp_path, dest_path) != 0) {
		EM_DEBUG_EXCEPTION("rename failed: %s", EM_STRERROR(errno_buf));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

FINISH_OFF:

	EM_SAFE_FREE(tmp_path);
	EM_SAFE_CLOSE(fd); /* prevent 39114, 39136*/
	EM_DEBUG_FUNC_END();
	return err;
}

#ifdef __FEATURE_UPDATE_DB_TABLE_SCHEMA__

INTERNAL_FUNC int emcore_update_db_table_schema(char *multi_user_name)
{
	EM_DEBUG_FUNC_BEGIN();
	int err = EMAIL_ERROR_NONE;

	err = emstorage_update_db_table_schema(multi_user_name);

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

#endif /* __FEATURE_UPDATE_DB_TABLE_SCHEMA__ */

INTERNAL_FUNC int emcore_unescape_from_url(char *input_url, char **output_url)
{
	EM_DEBUG_FUNC_BEGIN("input_url[%p] output_url[%p]", input_url, output_url);
	int err = EMAIL_ERROR_NONE;
	int input_length = 0;
	int output_length = 0;
	char *result_string = NULL;
	CURL *curl = NULL;

	if (input_url == NULL || output_url == NULL) {
		err = EMAIL_ERROR_INVALID_PARAM;
		EM_DEBUG_EXCEPTION("Invalid parameter");
		goto FINISH_OFF;
	}

	*output_url = NULL;
	curl = curl_easy_init();

	input_length = EM_SAFE_STRLEN(input_url);

	result_string = curl_easy_unescape(curl, input_url, input_length, &output_length);

	if (output_length)
		*output_url = EM_SAFE_STRDUP(result_string);

FINISH_OFF:
	if (result_string)
		curl_free(result_string);

	if (curl)
		curl_easy_cleanup(curl);

	EM_DEBUG_FUNC_END("err[%d]", err);
	return err;
}

INTERNAL_FUNC char *__em_get_month_in_string(int month)
{
	EM_DEBUG_FUNC_BEGIN("month [%d]", month);

	char *mon = NULL;

	switch (month) {
	    case 0:
			mon = strdup("jan");
	    	break;
	    case 1:
			mon = strdup("feb");
	    	break;
	    case 2:
			mon = strdup("mar");
	    	break;
	    case 3:
			mon = strdup("apr");
	    	break;
	    case 4:
			mon = strdup("may");
	    	break;
	    case 5:
			mon = strdup("jun");
	    	break;
	    case 6:
			mon = strdup("jul");
	    	break;
	    case 7:
			mon = strdup("aug");
	    	break;
	    case 8:
			mon = strdup("sep");
	    	break;
	    case 9:
			mon = strdup("oct");
	    	break;
	    case 10:
			mon = strdup("nov");
	    	break;
	    case 11:
			mon = strdup("dec");
	    	break;
	}
	return mon;
}

INTERNAL_FUNC int emcore_make_date_string_for_search(time_t input_time, char **output_date_string)
{
	EM_DEBUG_FUNC_BEGIN("input_time[%p] output_date_string[%p]", input_time, output_date_string);
	int err = EMAIL_ERROR_NONE;
	struct tm time_buf;
	struct tm   *timeinfo;
	char *mon = NULL;
	char *temp_date_string = NULL;

	EM_DEBUG_LOG("RawTime Info [%lu]", input_time);

	timeinfo = localtime_r(&input_time, &time_buf);
	if (timeinfo == NULL) {
		EM_DEBUG_EXCEPTION("localtime failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		return err;
	}

	EM_DEBUG_LOG(">>>>>Time %d %d %d %d %d %d", 1900+timeinfo->tm_year, timeinfo->tm_mon+1, timeinfo->tm_mday);

	mon = __em_get_month_in_string(timeinfo->tm_mon);
	if (mon) {
		temp_date_string = g_strdup_printf("%d-%s-%04d", timeinfo->tm_mday, mon, 1900 + timeinfo->tm_year);
		EM_DEBUG_LOG("DATE IS [ %s ] ", temp_date_string);
		EM_SAFE_FREE(mon);
	}

	if (output_date_string)
		*output_date_string = temp_date_string;
	else
		EM_SAFE_FREE(temp_date_string);

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}

INTERNAL_FUNC int emcore_make_uid_range_string(emcore_uid_list *uid_list, int total, char **output_uid_range_string)
{
	EM_DEBUG_FUNC_BEGIN("uid_list[%p] total[%d] output_uid_range_string[%p]", uid_list, total, output_uid_range_string);
	int err = EMAIL_ERROR_NONE;
	emcore_uid_list *uid_list_prev = NULL;
	emcore_uid_list *uid_list_fast = uid_list;
	int index = 0;
	int msg_count = total;
	int uid_range_size = msg_count * 8 + 1000;
	char *uid_range = NULL;

	if (uid_list == NULL || total == 0 || output_uid_range_string == NULL) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err  = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG("memory allocation for uid_range [%d, %d]", msg_count, uid_range_size);
	uid_range = malloc(sizeof(char) * uid_range_size);
	if (uid_range == NULL) {
		EM_DEBUG_EXCEPTION("memory allocation for uid_range failed");
		err  = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	uid_list_prev = uid_list_fast;

	if (uid_list_fast->next == NULL) {
		/* Single list entry */
		snprintf(uid_range, uid_range_size, "%d", atoi(uid_list_fast->uid));
		EM_DEBUG_LOG("uid_range [%d]", uid_range);
	} else {
		/* forming range of uids to be passed */
		while (uid_list_fast  != NULL) {
			/* uid_list_fast = uid_list_fast->next; */

			if ((uid_list_fast->next != NULL) && (((atoi(uid_list_prev->uid)) - (atoi(uid_list_fast->next->uid))) == 1)) {
				index += snprintf(uid_range+index, uid_range_size, "%d", atoi(uid_list_prev->uid));

				uid_list_fast = uid_list_fast->next;
				uid_list_prev = uid_list_fast;

				/* to make UID range string "abc, XX : YY" */
				while (uid_list_fast  != NULL) {
					if (uid_list_fast->next == NULL)
						break;
					if (((atoi(uid_list_prev->uid)) - (atoi(uid_list_fast->next->uid))) == 1) {
						uid_list_fast = uid_list_fast->next;
						uid_list_prev = uid_list_fast;
					} else
						break;
				}
				if ((uid_list_fast  != NULL) && (uid_list_fast->next  != NULL))
					index  += snprintf(uid_range+index, uid_range_size, ":%d,", atoi(uid_list_prev->uid));
				else
					index  += snprintf(uid_range+index, uid_range_size, ":%d", atoi(uid_list_prev->uid));

				uid_list_fast = uid_list_fast->next;
				uid_list_prev = uid_list_fast;
			} else {
				if (uid_list_fast->next  != NULL)
					index  += snprintf(uid_range+index, uid_range_size, "%d,", atoi(uid_list_prev->uid));
				else
					index  += snprintf(uid_range+index, uid_range_size, "%d", atoi(uid_list_prev->uid));
				uid_list_fast = uid_list_fast->next;
				uid_list_prev = uid_list_fast;
			}
		}
	}
	EM_DEBUG_LOG("index [%d]", index);

	*output_uid_range_string = uid_range;

FINISH_OFF:

	EM_DEBUG_FUNC_END("err [%d]", err);
	return err;
}
/* EOF */
