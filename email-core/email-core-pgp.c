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



/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ***
 *File :  email-core-smime.c
 *Desc :  MIME Operation
 *
 *Auth :
 *
 *History :
 *   2011.04.14  :  created
 ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ***/
#undef close

#include <glib.h>
#include <gmime/gmime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <vconf.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "email-utilities.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-smtp.h"
#include "email-storage.h"
#include "email-core-cert.h"
#include "email-debug-log.h"

#define PGP_SIGNED_FILE     "signature.asc"
#define PGP_ENCRYPTED_FILE  "encrypted.asc"
#define DECRYPTED_TEMP_FILE "decrypted_temp_file.eml"

static char *passphrase = NULL;

static gboolean request_passwd(GMimeCryptoContext *ctx, const char *user_id, 
								const char *prompt_ctx, gboolean reprompt, 
								GMimeStream *response, GError **err)
{
	EM_DEBUG_FUNC_BEGIN();
	EM_DEBUG_LOG("passpharse : [%s]", passphrase);
        if (g_mime_stream_write_string (response, passphrase) == -1 ||
            g_mime_stream_write (response, "\n", 1) == -1) {
                g_set_error (err, GMIME_ERROR, errno, "%s", g_strerror (errno));
                return false;
        }

	EM_SAFE_FREE(passphrase);

	EM_DEBUG_FUNC_END();
	return true;
}

static int emcore_pgp_get_gmime_digest_algo(email_digest_type digest_type)
{
	EM_DEBUG_FUNC_BEGIN();
	int gmime_digest_algo = GMIME_DIGEST_ALGO_DEFAULT;

	switch (digest_type) {
	case DIGEST_TYPE_SHA1:
		gmime_digest_algo = GMIME_DIGEST_ALGO_SHA1;
		break;
	case DIGEST_TYPE_MD5:
		gmime_digest_algo = GMIME_DIGEST_ALGO_MD5;
		break;
	case DIGEST_TYPE_RIPEMD160:
		gmime_digest_algo = GMIME_DIGEST_ALGO_RIPEMD160;
		break;
	case DIGEST_TYPE_MD2:
		gmime_digest_algo = GMIME_DIGEST_ALGO_MD2;
		break;
	case DIGEST_TYPE_TIGER192:
		gmime_digest_algo = GMIME_DIGEST_ALGO_TIGER192;
		break;
	case DIGEST_TYPE_HAVAL5160:
		gmime_digest_algo = GMIME_DIGEST_ALGO_HAVAL5160;
		break;
	case DIGEST_TYPE_SHA256:
		gmime_digest_algo = GMIME_DIGEST_ALGO_SHA256;
		break;
	case DIGEST_TYPE_SHA384:
		gmime_digest_algo = GMIME_DIGEST_ALGO_SHA384;
		break;
	case DIGEST_TYPE_SHA512:
		gmime_digest_algo = GMIME_DIGEST_ALGO_SHA512;
		break;
	case DIGEST_TYPE_SHA224:
		gmime_digest_algo = GMIME_DIGEST_ALGO_SHA224;
		break;
	case DIGEST_TYPE_MD4:
		gmime_digest_algo = GMIME_DIGEST_ALGO_MD4;
		break;
	default:
		gmime_digest_algo = GMIME_DIGEST_ALGO_DEFAULT;
		break;
	}

	EM_DEBUG_FUNC_END();
	return gmime_digest_algo;
}

