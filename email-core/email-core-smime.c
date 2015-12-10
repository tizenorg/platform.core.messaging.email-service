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

#include <openssl/pkcs7.h>
#include <openssl/pkcs12.h>
#include <openssl/buffer.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "email-utilities.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-smtp.h"
#include "email-storage.h"
#include "email-core-smime.h"
#include "email-core-pgp.h"
#include "email-core-cert.h"
#include "email-core-key-manager.h"
#include "email-debug-log.h"

#define SMIME_SIGNED_FILE "smime.p7s"
#define SMIME_ENCRYPT_FILE "smime.p7m"
#define DECRYPT_TEMP_FILE "decrypt_temp_file.eml"

/* If not present then the default digest algorithm for signing key will be used SHA1 */
static const EVP_MD *emcore_get_digest_algorithm(email_digest_type digest_type)
{
	const EVP_MD *digest_algo = NULL;

	switch (digest_type) {
	case DIGEST_TYPE_MD5:
		digest_algo = EVP_md5();
		break;
	case DIGEST_TYPE_SHA1:
	default:
		digest_algo = EVP_sha1();
		break;
	}

	return digest_algo;
}

/* If not present then the default cipher algorithm for signing key will be used RC2(40) */
static const EVP_CIPHER *emcore_get_cipher_algorithm(email_cipher_type cipher_type)
{
	const EVP_CIPHER *cipher = NULL;

	switch (cipher_type) {
	case CIPHER_TYPE_RC2_128:
		cipher = EVP_rc2_cbc();
		break;
	case CIPHER_TYPE_RC2_64:
		cipher = EVP_rc2_64_cbc();
		break;
	case CIPHER_TYPE_DES3:
		cipher = EVP_des_ede3_cbc();
		break;
	case CIPHER_TYPE_DES:
		cipher = EVP_des_cbc();
		break;
#ifdef __FEATURE_USE_MORE_CIPHER_TYPE__
	case CIPHER_TYPE_SEED:
		cipher = EVP_seed_cbc();
		break;
	case CIPHER_TYPE_AES128:
		cipher = EVP_aes_128_cbc();
		break;
	case CIPHER_TYPE_AES192:
		cipher = EVP_aes_192_cbc();
		break;
	case CIPHER_TYPE_AES256:
		cipher = EVP_aes_256_cbc();
		break;
#ifndef OPENSSL_NO_CAMELLIA
	case CIPHER_TYPE_CAMELLIA128:
		cipher = EVP_camellia_128_cbc();
		break;
	case CIPHER_TYPE_CAMELLIA192:
		cipher = EVP_camellia_192_cbc();
		break;
	case CIPHER_TYPE_CAMELLIA256:
		cipher = EVP_camellia_256_cbc();
		break;
#endif
#endif
	case CIPHER_TYPE_RC2_40:
	default:
		cipher = EVP_rc2_40_cbc();
		break;
	}

	return cipher;
}

