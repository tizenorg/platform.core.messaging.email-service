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

#include "email-utilities.h"
#include "email-core-global.h"
#include "email-core-utils.h"
#include "email-core-mail.h"
#include "email-core-smtp.h"
#include "email-storage.h"
#include "email-core-smime.h"
#include "email-core-cert.h"
#include "email-debug-log.h"

/* /opt/share/cert-svc/certs is a base path */

#define SMIME_SIGNED_FILE "smime.p7s"
#define SMIME_ENCRYPT_FILE "smime.p7m"

#define OUTMODE "wb"
#define INMODE "rb"
#define READMODE "r"

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
	case CIPHER_TYPE_RC2_128 :
		cipher = EVP_rc2_cbc();
		break;
	case CIPHER_TYPE_RC2_64 :
		cipher = EVP_rc2_64_cbc();
		break;
	case CIPHER_TYPE_DES3 :
		cipher = EVP_des_ede3_cbc();
		break;
	case CIPHER_TYPE_DES :
		cipher = EVP_des_cbc();
		break;
#ifdef __FEATURE_USE_MORE_CIPHER_TYPE__
	case CIPHER_TYPE_SEED :
		cipher = EVP_seed_cbc();
		break;
	case CIPHER_TYPE_AES128 :
		cipher = EVP_aes_128_cbc();
		break;
	case CIPHER_TYPE_AES192 :
		cipher = EVP_aes_192_cbc();
		break;
	case CIPHER_TYPE_AES256 :
		cipher = EVP_aes_256_cbc();
		break;
#endif
#ifndef OPENSSL_NO_CAMELLIA		
	case CIPHER_TYPE_CAMELLIA128 :
		cipher = EVP_camellia_128_cbc();
		break;
	case CIPHER_TYPE_CAMELLIA192 :
		cipher = EVP_camellia_192_cbc();
		break;
	case CIPHER_TYPE_CAMELLIA256 :
		cipher = EVP_camellia_256_cbc();
		break;
#endif
	case CIPHER_TYPE_RC2_40 :
	default :
		cipher = EVP_rc2_40_cbc();
		break;
	}

	return cipher;
}

static int get_x509_stack_of_recipient_certs(char *recipients, STACK_OF(X509) **output_recipient_certs, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("recipients : [%s], STACK_OF(X509) : [%p]", recipients, output_recipient_certs);

	int err = EMAIL_ERROR_NONE;
	int ret = false;	
	int cert_size = 0;
	char *temp_recipients = NULL;
	char *token = NULL;
	char *str = NULL;
	char file_name[512] = {0, };
	const unsigned char *in_cert = NULL;

	X509 *x509_cert = NULL;
	STACK_OF(X509) *temp_recipient_certs = NULL;

	CERT_CONTEXT *context = NULL;
	emstorage_certificate_tbl_t *cert = NULL;

	if (!recipients || !output_recipient_certs) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* Initialize the variable */
	context = cert_svc_cert_context_init();
	temp_recipient_certs = sk_X509_new_null();

	temp_recipients = EM_SAFE_STRDUP(recipients);
	token = strtok_r(temp_recipients, ";", &str);

	do {
		if (!emstorage_get_certificate_by_email_address(token, &cert, false, 0, &err)) {
			EM_DEBUG_EXCEPTION("emstorage_get_certificate_by_email_address failed : [%d]", err);
			goto FINISH_OFF;
		}
		
		SNPRINTF(file_name, sizeof(file_name), "%s", cert->filepath);
		EM_DEBUG_LOG("file_name : [%s]", file_name);
		err = cert_svc_load_file_to_context(context, file_name);
		if (err != CERT_SVC_ERR_NO_ERROR) {
			EM_DEBUG_EXCEPTION("cert_svc_load_file_to_context failed : [%d]", err);
			err = EMAIL_ERROR_SYSTEM_FAILURE;
			goto FINISH_OFF;
		}

		in_cert = context->certBuf->data;
		cert_size = context->certBuf->size;

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
		context = NULL;
		emstorage_free_certificate(&cert, 1, NULL);
		cert = NULL;
	} while ((token = strtok_r(NULL, ";", &str)));

	*output_recipient_certs = temp_recipient_certs;

	ret = true;

FINISH_OFF:

	if (!ret) {
		if (x509_cert)
			X509_free(x509_cert);

		if (temp_recipient_certs)
			sk_X509_pop_free(temp_recipient_certs, X509_free);
	}

	if (cert)
		emstorage_free_certificate(&cert, 1, NULL);

	cert_svc_cert_context_final(context);

	EM_SAFE_FREE(temp_recipients);

	if (err_code)
		*err_code = err;

	return ret;
}