static int get_stack_of_recipients(char *recipients, GPtrArray **output_recipients_array) {
	EM_DEBUG_FUNC_BEGIN();
	int err                       = EMAIL_ERROR_NONE;
	char *temp_key                = NULL;
	char *keys                    = NULL;
	GPtrArray *p_recipients_array = NULL;
	gchar **strings               = NULL;
	gchar **ptr                   = NULL;

	p_recipients_array = g_ptr_array_new();
	if (p_recipients_array == NULL) {
		EM_DEBUG_EXCEPTION("g_ptr_array_new failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	keys = EM_SAFE_STRDUP(recipients);
	strings = g_strsplit(keys, ",", -1);

	for (ptr = strings; *ptr; ptr++){
	        temp_key = NULL;
	        temp_key = g_strdup_printf("0x%s",*(ptr));
	        g_ptr_array_add(p_recipients_array, temp_key);
	  }

FINISH_OFF:
	g_strfreev(strings);
	EM_SAFE_FREE(keys);
	if (err != EMAIL_ERROR_NONE)
		g_ptr_array_free(p_recipients_array, true);
	else 
		*output_recipients_array = p_recipients_array;

	EM_DEBUG_FUNC_END();
	return err;
}

static GMimeSignatureStatus get_signature_status(GMimeSignatureList *signatures)
{
	EM_DEBUG_FUNC_BEGIN();

	int i = 0;

	GMimeSignature *sig = NULL;
	GMimeSignatureStatus status = GMIME_SIGNATURE_STATUS_GOOD;
 
	if (!signatures || signatures->array->len == 0)
		return GMIME_SIGNATURE_STATUS_ERROR;
 
	for (i = 0; i < g_mime_signature_list_length(signatures); i++) {
		sig = g_mime_signature_list_get_signature(signatures, i);
		if (sig) status = MAX(status, sig->status);
	}   

	EM_DEBUG_FUNC_END();

	return status;
}

INTERNAL_FUNC int emcore_pgp_set_signed_message(char *certificate, 
												char *password, 
												char *mime_entity, 
												char *user_id, 
												email_digest_type digest_type, 
												char **file_path)
{
	EM_DEBUG_FUNC_BEGIN_SEC("Certificate path : [%s], mime_entity : [%s]", certificate, mime_entity);

#ifdef __FEATURE_SECURE_PGP__
	int err = EMAIL_ERROR_NONE;
	int clear_fd = 0;
	int gpg_fd = 0;
	int p_digest_type = 0;
	char temp_pgp_filepath[512] = {0, };

	GMimeCryptoContext *ctx  = NULL;
	GMimeStream *clear_text  = NULL;
	GMimeStream *signed_text = NULL;
	GError *g_err            = NULL;

	if (!password || !mime_entity || !user_id) {
		EM_DEBUG_EXCEPTION("Invalid param");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	/* Initialized the output stream (signed stream) */
	EM_SAFE_FREE(passphrase);
	passphrase = EM_SAFE_STRDUP(password);

#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif
	g_mime_init(0);

	SNPRINTF(temp_pgp_filepath, sizeof(temp_pgp_filepath), "%s%s%s", MAILTEMP, DIR_SEPERATOR, PGP_SIGNED_FILE);
	EM_DEBUG_LOG_SEC("attachment file path of pgp : [%s]", temp_pgp_filepath);

	err = em_open(temp_pgp_filepath, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH, &gpg_fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed : [%s] [%d]", temp_pgp_filepath, err);
		goto FINISH_OFF;
	}

	signed_text = g_mime_stream_fs_new(gpg_fd);
	if (g_mime_stream_reset(signed_text) == -1) {
		EM_DEBUG_EXCEPTION("g_mime_stream_reset signed_text failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ctx = g_mime_gpg_context_new(request_passwd, "/usr/bin/gpg");
	g_mime_gpg_context_set_always_trust ((GMimeGpgContext *)ctx, true);

	/* Initialized the input stream (clear text stream) */
	EM_DEBUG_LOG("mime_entity : [%s]", mime_entity);
	err = em_open(mime_entity, O_RDONLY, 0, &clear_fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed : [%s] [%d]", mime_entity, err);
		goto FINISH_OFF;
	}

	clear_text = g_mime_stream_fs_new(clear_fd);
	if (g_mime_stream_reset(clear_text) == -1) {
		EM_DEBUG_EXCEPTION("g_mime_stream_reset clear_text failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Get the digest type of gmime */
	p_digest_type = emcore_pgp_get_gmime_digest_algo(digest_type);

	/* Set the signed message */
	if ((g_mime_crypto_context_sign(ctx, 
									user_id, 
									p_digest_type, 
									clear_text, 
									signed_text, 
									&g_err) < 0) && (g_err != NULL)) {
		EM_DEBUG_EXCEPTION("g_mime_crypto_context_sign failed : [%s]", g_err->message);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

FINISH_OFF:

	if (g_err)
		g_error_free(g_err);

	if (clear_text)
		g_object_unref(clear_text);

	if (signed_text)
		g_object_unref(signed_text);

	if (ctx)
		g_object_unref(ctx);

	g_mime_shutdown();

	close(clear_fd);
	close(gpg_fd);

	if (file_path)
		*file_path = EM_SAFE_STRDUP(temp_pgp_filepath);

	return err;	
#else /* __FEATURE_SECURE_PGP__ */

	return EMAIL_ERROR_NOT_SUPPORTED;

#endif
}

INTERNAL_FUNC int emcore_pgp_set_encrypted_message(char *recipient_list, 
													char *certificate,
													char *password,
													char *mime_entity, 
													char *user_id, 
													email_digest_type digest_type, 
													char **file_path)
{
	EM_DEBUG_FUNC_BEGIN_SEC("Certificate path : [%s], mime_entity : [%s]", certificate, mime_entity);

#ifdef __FEATURE_SECURE_PGP__
	int err = EMAIL_ERROR_NONE;
	int clear_fd = 0;
	int gpg_fd = 0;
	int p_digest_type = 0;
	char temp_pgp_filepath[512] = {0, };

	GPtrArray *recipients = NULL;

	GMimeCryptoContext *ctx     = NULL;
	GMimeStream *clear_text     = NULL;
	GMimeStream *encrypted_text = NULL;
	GError *g_err               = NULL;

	if (!recipient_list || !mime_entity || !user_id) {
		EM_DEBUG_EXCEPTION("Invalid param");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	/* Initialized the output stream (signed stream) */
#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif
	g_mime_init(0);

	SNPRINTF(temp_pgp_filepath, sizeof(temp_pgp_filepath), "%s%s%s", MAILTEMP, DIR_SEPERATOR, PGP_ENCRYPTED_FILE);
	EM_DEBUG_LOG_SEC("attachment file path of pgp : [%s]", temp_pgp_filepath);

	err = em_open(temp_pgp_filepath, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH, &gpg_fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed : [%s] [%d]", temp_pgp_filepath, err);
		goto FINISH_OFF;
	}

	encrypted_text = g_mime_stream_fs_new(gpg_fd);
	if (g_mime_stream_reset(encrypted_text) == -1) {
		EM_DEBUG_EXCEPTION("g_mime_stream_reset encrypted_text failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ctx = g_mime_gpg_context_new(request_passwd, "/usr/bin/gpg");
	g_mime_gpg_context_set_always_trust ((GMimeGpgContext *)ctx, true);

	/* Initialized the input stream (clear text stream) */
	EM_DEBUG_LOG("mime_entity : [%s]", mime_entity);
	err = em_open(mime_entity, O_RDONLY, 0, &clear_fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed : [%s] [%d]", mime_entity, err);
		goto FINISH_OFF;
	}

	clear_text = g_mime_stream_fs_new(clear_fd);
	if (g_mime_stream_reset(clear_text) == -1) {
		EM_DEBUG_EXCEPTION("g_mime_stream_reset clear_text failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Get the digest type of gmime */
	p_digest_type = emcore_pgp_get_gmime_digest_algo(digest_type);

	/* Set the recipients list */
	if ((err = get_stack_of_recipients(recipient_list, &recipients)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("get_stack_of_recipients failed : [%d]", err);
		goto FINISH_OFF;
	}

	/* Set the signed message */
	if ((g_mime_crypto_context_encrypt(ctx, 
										false, 
										user_id, 
										p_digest_type, 
										recipients, 
										clear_text, 
										encrypted_text, 
										&g_err) < 0) && (g_err != NULL)) {
		EM_DEBUG_EXCEPTION("NO signature : g_mime_crypto_context_encrypt failed : [%s]", g_err->message);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

FINISH_OFF:

	g_ptr_array_free(recipients, true);

	if (g_err)
		g_error_free(g_err);

	if (clear_text)
		g_object_unref(clear_text);

	if (encrypted_text)
		g_object_unref(encrypted_text);

	if (ctx)
		g_object_unref(ctx);

	g_mime_shutdown();

	close(clear_fd);
	close(gpg_fd);

	if (file_path)
		*file_path = EM_SAFE_STRDUP(temp_pgp_filepath);

	return err;	
#else /* __FEATURE_SECURE_PGP__ */

	return EMAIL_ERROR_NOT_SUPPORTED;

#endif

}

INTERNAL_FUNC int emcore_pgp_set_signed_and_encrypted_message(char *recipient_list, 
																char *certificate, 
																char *password, 
																char *mime_entity, 
																char *user_id, 
																email_digest_type digest_type, 
																char **file_path)
{
	EM_DEBUG_FUNC_BEGIN_SEC("mime_entity : [%s]", mime_entity);

#ifdef __FEATURE_SECURE_PGP__
	int err                     = EMAIL_ERROR_NONE;
	int clear_fd                = 0;
	int gpg_fd                  = 0;
	int p_digest_type           = 0;
	char temp_pgp_filepath[512] = {0, };

	GPtrArray *recipients       = NULL;
	GMimeCryptoContext *ctx     = NULL;
	GMimeStream *clear_text     = NULL;
	GMimeStream *encrypted_text = NULL;
	GError *g_err               = NULL;

	if (!recipient_list || !password || !mime_entity || !user_id) {
		EM_DEBUG_EXCEPTION("Invalid param");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	/* Initialized the output stream (signed stream) */
	EM_SAFE_FREE(passphrase);
	passphrase = strdup(password);

#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif
	g_mime_init(0);

	SNPRINTF(temp_pgp_filepath, sizeof(temp_pgp_filepath), "%s%s%s", MAILTEMP, DIR_SEPERATOR, PGP_ENCRYPTED_FILE);
	EM_DEBUG_LOG_SEC("attachment file path of pgp : [%s]", temp_pgp_filepath);

	err = em_open(temp_pgp_filepath, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH, &gpg_fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed : [%s], [%d]", temp_pgp_filepath, err);
		goto FINISH_OFF;
	}

	encrypted_text = g_mime_stream_fs_new(gpg_fd);
	if (g_mime_stream_reset(encrypted_text) == -1) {
		EM_DEBUG_EXCEPTION("g_mime_stream_reset encrypted_text failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ctx = g_mime_gpg_context_new(request_passwd, "/usr/bin/gpg");
	g_mime_gpg_context_set_always_trust ((GMimeGpgContext *)ctx, true);

	/* Initialized the input stream (clear text stream) */
	EM_DEBUG_LOG("mime_entity : [%s]", mime_entity);
	err = em_open(mime_entity, O_RDONLY, 0, &clear_fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed : [%s] [%d]", mime_entity, err);
		goto FINISH_OFF;
	}

	clear_text = g_mime_stream_fs_new(clear_fd);
	if (g_mime_stream_reset(clear_text) == -1) {
		EM_DEBUG_EXCEPTION("g_mime_stream_reset clear_text failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Get the digest type of gmime */
	p_digest_type = emcore_pgp_get_gmime_digest_algo(digest_type);

	/* Set the recipients list */
	if ((err = get_stack_of_recipients(recipient_list, &recipients)) != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("get_stack_of_recipients failed : [%d]", err);
		goto FINISH_OFF;
	}

	/* Set the signed message */
	if ((g_mime_crypto_context_encrypt(ctx, 
										true, 
										user_id, 
										p_digest_type, 
										recipients, 
										clear_text, 
										encrypted_text, 
										&g_err) < 0) && (g_err != NULL)) {
		EM_DEBUG_EXCEPTION("Signature : g_mime_crypto_context_encrypt failed : [%s]", g_err->message);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

FINISH_OFF:

	g_ptr_array_free(recipients, true);

	if (g_err)
		g_error_free(g_err);

	if (clear_text)
		g_object_unref(clear_text);

	if (encrypted_text)
		g_object_unref(encrypted_text);

	if (ctx)
		g_object_unref(ctx);

	g_mime_shutdown();

	close(clear_fd);
	close(gpg_fd);

	if (file_path)
		*file_path = EM_SAFE_STRDUP(temp_pgp_filepath);

	return err;	
#else /* __FEATURE_SECURE_PGP__ */

	return EMAIL_ERROR_NOT_SUPPORTED;

#endif

}

INTERNAL_FUNC int emcore_pgp_get_verify_signature(char *signature_path, 
													char *mime_entity, 
													email_digest_type digest_type, 
													int *verify)
{
	EM_DEBUG_FUNC_BEGIN_SEC("signature path : [%s], mime_entity : [%s]", signature_path, mime_entity);

#ifdef __FEATURE_SECURE_PGP__
	int err = EMAIL_ERROR_NONE;
	int clear_fd = 0;
	int signed_fd = 0;
	int p_digest_type = 0;
	int p_verify = false;

	GMimeCryptoContext *ctx        = NULL;
	GMimeStream *clear_text        = NULL;
	GMimeStream *signed_text       = NULL;
	GMimeSignatureList *signatures = NULL;
	GError *g_err                  = NULL;

	if (!signature_path || !mime_entity) {
		EM_DEBUG_EXCEPTION("Invalid param");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	/* Initialized the Context */
#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif
	g_mime_init(0);

	ctx = g_mime_gpg_context_new(request_passwd, "/usr/bin/gpg");
	g_mime_gpg_context_set_always_trust ((GMimeGpgContext *)ctx, true);

	/* Initialized the input stream (clear text stream) */
	EM_DEBUG_LOG("mime_entity : [%s]", mime_entity);
	err = em_open(mime_entity, O_RDONLY, 0, &clear_fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed : [%s] [%d]", mime_entity, err);
		goto FINISH_OFF;
	}

	clear_text = g_mime_stream_fs_new(clear_fd);
	if (g_mime_stream_reset(clear_text) == -1) {
		EM_DEBUG_EXCEPTION("g_mime_stream_reset clear_text failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Initialized the output stream (signed stream) */
	EM_DEBUG_LOG("signature_path : [%s]", signature_path);
	err = em_open(signature_path, O_RDONLY, 0, &signed_fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed : [%s] [%d]", signature_path, err);
		goto FINISH_OFF;
	}
	
	signed_text = g_mime_stream_fs_new(signed_fd);
	if (g_mime_stream_reset(signed_text) == -1) {
		EM_DEBUG_EXCEPTION("g_mime_stream_reset signed_text failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Get the digest type of gmime */
	p_digest_type = emcore_pgp_get_gmime_digest_algo(digest_type);

	/* Verify the signature */
	signatures = g_mime_crypto_context_verify(ctx, p_digest_type, clear_text, signed_text, &g_err);
	if (signatures == NULL) {
		EM_DEBUG_EXCEPTION("g_mime_crypto_context_verify failed : [%s]", g_err->message);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (get_signature_status(signatures) != GMIME_SIGNATURE_STATUS_GOOD) {
		EM_DEBUG_LOG("Invalid the signature");
		goto FINISH_OFF;
	}

	p_verify = true;

FINISH_OFF:

	if (g_err)
		g_error_free(g_err);

	if (signatures)
		g_object_unref(signatures);

	if (clear_text)
		g_object_unref(clear_text);

	if (signed_text)
		g_object_unref(signed_text);

	if (ctx)
		g_object_unref(ctx);

	g_mime_shutdown();

	close(clear_fd);
	close(signed_fd);

	if (verify)
		*verify = p_verify;

	return err;	
#else /* __FEATURE_SECURE_PGP__ */

	return EMAIL_ERROR_NOT_SUPPORTED;

#endif

}

INTERNAL_FUNC int emcore_pgp_get_decrypted_message(char *encrypted_message, 
													char *password, 
													int sign, 
													char **decrypted_file, 
													int *verify)
{
	EM_DEBUG_FUNC_BEGIN_SEC("Encrypted message : [%s], password : [%s]", encrypted_message, password);

#ifdef __FEATURE_SECURE_PGP__
	int err                         = EMAIL_ERROR_NONE;
	int p_verify                    = false;
	int decrypted_fd                = 0;
	int encrypted_fd                = 0;
	char temp_decrypt_filepath[512] = {0, };

	GError *g_err                   = NULL;
	GMimeCryptoContext *ctx         = NULL;
	GMimeDecryptResult *result      = NULL;
	GMimeStream *encrypted_text     = NULL;
	GMimeStream *decrypted_text     = NULL;

	if (!encrypted_message && !password) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	EM_SAFE_FREE(passphrase);
	passphrase = EM_SAFE_STRDUP(password);

#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif
	g_mime_init(0);

	/* Initialized the context */
	ctx = g_mime_gpg_context_new(request_passwd, "/usr/bin/gpg");
	g_mime_gpg_context_set_always_trust ((GMimeGpgContext *)ctx, true);

	/* Initialized the input stream (clear text stream) */
	err = em_open(encrypted_message, O_RDONLY, 0, &encrypted_fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed : [%s] [%d]", encrypted_message, err);
		goto FINISH_OFF;
	}

	encrypted_text = g_mime_stream_fs_new(encrypted_fd);
	if (g_mime_stream_reset(encrypted_text) == -1) {
		EM_DEBUG_EXCEPTION("g_mime_stream_reset encrypted_text failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Initialized the output stream (signed stream) */
	SNPRINTF(temp_decrypt_filepath, sizeof(temp_decrypt_filepath), "%s%s%s", MAILTEMP, DIR_SEPERATOR, DECRYPTED_TEMP_FILE);
	EM_DEBUG_LOG_SEC("tmp decrypt file path : [%s]", temp_decrypt_filepath);

	err = em_open(temp_decrypt_filepath, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH, &decrypted_fd);
	if (err != EMAIL_ERROR_NONE) {
		EM_DEBUG_EXCEPTION("em_open failed : [%s] [%d]", temp_decrypt_filepath, err);
		goto FINISH_OFF;
	}

	decrypted_text = g_mime_stream_fs_new(decrypted_fd);
	if (g_mime_stream_reset(decrypted_text) == -1) {
		EM_DEBUG_EXCEPTION("g_mime_stream_reset decrypted_text failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Get the decrypt message */
	result = g_mime_crypto_context_decrypt(ctx, encrypted_text, decrypted_text, &g_err);
	if (result == NULL) {
		EM_DEBUG_EXCEPTION("g_mime_crypto_context_decrypt failed : [%s]", g_err->message);
		err = EMAIL_ERROR_DECRYPT_FAILED;
		goto FINISH_OFF;
	}

	if (result->signatures) {
		if (get_signature_status(result->signatures) != GMIME_SIGNATURE_STATUS_GOOD)
			p_verify = false;
		else
			p_verify = true;
	} else {
		   p_verify = -1;
	}

FINISH_OFF:

	if (g_err)
		g_error_free(g_err);

	if (ctx)
		g_object_unref(ctx);

	if (encrypted_text)
		g_object_unref(encrypted_text);

	if (decrypted_text)
		g_object_unref(decrypted_text);

	if (result)
		g_object_unref(result);

	g_mime_shutdown();

	close(encrypted_fd);
	close(decrypted_fd);

	if (verify)
		*verify = p_verify;
	
	if (decrypted_file)
		*decrypted_file = EM_SAFE_STRDUP(temp_decrypt_filepath);

	return err;	
#else /* __FEATURE_SECURE_PGP__ */

	return EMAIL_ERROR_NOT_SUPPORTED;

#endif
}