static int get_x509_stack_of_recipient_certs(char *multi_user_name,
											char *recipients,
											STACK_OF(X509) **output_recipient_certs,
											int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("recipients : [%s], STACK_OF(X509) : [%p]", recipients, output_recipient_certs);

	int err = EMAIL_ERROR_NONE;
	int ret = false;
	int i = 0, j = 0;
	int cert_size = 0;
	char *temp_recipients = NULL;
	const unsigned char *in_cert = NULL;

	ADDRESS *token_address = NULL;

	X509 *x509_cert = NULL;
	STACK_OF(X509) *temp_recipient_certs = NULL;

	if (!recipients || !output_recipient_certs) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* Initialize the variable */
	temp_recipient_certs = sk_X509_new_null();

	temp_recipients = g_strdup(recipients);

	for (i = 0, j = EM_SAFE_STRLEN(temp_recipients); i < j; i++)
		if (temp_recipients[i] == ';') temp_recipients[i] = ',';

	rfc822_parse_adrlist(&token_address, temp_recipients, NULL);

	while (token_address) {
		EM_DEBUG_LOG_SEC("email_address_mailbox : [%s], email_address_host : [%s]", token_address->mailbox,
																					token_address->host);
		/* Plan : Certificate load to using key-manager */
		err = emcore_get_certificate_in_key_manager(token_address->host, NULL, &in_cert, &cert_size);
		if (err != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_get_certificate_in_key_manager failed : [%d]", err);
			goto FINISH_OFF;
		}

		if (d2i_X509(&x509_cert, &in_cert, cert_size) == NULL) {
			EM_DEBUG_EXCEPTION("d2i_X509 failed");
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		if (!sk_X509_push(temp_recipient_certs, x509_cert)) {
			EM_DEBUG_EXCEPTION("sk_X509_push failed");
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		x509_cert = NULL;
		token_address = token_address->next;
	}

	*output_recipient_certs = temp_recipient_certs;

	ret = true;

FINISH_OFF:

	if (!ret) {
		if (temp_recipient_certs)
			sk_X509_pop_free(temp_recipient_certs, X509_free);

		if (x509_cert)
			X509_free(x509_cert);
	}

	EM_SAFE_FREE(in_cert);
	EM_SAFE_FREE(temp_recipients);
	if (token_address)
		mail_free_address(&token_address);

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END("err : [%d]", err);
	return ret;
}

/* Opaque signed and encrypted method */
/*
static PKCS7 *opaque_signed_and_encrypt(STACK_OF(X509) *recipients_cert,
										X509 *signer,
										EVP_PKEY *private_key,
										BIO *mime_entity,
										const EVP_CIPHER *cipher,
										const EVP_MD *md,
										int flags)
{
	EM_DEBUG_FUNC_BEGIN();
	int i = 0;
	PKCS7 *pkcs7 = NULL;
	BIO *p7bio = NULL;
	X509 *x509;

	if (!(pkcs7 = PKCS7_new())) {
		EM_DEBUG_EXCEPTION("PKCS7 malloc failed");
		return NULL;
	}

	if (!PKCS7_set_type(pkcs7, NID_pkcs7_signedAndEnveloped)) {
		EM_DEBUG_EXCEPTION("Set type failed");
		goto FINISH_OFF;
	}

	if (!PKCS7_add_signature(pkcs7, signer, private_key, md)) {
		EM_DEBUG_EXCEPTION("PKCS7_add_signature failed");
		goto FINISH_OFF;
	}

	if (!PKCS7_add_certificate(pkcs7, signer)) {
		EM_DEBUG_EXCEPTION("PKCS7_add_certificate failed");
		goto FINISH_OFF;
	}

	for (i = 0; i < sk_X509_num(recipients_cert); i++) {
		x509 = sk_X509_value(recipients_cert, i);
		if (!PKCS7_add_recipient(pkcs7, x509)) {
			EM_DEBUG_EXCEPTION("PKCS7_add_recipient failed");
			goto FINISH_OFF;
		}
	}

	if (!PKCS7_set_cipher(pkcs7, cipher)) {
		EM_DEBUG_EXCEPTION("Cipher failed");
		goto FINISH_OFF;
	}

	if (flags & PKCS7_STREAM)
		return pkcs7;

	if (PKCS7_final(pkcs7, mime_entity, flags))
		return pkcs7;

FINISH_OFF:
	BIO_free_all(p7bio);
	PKCS7_free(pkcs7);
	return NULL;
}
*/

INTERNAL_FUNC int emcore_smime_set_signed_message(char *certificate,
												char *mime_entity,
												email_digest_type digest_type,
												char **file_path,
												int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("certificate path : [%s], mime_entity : [%s]", certificate, mime_entity);
	int err, ret = false;
	char temp_smime_filepath[512];
	X509 *cert = NULL;
	STACK_OF(X509) *other_certs = NULL;
	EVP_PKEY *private_key = NULL;
	const EVP_MD *digest = NULL;
	BIO *bio_mime_entity = NULL, *bio_cert = NULL, *bio_prikey = NULL;
	BIO *smime_attachment = NULL;
	PKCS7 *signed_message = NULL;
	int flags = PKCS7_DETACHED | PKCS7_PARTIAL;

	SNPRINTF(temp_smime_filepath, sizeof(temp_smime_filepath), "%s%s%s", MAILTEMP, DIR_SEPERATOR, SMIME_SIGNED_FILE);
	EM_DEBUG_LOG_SEC("attachment file path of smime : [%s]", temp_smime_filepath);

	smime_attachment = BIO_new_file(temp_smime_filepath, OUTMODE);
	if (!smime_attachment) {
		EM_DEBUG_EXCEPTION_SEC("Cannot open output file %s", temp_smime_filepath);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Load certificate for getting the certificate and private key */
	if (!emcore_load_PFX_file(certificate, &private_key, &cert, &other_certs, &err)) {
		EM_DEBUG_EXCEPTION("Load the private certificate failed : [%d]", err);
		goto FINISH_OFF;
	}

	bio_mime_entity = BIO_new_file(mime_entity, READMODE);
	if (!bio_mime_entity) {
		EM_DEBUG_EXCEPTION("Cannot open file[%s]", mime_entity);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	signed_message = PKCS7_sign(NULL, NULL, other_certs, bio_mime_entity, flags);
	if (!signed_message) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("Error creating PKCS#7 structure : [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Get the digest algorithm */
	digest = emcore_get_digest_algorithm(digest_type);

	if (!PKCS7_sign_add_signer(signed_message, cert, private_key, digest, flags)) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("PKCS7_sign_add_signer failed : [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (!PKCS7_final(signed_message, bio_mime_entity, flags)) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("PKCS7_final failed : [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (!i2d_PKCS7_bio_stream(smime_attachment, signed_message, bio_mime_entity, flags)) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("i2d_PKCS7_bio_stream failed : [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	BIO_flush(smime_attachment);
	ret = true;

FINISH_OFF:
	if (file_path && ret)
		*file_path = g_strdup(temp_smime_filepath);

	X509_free(cert);
	sk_X509_pop_free(other_certs, X509_free);
	EVP_PKEY_free(private_key);
	PKCS7_free(signed_message);

	BIO_free(bio_mime_entity);
	BIO_free(bio_cert);
	BIO_free(bio_prikey);
	BIO_free_all(smime_attachment);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

INTERNAL_FUNC int emcore_smime_set_encrypt_message(char *multi_user_name,
													char *recipient_list,
													char *mime_entity,
													email_cipher_type cipher_type,
													char **file_path,
													int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("certificate path : [%p], mime_entity : [%p]", recipient_list, mime_entity);
	char temp_smime_filepath[512];
	int err = EMAIL_ERROR_NONE, ret = false;
//	int flags = PKCS7_DETACHED | PKCS7_STREAM;
	int flags = PKCS7_BINARY;

	STACK_OF(X509) *recipient_certs = NULL;
	X509 *cert = NULL;
	BIO *bio_mime_entity = NULL, *bio_cert = NULL;
	BIO *smime_attachment = NULL;
	PKCS7 *encrypt_message = NULL;
	const EVP_CIPHER *cipher = NULL;

	SNPRINTF(temp_smime_filepath, sizeof(temp_smime_filepath), "%s%s%s", MAILTEMP, DIR_SEPERATOR, SMIME_ENCRYPT_FILE);
	EM_DEBUG_LOG_SEC("attachment file path of smime : [%s]", temp_smime_filepath);

	smime_attachment = BIO_new_file(temp_smime_filepath, OUTMODE);
	if (!smime_attachment) {
		EM_DEBUG_EXCEPTION_SEC("Cannot open output file %s", temp_smime_filepath);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (!get_x509_stack_of_recipient_certs(multi_user_name, recipient_list, &recipient_certs, &err)) {
		EM_DEBUG_EXCEPTION("get_x509_stack_of_recipient_certs failed [%d]", err);
		goto FINISH_OFF;
	}

	bio_mime_entity = BIO_new_file(mime_entity, READMODE);
	if (!bio_mime_entity) {
		EM_DEBUG_EXCEPTION("Cannot open file[%s]", mime_entity);
		goto FINISH_OFF;
	}

	/* Get cipher algorithm */
	cipher = emcore_get_cipher_algorithm(cipher_type);

	encrypt_message = PKCS7_encrypt(recipient_certs, bio_mime_entity, cipher, flags);
	if (encrypt_message == NULL) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("PKCS7_encrypt failed [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (!i2d_PKCS7_bio_stream(smime_attachment, encrypt_message, bio_mime_entity, flags)) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("i2d_PKCS7_bio_stream failed : [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	BIO_flush(smime_attachment);

	ret = true;

FINISH_OFF:
	if (file_path && ret)
		*file_path = g_strdup(temp_smime_filepath);

	PKCS7_free(encrypt_message);

	X509_free(cert);
	sk_X509_pop_free(recipient_certs, X509_free);

	BIO_free(bio_cert);
	BIO_free(bio_mime_entity);
	BIO_free_all(smime_attachment);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

INTERNAL_FUNC int emcore_smime_set_signed_and_encrypt_message(char *multi_user_name,
															char *recipient_list,
															char *certificate,
															char *mime_entity,
															email_cipher_type cipher_type,
															email_digest_type digest_type,
															char **file_path,
															int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("certificate path : [%s], mime_entity : [%s]", recipient_list, mime_entity);
	char temp_smime_filepath[512];
	char temp_mime_entity_path[512];
	int err = EMAIL_ERROR_NONE, ret = false;
	int flags = PKCS7_DETACHED | PKCS7_PARTIAL | PKCS7_STREAM;

	STACK_OF(X509) *recipient_certs = NULL;
	STACK_OF(X509) *other_certs = NULL;
	BIO *bio_mime_entity = NULL, *bio_cert = NULL;
	BIO *bio_signed_message = NULL;
	BIO *smime_attachment = NULL;
	PKCS7 *signed_message = NULL;
	PKCS7 *encrypt_message = NULL;
	const EVP_CIPHER *cipher = NULL;
	const EVP_MD *digest = NULL;

	/* Variable for private certificate */
	EVP_PKEY *private_key = NULL;
	X509 *cert = NULL;

	SNPRINTF(temp_smime_filepath, sizeof(temp_smime_filepath), "%s%s%s", MAILTEMP, DIR_SEPERATOR, SMIME_ENCRYPT_FILE);
	EM_DEBUG_LOG_SEC("attachment file path of smime : [%s]", temp_smime_filepath);

	smime_attachment = BIO_new_file(temp_smime_filepath, OUTMODE);
	if (!smime_attachment) {
		EM_DEBUG_EXCEPTION_SEC("Cannot open output file %s", temp_smime_filepath);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Signing the mail */
	/* 1. Load the private certificate */
	if (!emcore_load_PFX_file(certificate, &private_key, &cert, &other_certs, &err)) {
		EM_DEBUG_EXCEPTION("Load the private certificate failed : [%d]", err);
		goto FINISH_OFF;
	}

	/* 2. Read mime entity */
	bio_mime_entity = BIO_new_file(mime_entity, READMODE);
	if (!bio_mime_entity) {
		EM_DEBUG_EXCEPTION("Cannot open file[%s]", mime_entity);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* 3. signing */
	signed_message = PKCS7_sign(NULL, NULL, other_certs, bio_mime_entity, flags);
	if (!signed_message) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("Error creating PKCS#7 structure : [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* 4. Get the digest algorithm */
	digest = emcore_get_digest_algorithm(digest_type);

	/* 5. Apply a digest algorithm */
	if (!PKCS7_sign_add_signer(signed_message, cert, private_key, digest, flags)) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("PKCS7_sign_add_signer failed : [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* 6. Create signing message */
	SNPRINTF(temp_mime_entity_path, sizeof(temp_mime_entity_path), "%s%smime_entity", MAILTEMP, DIR_SEPERATOR);
	EM_DEBUG_LOG_SEC("attachment file path of smime : [%s]", temp_mime_entity_path);

	bio_signed_message = BIO_new_file(temp_mime_entity_path, WRITEMODE);
	if (!bio_signed_message) {
		EM_DEBUG_EXCEPTION_SEC("Cannot open output file %s", temp_mime_entity_path);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (!SMIME_write_PKCS7(bio_signed_message, signed_message, bio_mime_entity,
						flags | SMIME_OLDMIME | SMIME_CRLFEOL)) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("SMIME_write_PKCS7 error : [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	BIO_free(bio_signed_message);

	/* Encrypting the mail */
	/* 1. Get the recipient certs */
	if (!get_x509_stack_of_recipient_certs(multi_user_name, recipient_list, &recipient_certs, &err)) {
		EM_DEBUG_EXCEPTION("get_x509_stack_of_recipient_certs failed [%d]", err);
		goto FINISH_OFF;
	}

	/* 2. Get cipher algorithm */
	cipher = emcore_get_cipher_algorithm(cipher_type);

	flags = PKCS7_BINARY;

	/* 3. Encrypt the signing message */
	bio_signed_message = BIO_new_file(temp_mime_entity_path, READMODE);
	if (!bio_signed_message) {
		EM_DEBUG_EXCEPTION_SEC("Cannot open output file %s", temp_mime_entity_path);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	encrypt_message = PKCS7_encrypt(recipient_certs, bio_signed_message, cipher, flags);
	if (encrypt_message == NULL) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("PKCS7_encrypt failed [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* 4. Write the encrypt message in file */
	if (!i2d_PKCS7_bio_stream(smime_attachment, encrypt_message, bio_mime_entity, flags)) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("i2d_PKCS7_bio_stream failed : [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	BIO_flush(smime_attachment);

	ret = true;

FINISH_OFF:
	if (file_path && ret)
		*file_path = g_strdup(temp_smime_filepath);

	emstorage_delete_file(temp_mime_entity_path, NULL);

	PKCS7_free(signed_message);
	PKCS7_free(encrypt_message);
	EVP_PKEY_free(private_key);

	X509_free(cert);
	sk_X509_pop_free(other_certs, X509_free);
	sk_X509_pop_free(recipient_certs, X509_free);

	BIO_free(bio_cert);
	BIO_free(bio_mime_entity);
	BIO_free(bio_signed_message);
	BIO_free_all(smime_attachment);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

INTERNAL_FUNC int emcore_smime_get_decrypt_message(char *encrypt_message,
													char *certificate,
													char **decrypt_message,
													int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("encrypt_file : [%s], certificate : [%s]", encrypt_message, certificate);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	char temp_decrypt_filepath[512] = {0, };

	X509 *cert = NULL;
	EVP_PKEY *private_key = NULL;
	BIO *infile = NULL, *out_buf = NULL;
	PKCS7 *p7_encrypt_message = NULL;
	STACK_OF(X509) *recipient_certs = NULL;

	/* Load the encrypted message */
	infile = BIO_new_file(encrypt_message, INMODE);
	if (infile == NULL) {
		EM_DEBUG_EXCEPTION("Cannot open output file %s", encrypt_message);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	p7_encrypt_message = d2i_PKCS7_bio(infile, NULL);
	if (!p7_encrypt_message) {
		EM_DEBUG_EXCEPTION("Error reading S/MIME message");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* Initialize the output file for decrypted message */
	SNPRINTF(temp_decrypt_filepath, sizeof(temp_decrypt_filepath), "%s%s%s", MAILTEMP, DIR_SEPERATOR, DECRYPT_TEMP_FILE);
	EM_DEBUG_LOG_SEC("attachment file path of smime : [%s]", temp_decrypt_filepath);

	out_buf = BIO_new_file(temp_decrypt_filepath, OUTMODE);
	if (!out_buf) {
		EM_DEBUG_EXCEPTION_SEC("Cannot open output file %s", temp_decrypt_filepath);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Search private cert */
	if (!emcore_load_PFX_file(certificate, &private_key, &cert, NULL, &err)) {
		EM_DEBUG_EXCEPTION("Load the private certificate failed : [%d]", err);
		goto FINISH_OFF;
	}

	if (!PKCS7_decrypt(p7_encrypt_message, private_key, cert, out_buf, 0)) {
		unsigned long error = ERR_get_error();
		EM_DEBUG_EXCEPTION("Decrpyt failed : [%ld][%s]", error, ERR_error_string(error, NULL));
		err = EMAIL_ERROR_DECRYPT_FAILED;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (decrypt_message && ret)
		*decrypt_message = g_strdup(temp_decrypt_filepath);

	X509_free(cert);
	EVP_PKEY_free(private_key);
	BIO_free(out_buf);
	BIO_free_all(infile);
	sk_X509_pop_free(recipient_certs, X509_free);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

/*
INTERNAL_FUNC int emcore_smime_verify_signed_message(char *signed_message, char *ca_file, char *ca_path, int *verify)
{
	int ret = false;
	int temp_verify = 0;
	BIO *indata = NULL;
	BIO *content = NULL;
	X509_STORE *store = NULL;
	X509_LOOKUP *lookup = NULL;
	PKCS7 *p7 = NULL;

	if (BIO_write(indata, signed_message, sizeof(signed_message)) <= 0) {
		EM_DEBUG_EXCEPTION("Char to Bio failed");
		goto FINISH_OFF;
	}

	p7 = SMIME_read_PKCS7(indata, &content);
	if (!p7) {
		EM_DEBUG_EXCEPTION("SMIME_read_PKCS7 failed");
		goto FINISH_OFF;
	}

	if (!(store = X509_STORE_new())) {
		EM_DEBUG_EXCEPTION("Initialize x509_store failed");
		goto FINISH_OFF;
	}

	lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file());
	if (lookup == NULL) {
		EM_DEBUG_EXCEPTION("Initialize lookup store failed");
		goto FINISH_OFF;
	}

	if (ca_file) {
		if (!X509_LOOKUP_load_file(lookup, ca_file, X509_FILETYPE_PEM)) {
			EM_DEBUG_EXCEPTION("X509_LOOKUP_load_file failed");
			goto FINISH_OFF;
		}
	} else {
		X509_LOOKUP_load_file(lookup, NULL, X509_FILETYPE_DEFAULT);
	}

	lookup = X509_STORE_add_lookup(store, X509_LOOKUP_hash_dir());
	if (lookup == NULL) {
		EM_DEBUG_EXCEPTION("X509_STORE_add_lookup failed");
		goto FINISH_OFF;
	}

	if (ca_path) {
		if (!X509_LOOKUP_add_dir(lookup, ca_path, X509_FILETYPE_PEM)) {
			EM_DEBUG_EXCEPTION("CA path load failed");
			goto FINISH_OFF;
		}
	} else {
		X509_LOOKUP_add_dir(lookup, NULL, X509_FILETYPE_DEFAULT);
	}

	temp_verify = PKCS7_verify(p7, NULL, store, content, NULL, 0);
	if (temp_verify)
		EM_DEBUG_LOG("Verification Successful\n");

	ret = true;

FINISH_OFF:
	if (store)
		X509_STORE_free(store);
	if (p7)
		PKCS7_free(p7);

	if (indata)
		BIO_free(indata);

	if (verify != NULL)
		*verify = temp_verify;

	ERR_clear_error();
	return ret;
}
*/

INTERNAL_FUNC int emcore_convert_mail_data_to_smime_data(char *multi_user_name,
														emstorage_account_tbl_t *account_tbl_item,
														email_mail_data_t *input_mail_data,
														email_attachment_data_t *input_attachment_data_list,
														int input_attachment_count,
														email_mail_data_t **output_mail_data,
														email_attachment_data_t **output_attachment_data_list,
														int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list [%p], input_attachment_count [%d], output_mail_data [%p], output_attachment_data_list [%p]",
						input_mail_data, input_attachment_data_list, input_attachment_count,
						output_mail_data, output_attachment_data_list);

	int i = 0;
	int err = EMAIL_ERROR_NONE;
	int smime_type = EMAIL_SMIME_NONE;
	int attachment_count = input_attachment_count;
	int file_size = 0;
	char *name = NULL;
	char *smime_file_path = NULL;
	char *other_certificate_list = NULL;
	email_attachment_data_t new_attachment_data = {0};
	email_attachment_data_t *new_attachment_list = NULL;

	/* Validating parameters */
	if (!input_mail_data || !(input_mail_data->account_id) || !(input_mail_data->mailbox_id)
		|| !output_attachment_count || !output_mail_data || !output_attachment_data_list) { /*prevent#53051*/
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		err = EMAIL_ERROR_INVALID_PARAM;
		return err;
	}

	smime_type = input_mail_data->smime_type;
	if (!smime_type)
		smime_type = account_tbl_item->smime_type;

	/* Signed and Encrypt the message */
	switch (smime_type) {
	case EMAIL_SMIME_SIGNED:			/* Clear signed message */
		if (!emcore_smime_set_signed_message(account_tbl_item->certificate_path,
											input_mail_data->file_path_mime_entity,
											account_tbl_item->digest_type,
											&smime_file_path,
											&err)) {
			EM_DEBUG_EXCEPTION("em_core_smime_set_clear_signed_message is failed : [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_SEC("smime_file_path : %s", smime_file_path);
		name = strrchr(smime_file_path, '/');

		new_attachment_data.attachment_name = g_strdup(name + 1);
		new_attachment_data.attachment_path = g_strdup(smime_file_path);
		new_attachment_data.attachment_mime_type = strdup("pkcs7-signature");

		attachment_count += 1;

		break;

	case EMAIL_SMIME_ENCRYPTED:			/* Encryption message */
		other_certificate_list = g_strconcat(input_mail_data->full_address_from, ";",
											 input_mail_data->full_address_to, ";",
											 input_mail_data->full_address_cc, ";",
											 input_mail_data->full_address_bcc, NULL);

		if (!emcore_smime_set_encrypt_message(multi_user_name,
											other_certificate_list,
											input_mail_data->file_path_mime_entity,
											account_tbl_item->cipher_type,
											&smime_file_path,
											&err)) {
			EM_DEBUG_EXCEPTION("emcore_smime_set_encrypt_message is failed : [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_SEC("smime_file_path : %s", smime_file_path);
		name = strrchr(smime_file_path, '/');

		new_attachment_data.attachment_name = g_strdup(name + 1);
		new_attachment_data.attachment_path = g_strdup(smime_file_path);
		new_attachment_data.attachment_mime_type = strdup("pkcs7-mime");

		attachment_count = 1;

		break;

	case EMAIL_SMIME_SIGNED_AND_ENCRYPTED:			/* Signed and Encryption message */
		other_certificate_list = g_strconcat(input_mail_data->full_address_from, ";",
											 input_mail_data->full_address_to, ";",
											 input_mail_data->full_address_cc, ";",
											 input_mail_data->full_address_bcc, NULL);

		if (!emcore_smime_set_signed_and_encrypt_message(multi_user_name,
														other_certificate_list,
														account_tbl_item->certificate_path,
														input_mail_data->file_path_mime_entity,
														account_tbl_item->cipher_type,
														account_tbl_item->digest_type,
														&smime_file_path,
														&err)) {
			EM_DEBUG_EXCEPTION("em_core_smime_set_signed_and_encrypt_message is failed : [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_SEC("smime_file_path : %s", smime_file_path);
		name = strrchr(smime_file_path, '/');

		new_attachment_data.attachment_name = g_strdup(name + 1);
		new_attachment_data.attachment_path = g_strdup(smime_file_path);
		new_attachment_data.attachment_mime_type = strdup("pkcs7-mime");

		attachment_count = 1;

		break;

	case EMAIL_PGP_SIGNED:
		if ((err = emcore_pgp_set_signed_message(NULL,
												input_mail_data->pgp_password,
												input_mail_data->file_path_mime_entity,
												input_mail_data->key_id,
												account_tbl_item->digest_type,
												&smime_file_path)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_pgp_set_signed_message is failed : [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG_SEC("smime_file_path : %s", smime_file_path);
		name = strrchr(smime_file_path, '/');

		new_attachment_data.attachment_name = g_strdup(name + 1);
		new_attachment_data.attachment_path = g_strdup(smime_file_path);
		new_attachment_data.attachment_mime_type = strdup("pgp-signature");

		attachment_count += 1;

		break;

	case EMAIL_PGP_ENCRYPTED:
/*
		other_certificate_list = g_strconcat(input_mail_data->full_address_from, ";",
											 input_mail_data->full_address_to, ";",
											 input_mail_data->full_address_cc, ";",
											 input_mail_data->full_address_bcc, NULL);
*/
		other_certificate_list = g_strdup(input_mail_data->key_list);
		if ((err = emcore_pgp_set_encrypted_message(other_certificate_list,
													NULL,
													input_mail_data->pgp_password,
													input_mail_data->file_path_mime_entity,
													account_tbl_item->user_email_address,
													input_mail_data->digest_type,
													&smime_file_path)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_pgp_set_encrypted_message failed : [%d]", err);
			goto FINISH_OFF;
		}

		name = strrchr(smime_file_path, '/');

		new_attachment_data.attachment_name = g_strdup(name + 1);
		new_attachment_data.attachment_path = g_strdup(smime_file_path);
		new_attachment_data.attachment_mime_type = strdup("octet-stream");

		attachment_count = 1;

		break;

	case EMAIL_PGP_SIGNED_AND_ENCRYPTED:
/*
		other_certificate_list = g_strconcat(input_mail_data->full_address_from, ";",
											 input_mail_data->full_address_to, ";",
											 input_mail_data->full_address_cc, ";",
											 input_mail_data->full_address_bcc, NULL);
*/
		other_certificate_list = g_strdup(input_mail_data->key_list);
		if ((err = emcore_pgp_set_signed_and_encrypted_message(other_certificate_list,
															NULL,
															input_mail_data->pgp_password,
															input_mail_data->file_path_mime_entity,
															input_mail_data->key_id,
															input_mail_data->digest_type,
															&smime_file_path)) != EMAIL_ERROR_NONE) {
			EM_DEBUG_EXCEPTION("emcore_pgp_set_signed_and_encrypted_message failed : [%d]", err);
			goto FINISH_OFF;
		}

		name = strrchr(smime_file_path, '/');

		new_attachment_data.attachment_name = g_strdup(name + 1);
		new_attachment_data.attachment_path = g_strdup(smime_file_path);
		new_attachment_data.attachment_mime_type = strdup("octet-stream");

		attachment_count = 1;

		break;

	default:
		EM_DEBUG_LOG("MIME none");
		break;

	}

	if (!emcore_get_file_size(smime_file_path, &file_size, NULL)) {
		EM_DEBUG_EXCEPTION("emcore_get_file_size failed");
		goto FINISH_OFF;
	}

	new_attachment_data.attachment_size = file_size;
	new_attachment_data.save_status = 1;

	new_attachment_list = (email_attachment_data_t *)em_malloc(sizeof(email_attachment_data_t) * attachment_count);
	if (new_attachment_list == NULL) {
		EM_DEBUG_EXCEPTION("em_mallocfailed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (smime_type ==  EMAIL_SMIME_SIGNED) {
		for (i = 0; i < input_attachment_count; i++) {
			new_attachment_list[i].attachment_id         = input_attachment_data_list[i].attachment_id;
			new_attachment_list[i].attachment_name       = g_strdup(input_attachment_data_list[i].attachment_name);
			new_attachment_list[i].attachment_path       = g_strdup(input_attachment_data_list[i].attachment_path);
			new_attachment_list[i].content_id            = g_strdup(input_attachment_data_list[i].content_id);
			new_attachment_list[i].attachment_size       = input_attachment_data_list[i].attachment_size;
			new_attachment_list[i].mail_id               = input_attachment_data_list[i].mail_id;
			new_attachment_list[i].account_id            = input_attachment_data_list[i].account_id;
			new_attachment_list[i].mailbox_id            = input_attachment_data_list[i].mailbox_id;
			new_attachment_list[i].save_status           = input_attachment_data_list[i].save_status;
			new_attachment_list[i].drm_status            = input_attachment_data_list[i].drm_status;
			new_attachment_list[i].inline_content_status = input_attachment_data_list[i].inline_content_status;
			new_attachment_list[i].attachment_mime_type  = g_strdup(input_attachment_data_list[i].attachment_mime_type);
		}
	}

	new_attachment_list[attachment_count - 1].attachment_id         = new_attachment_data.attachment_id;
	new_attachment_list[attachment_count - 1].attachment_name       = g_strdup(new_attachment_data.attachment_name);
	new_attachment_list[attachment_count - 1].attachment_path       = g_strdup(new_attachment_data.attachment_path);
	new_attachment_list[attachment_count - 1].content_id            = g_strdup(new_attachment_data.content_id);
	new_attachment_list[attachment_count - 1].attachment_size       = new_attachment_data.attachment_size;
	new_attachment_list[attachment_count - 1].mail_id               = new_attachment_data.mail_id;
	new_attachment_list[attachment_count - 1].account_id            = new_attachment_data.account_id;
	new_attachment_list[attachment_count - 1].mailbox_id            = new_attachment_data.mailbox_id;
	new_attachment_list[attachment_count - 1].save_status           = new_attachment_data.save_status;
	new_attachment_list[attachment_count - 1].drm_status            = new_attachment_data.drm_status;
	new_attachment_list[attachment_count - 1].inline_content_status = new_attachment_data.inline_content_status;
	new_attachment_list[attachment_count - 1].attachment_mime_type  = g_strdup(new_attachment_data.attachment_mime_type);

	input_mail_data->smime_type = smime_type;
	input_mail_data->digest_type = account_tbl_item->digest_type;

FINISH_OFF:

	EM_SAFE_FREE(other_certificate_list);
	EM_SAFE_FREE(smime_file_path);

	*output_attachment_count = attachment_count;

	*output_attachment_data_list = new_attachment_list;

	*output_mail_data = input_mail_data;

    EM_SAFE_FREE(new_attachment_data.attachment_name);
    EM_SAFE_FREE(new_attachment_data.attachment_path);
    EM_SAFE_FREE(new_attachment_data.attachment_mime_type);
    EM_SAFE_FREE(new_attachment_data.content_id);

    EM_DEBUG_LOG("err : [%d]", err);
	return err;
}

INTERNAL_FUNC void emcore_init_openssl_library()
{
        EM_DEBUG_FUNC_BEGIN();
        SSL_library_init();
        ERR_load_crypto_strings();
        EM_DEBUG_FUNC_END();
}

INTERNAL_FUNC void emcore_clean_openssl_library()
{
        EM_DEBUG_FUNC_BEGIN();
        ERR_free_strings();
        EVP_cleanup();
        EM_DEBUG_FUNC_END();
}