/* Opaque signed and encrypted method */
/*
static PKCS7 *opaque_signed_and_encrypt(STACK_OF(X509) *recipients_cert, X509 *signer, EVP_PKEY *private_key, BIO *mime_entity, const EVP_CIPHER *cipher, const EVP_MD *md, int flags)
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
#define SMIME_DEBUG 1

#ifdef SMIME_DEBUG
#define TEMP_PASSWORD_PATH "/opt/data/cert_password"
#endif

INTERNAL_FUNC int emcore_smime_set_signed_message(char *certificate, char *password, char *mime_entity, email_digest_type digest_type, char **file_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("certificate path : [%s], password : [%s], mime_entity : [%s]", certificate, password, mime_entity);
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

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	SNPRINTF(temp_smime_filepath, sizeof(temp_smime_filepath), "%s%s%s%s%s", MAILHOME, DIR_SEPERATOR, MAILTEMP, DIR_SEPERATOR, SMIME_SIGNED_FILE);
	EM_DEBUG_LOG("attachment file path of smime : [%s]", temp_smime_filepath);

	smime_attachment = BIO_new_file(temp_smime_filepath, OUTMODE);
	if (!smime_attachment) {
		EM_DEBUG_EXCEPTION("Cannot open output file %s", temp_smime_filepath);
		err = EMAIL_ERROR_FILE_NOT_FOUND;
		goto FINISH_OFF;
	}

	/* Load certificate for getting the certificate and private key */
	if (!emcore_load_PFX_file(certificate, password, &private_key, &cert, &other_certs, &err)) {
		EM_DEBUG_EXCEPTION("Load the private certificate failed : [%d]", err);
		goto FINISH_OFF;
	}

	bio_mime_entity = BIO_new_file(mime_entity, READMODE);
	if (!bio_mime_entity) {
		EM_DEBUG_EXCEPTION("Cannot open file[%s]", mime_entity);
		goto FINISH_OFF;
	}

	signed_message = PKCS7_sign(NULL, NULL, other_certs, bio_mime_entity, flags);
	if (!signed_message) {
		EM_DEBUG_EXCEPTION("Error creating PKCS#7 structure");
		goto FINISH_OFF;
	}
	
	/* Get the digest algorithm */
	digest = emcore_get_digest_algorithm(digest_type);

	if (!PKCS7_sign_add_signer(signed_message, cert, private_key, digest, flags)) {
		EM_DEBUG_EXCEPTION("PKCS7_sign_add_signer failed");
		goto FINISH_OFF;
	}

	if (!PKCS7_final(signed_message, bio_mime_entity, flags)) {
		EM_DEBUG_EXCEPTION("PKCS7_final failed");
		goto FINISH_OFF;
	}

	if (!i2d_PKCS7_bio_stream(smime_attachment, signed_message, bio_mime_entity, flags)) {
		EM_DEBUG_EXCEPTION("i2d_PKCS7_bio_stream failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	BIO_flush(smime_attachment);
#ifdef SMIME_DEBUG
	BIO *out = NULL;
	
	out = BIO_new_file("/opt/data/email/.emfdata/tmp/smout.txt", "w");
	if (!out)
		goto FINISH_OFF;

	if (!SMIME_write_PKCS7(out, signed_message, bio_mime_entity, flags))
		goto FINISH_OFF;

	if (out)
		BIO_free(out);
#endif
	ret = true;

FINISH_OFF:
	if (file_path)
		*file_path = temp_smime_filepath;

	X509_free(cert);
	EVP_PKEY_free(private_key);
	PKCS7_free(signed_message);

	BIO_free(bio_mime_entity);
	BIO_free(bio_cert);
	BIO_free(bio_prikey);
	BIO_free_all(smime_attachment);

	EVP_cleanup();
	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

INTERNAL_FUNC int emcore_smime_set_encrypt_message(char *recipient_list, char *mime_entity, email_cipher_type cipher_type, char **file_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("certificate path : [%p], mime_entity : [%p]", recipient_list, mime_entity);
	char temp_smime_filepath[512];
	int err = EMAIL_ERROR_NONE, ret = false;
//	int flags = PKCS7_DETACHED | PKCS7_STREAM;
	int flags = 0;

	CERT_CONTEXT *loaded_cert = NULL;
	STACK_OF(X509) *recipient_certs = NULL;
	X509 *cert = NULL;
	BIO *bio_mime_entity = NULL, *bio_cert = NULL;
	BIO *smime_attachment = NULL;
	PKCS7 *encrypt_message = NULL;
	const EVP_CIPHER *cipher = NULL;

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	loaded_cert = cert_svc_cert_context_init();

	SNPRINTF(temp_smime_filepath, sizeof(temp_smime_filepath), "%s%s%s%s%s", MAILHOME, DIR_SEPERATOR, MAILTEMP, DIR_SEPERATOR, SMIME_ENCRYPT_FILE);
	EM_DEBUG_LOG("attachment file path of smime : [%s]", temp_smime_filepath);

	smime_attachment = BIO_new_file(temp_smime_filepath, OUTMODE);
	if (!smime_attachment) {
		EM_DEBUG_EXCEPTION("Cannot open output file %s", temp_smime_filepath);
		err = EMAIL_ERROR_FILE_NOT_FOUND;
		goto FINISH_OFF;
	}

	if (!get_x509_stack_of_recipient_certs(recipient_list, &recipient_certs, &err)) {
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
		EM_DEBUG_EXCEPTION("PKCS7_encrypt failed [%ld]", ERR_get_error());
		goto FINISH_OFF;
	}

	if (!i2d_PKCS7_bio_stream(smime_attachment, encrypt_message, bio_mime_entity, flags)) {
		EM_DEBUG_EXCEPTION("i2d_PKCS7_bio_stream failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	BIO_flush(smime_attachment);

#ifdef SMIME_DEBUG
	BIO *out = NULL;
	
	out = BIO_new_file("/opt/data/email/.emfdata/tmp/smout.txt", "w");
	if (!out)
		goto FINISH_OFF;

	if (!SMIME_write_PKCS7(out, encrypt_message, bio_mime_entity, flags))
		goto FINISH_OFF;

	if (out)
		BIO_free(out);
#endif

	ret = true;

FINISH_OFF:
	if (file_path)
		*file_path = temp_smime_filepath;

	PKCS7_free(encrypt_message);

	X509_free(cert);
	sk_X509_pop_free(recipient_certs, X509_free);

	BIO_free(bio_cert);
	BIO_free(bio_mime_entity);
	BIO_free_all(smime_attachment);

	cert_svc_cert_context_final(loaded_cert);
	EVP_cleanup();
	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}

INTERNAL_FUNC int emcore_smime_set_signed_and_encrypt_message(char *recipient_list, char *certificate, char *password, char *mime_entity, email_cipher_type cipher_type, email_digest_type digest_type, char **file_path, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("certificate path : [%s], mime_entity : [%s]", recipient_list, mime_entity);
	char temp_smime_filepath[512];
	int err = EMAIL_ERROR_NONE, ret = false;
	int flags = PKCS7_DETACHED | PKCS7_PARTIAL | PKCS7_STREAM;

	STACK_OF(X509) *recipient_certs = NULL;
	STACK_OF(X509) *other_certs = NULL;
	BIO *bio_mime_entity = NULL, *bio_cert = NULL;
	BIO *bio_signed_message = BIO_new(BIO_s_mem());
	BIO *smime_attachment = NULL;
	PKCS7 *signed_message = NULL;
	PKCS7 *encrypt_message = NULL;
	const EVP_CIPHER *cipher = NULL;
	const EVP_MD *digest = NULL;

#ifdef SMIME_DEBUG
	char *signed_string = NULL;
	BUF_MEM *buf_mem = NULL;
#endif
	/* Variable for private certificate */
	EVP_PKEY *private_key = NULL;
	X509 *cert = NULL;

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	SNPRINTF(temp_smime_filepath, sizeof(temp_smime_filepath), "%s%s%s%s%s", MAILHOME, DIR_SEPERATOR, MAILTEMP, DIR_SEPERATOR, SMIME_ENCRYPT_FILE);
	EM_DEBUG_LOG("attachment file path of smime : [%s]", temp_smime_filepath);

	smime_attachment = BIO_new_file(temp_smime_filepath, OUTMODE);
	if (!smime_attachment) {
		EM_DEBUG_EXCEPTION("Cannot open output file %s", temp_smime_filepath);
		err = EMAIL_ERROR_FILE_NOT_FOUND;
		goto FINISH_OFF;
	}

	/* Signing the mail */
	/* 1. Load the private certificate */
	if (!emcore_load_PFX_file(certificate, password, &private_key, &cert, &other_certs, &err)) {
		EM_DEBUG_EXCEPTION("Load the certificate failed : [%d]", err);
		goto FINISH_OFF;
	}

	/* 2. Read mime entity */
	bio_mime_entity = BIO_new_file(mime_entity, READMODE);
	if (!bio_mime_entity) {
		EM_DEBUG_EXCEPTION("Cannot open file[%s]", mime_entity);
		goto FINISH_OFF;
	}

	/* 3. signing */
	signed_message = PKCS7_sign(NULL, NULL, other_certs, bio_mime_entity, flags);
	if (!signed_message) {
		EM_DEBUG_EXCEPTION("Error creating PKCS#7 structure");
		goto FINISH_OFF;
	}
	
	/* 4. Get the digest algorithm */
	digest = emcore_get_digest_algorithm(digest_type);

	/* 5. Apply a digest algorithm */
	if (!PKCS7_sign_add_signer(signed_message, cert, private_key, digest, flags)) {
		EM_DEBUG_EXCEPTION("PKCS7_sign_add_signer failed");
		goto FINISH_OFF;
	}

	/* 6. Create signing message */
	if (!SMIME_write_PKCS7(bio_signed_message, signed_message, bio_mime_entity, flags | SMIME_OLDMIME | SMIME_CRLFEOL)) {
		EM_DEBUG_EXCEPTION("SMIME_write_PKCS7 error");
		goto FINISH_OFF;
	}

#ifdef SMIME_DEBUG
	BIO_get_mem_ptr(bio_signed_message, &buf_mem);
	signed_string = em_malloc(buf_mem->length);
	memcpy(signed_string, buf_mem->data, buf_mem->length-1);
	EM_DEBUG_LOG("Signed message : [%d]", buf_mem->length);
	EM_DEBUG_LOG("%s", signed_string);
	EM_DEBUG_LOG("buf data: \n, %s", buf_mem->data);
	EM_SAFE_FREE(signed_string);
#endif	

	/* Encrypting the mail */
	/* 1. Get the recipient certs */
	if (!get_x509_stack_of_recipient_certs(recipient_list, &recipient_certs, &err)) {
		EM_DEBUG_EXCEPTION("get_x509_stack_of_recipient_certs failed [%d]", err);
		goto FINISH_OFF;
	}

	/* 2. Get cipher algorithm */
	cipher = emcore_get_cipher_algorithm(cipher_type);
	
	flags = 0;

	/* 3. Encrypt the signing message */	
	encrypt_message = PKCS7_encrypt(recipient_certs, bio_signed_message, cipher, flags);
	if (encrypt_message == NULL) {
		EM_DEBUG_EXCEPTION("PKCS7_encrypt failed [%ld]", ERR_get_error());
		goto FINISH_OFF;
	}
	
	/* 4. Write the encrypt message in file */
	if (!i2d_PKCS7_bio_stream(smime_attachment, encrypt_message, bio_mime_entity, flags)) {
		EM_DEBUG_EXCEPTION("i2d_PKCS7_bio_stream failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	BIO_flush(smime_attachment);

#ifdef SMIME_DEBUG
	BIO *out = NULL;
	
	out = BIO_new_file("/opt/data/email/.emfdata/tmp/smout.txt", "w");
	if (!out)
		goto FINISH_OFF;

	if (!SMIME_write_PKCS7(out, encrypt_message, bio_mime_entity, flags))
		goto FINISH_OFF;

	if (out)
		BIO_free(out);
#endif

	ret = true;

FINISH_OFF:
	if (file_path)
		*file_path = temp_smime_filepath;

	PKCS7_free(signed_message);
	PKCS7_free(encrypt_message);

	X509_free(cert);
	sk_X509_pop_free(other_certs, X509_free);
	sk_X509_pop_free(recipient_certs, X509_free);

	BIO_free(bio_cert);
	BIO_free(bio_mime_entity);
	BIO_free(bio_signed_message);
	BIO_free_all(smime_attachment);

	EVP_cleanup();
	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END("err [%d]", err);
	return ret;
}



INTERNAL_FUNC int emcore_smime_set_decrypt_message(char *encrypt_message, char *certificate, char *password, char **decrypt_message, int *err_code)
{
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	long string_len = 0;
	char *temp_decrypt_message;
	FILE *fp = NULL;

	X509 *cert = NULL;
	STACK_OF(X509) *ca = NULL;
	EVP_PKEY *private_key = NULL;
	BIO *bio_cert = NULL, *bio_prikey = NULL;
	BIO *infile = NULL, *out_buf = NULL;
	PKCS7 *p7_encrypt_message = NULL;
	PKCS12 *p12 = NULL;
	CERT_CONTEXT *loaded_cert = NULL;

	infile = BIO_new_file(encrypt_message, INMODE);

	p7_encrypt_message = d2i_PKCS7_bio(infile, NULL);
	if (!p7_encrypt_message) {
		EM_DEBUG_EXCEPTION("Error reading S/MIME message");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	loaded_cert = cert_svc_cert_context_init();

	fp = fopen(certificate, INMODE);
	if (!fp) {
		EM_DEBUG_EXCEPTION("Certificate file open failed");
		goto FINISH_OFF;
	}
	
	p12 = d2i_PKCS12_fp(fp, NULL);
	fclose(fp);

	if (!p12) {
		EM_DEBUG_EXCEPTION("Error reading PKCS#12 file\n");
		goto FINISH_OFF;
	}	

	if (!PKCS12_parse(p12, password, &private_key, &cert, &ca)) {
		EM_DEBUG_EXCEPTION("PKCS12_parse failed");
		goto FINISH_OFF;
	}

	out_buf = BIO_new(BIO_s_mem());
	if (!out_buf) {
		EM_DEBUG_EXCEPTION("There is not enough memory.");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	PKCS7_decrypt(p7_encrypt_message, private_key, cert, out_buf, 0);
	string_len = BIO_get_mem_data(out_buf, &temp_decrypt_message);

	ret = true;

FINISH_OFF:
	*decrypt_message = temp_decrypt_message;

	X509_free(cert);
	BIO_free(out_buf);
	BIO_free(bio_cert);
	BIO_free(bio_prikey);
	BIO_free_all(infile);

	if (err_code != NULL)
		*err_code = err;

	cert_svc_cert_context_final(loaded_cert);
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

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

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
	EVP_cleanup();
	return ret;
}
*/

static char *emcore_get_mime_entity(char *mime_path)
{
	EM_DEBUG_FUNC_BEGIN("mime_path : [%s]", mime_path);
	FILE *fp_read = NULL;
	FILE *fp_write = NULL;
	char *mime_entity = NULL;
	char *mime_entity_path = NULL;
	char temp_buffer[255] = {0,};
	int err;
	int searched = 0;

	if (!emcore_get_temp_file_name(&mime_entity_path, &err))  {
		EM_DEBUG_EXCEPTION(" em_core_get_temp_file_name failed[%d]", err);
		goto FINISH_OFF;
	}

	/* get mime entity */
	if (mime_path != NULL) {
		fp_read = fopen(mime_path, "r");
		if (fp_read == NULL) {
			EM_DEBUG_EXCEPTION("File open(read) is failed : filename [%s]", mime_path);
			goto FINISH_OFF;
		}

		fp_write = fopen(mime_entity_path, "w");
		if (fp_write == NULL) {
			EM_DEBUG_EXCEPTION("File open(write) is failed : filename [%s]", mime_entity_path);
			goto FINISH_OFF;
		}

		fseek(fp_read, 0, SEEK_SET);
		fseek(fp_write, 0, SEEK_SET);           

		while (fgets(temp_buffer, 255, fp_read) != NULL) {
			mime_entity = strcasestr(temp_buffer, "content-type");
			if (mime_entity != NULL && !searched)
				searched = 1;

			if (searched) {
				EM_DEBUG_LOG("temp_buffer : %s", temp_buffer);
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
	EM_SAFE_FREE(mime_path);

	EM_DEBUG_FUNC_END();
	return mime_entity_path;
}

INTERNAL_FUNC int emcore_convert_mail_data_to_smime_data(emstorage_account_tbl_t *account_tbl_item, email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_mail_data_t **output_mail_data, email_attachment_data_t **output_attachment_data_list, int *output_attachment_count)
{
	EM_DEBUG_FUNC_BEGIN("input_mail_data[%p], input_attachment_data_list [%p], input_attachment_count [%d], output_mail_data [%p], output_attachment_data_list [%p]", input_mail_data, input_attachment_data_list, input_attachment_count, output_mail_data, output_attachment_data_list);

	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int smime_type = EMAIL_SMIME_NONE;
	int address_length = 0;
	int attachment_count = input_attachment_count;
	char *name = NULL;
	char *rfc822_file = NULL;
	char *mime_entity = NULL;
	char *smime_file_path = NULL;
	char *other_certificate_list = NULL;
#ifdef SMIME_DEBUG
	char *buf_pointer = NULL;
	char *password = NULL;
	char buf[81] = {0, };
	FILE *fp = NULL;
#endif
	email_attachment_data_t new_attachment_data = {0};
	email_attachment_data_t *temp_attachment_data = NULL;

	/* Validating parameters */
	
	if (!input_mail_data || !(input_mail_data->account_id) || !(input_mail_data->mailbox_id)) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		goto FINISH_OFF;
	}

	
#ifdef SMIME_DEBUG
	fp = fopen(TEMP_PASSWORD_PATH, "r");
	if (fp == NULL) {
		EM_DEBUG_EXCEPTION("Open failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	while (!feof(fp)) {
		buf_pointer = fgets(buf, 80, fp);
	}

	fclose(fp);
	
	EM_DEBUG_LOG("password : [%s], strlen : [%d]", buf, strlen(buf)-1);
	password = em_malloc(sizeof(buf));
	memcpy(password, buf, strlen(buf) - 1);

#endif
	
	if (!emcore_make_rfc822_file(input_mail_data, input_attachment_data_list, attachment_count, &rfc822_file, &err))  {
		EM_DEBUG_EXCEPTION("emcore_make_rfc822_file_from_mail failed [%d]", err);
		goto FINISH_OFF;
	}

	mime_entity = emcore_get_mime_entity(rfc822_file);

	smime_type = input_mail_data->smime_type;
	if (!smime_type) 
		smime_type = account_tbl_item->smime_type;

	/* Signed and Encrypt the message */
	switch (smime_type) {
	case EMAIL_SMIME_SIGNED:			/* Clear signed message */
		if (!emcore_smime_set_signed_message(account_tbl_item->certificate_path, password, mime_entity, account_tbl_item->digest_type, &smime_file_path, &err)) {
			EM_DEBUG_EXCEPTION("em_core_smime_set_clear_signed_message is failed : [%d]", err);
			goto FINISH_OFF;
		}
	
		EM_DEBUG_LOG("smime_file_path : %s", smime_file_path);	
		name = strrchr(smime_file_path, '/');

		new_attachment_data.attachment_name = EM_SAFE_STRDUP(name + 1);
		new_attachment_data.attachment_path = EM_SAFE_STRDUP(smime_file_path);
		new_attachment_data.attachment_size = 1;
		new_attachment_data.save_status = 1;

		attachment_count += 1;

		break;
	case EMAIL_SMIME_ENCRYPTED:			/* Encryption message */
		address_length = EM_SAFE_STRLEN(input_mail_data->full_address_to) + EM_SAFE_STRLEN(input_mail_data->full_address_cc) + EM_SAFE_STRLEN(input_mail_data->full_address_bcc);

		other_certificate_list = em_malloc(address_length + 3);
		
		SNPRINTF(other_certificate_list, address_length + 2, "%s;%s;%s", input_mail_data->full_address_to, input_mail_data->full_address_cc, input_mail_data->full_address_bcc);

		EM_DEBUG_LOG("to:[%s], cc:[%s], bcc:[%s]", input_mail_data->full_address_to, input_mail_data->full_address_cc, input_mail_data->full_address_bcc);
		EM_DEBUG_LOG("length : [%d], email_address : [%s]", address_length, other_certificate_list);

		if (!emcore_smime_set_encrypt_message(other_certificate_list, mime_entity, account_tbl_item->cipher_type, &smime_file_path, &err)) {
			EM_DEBUG_EXCEPTION("emcore_smime_set_encrypt_message is failed : [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("smime_file_path : %s", smime_file_path);	
		name = strrchr(smime_file_path, '/');

		new_attachment_data.attachment_name = EM_SAFE_STRDUP(name + 1);
		new_attachment_data.attachment_path = EM_SAFE_STRDUP(smime_file_path);
		new_attachment_data.attachment_size = 1;
		new_attachment_data.save_status = 1;

		attachment_count += 1;

		break;
	case EMAIL_SMIME_SIGNED_AND_ENCRYPTED:			/* Signed and Encryption message */
		address_length = EM_SAFE_STRLEN(input_mail_data->full_address_to) + EM_SAFE_STRLEN(input_mail_data->full_address_cc) + EM_SAFE_STRLEN(input_mail_data->full_address_bcc);

		other_certificate_list = em_malloc(address_length + 3);

		SNPRINTF(other_certificate_list, address_length + 2, "%s;%s;%s", input_mail_data->full_address_to, input_mail_data->full_address_cc, input_mail_data->full_address_bcc);

		EM_DEBUG_LOG("to:[%s], cc:[%s], bcc:[%s]", input_mail_data->full_address_to, input_mail_data->full_address_cc, input_mail_data->full_address_bcc);
		EM_DEBUG_LOG("length : [%d], email_address : [%s]", address_length, other_certificate_list);

		if (!emcore_smime_set_signed_and_encrypt_message(other_certificate_list, account_tbl_item->certificate_path, password, mime_entity, account_tbl_item->cipher_type, account_tbl_item->digest_type, &smime_file_path, &err)) {
			EM_DEBUG_EXCEPTION("em_core_smime_set_signed_and_encrypt_message is failed : [%d]", err);
			goto FINISH_OFF;
		}

		EM_DEBUG_LOG("smime_file_path : %s", smime_file_path);	
		name = strrchr(smime_file_path, '/');

		new_attachment_data.attachment_name = EM_SAFE_STRDUP(name + 1);
		new_attachment_data.attachment_path = EM_SAFE_STRDUP(smime_file_path);
		new_attachment_data.attachment_size = 1;
		new_attachment_data.save_status = 1;

		attachment_count = 1;

		break;
	default:
		break;
	}

	temp_attachment_data = (email_attachment_data_t *)em_malloc(sizeof(email_attachment_data_t) * attachment_count);
	if (input_attachment_data_list != NULL)
		temp_attachment_data = input_attachment_data_list;

	temp_attachment_data[attachment_count-1] = new_attachment_data;

	input_mail_data->smime_type = smime_type;
	input_mail_data->file_path_mime_entity = EM_SAFE_STRDUP(mime_entity);
	input_mail_data->digest_type = account_tbl_item->digest_type;

	ret = true;

FINISH_OFF:	
	if (output_attachment_count)
		*output_attachment_count = attachment_count;
	
	if (output_attachment_data_list) 
		*output_attachment_data_list = temp_attachment_data;
	

	*output_mail_data = input_mail_data;

	if (!ret && temp_attachment_data)
		emcore_free_attachment_data(&temp_attachment_data, attachment_count, NULL);	
	
#ifdef SMIME_DEBUG
	if (password)
		EM_SAFE_FREE(password);
#endif
	
	return ret;				
}


