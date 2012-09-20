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
 * File :  email-core-cert.h
 * Desc :  Certificate API
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#include <openssl/pkcs7.h>
#include <openssl/pkcs12.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <cert-service.h>
#include <glib.h>

#include "email-core-cert.h"
#include "email-core-mail.h"
#include "email-utilities.h"
#include "email-storage.h"
#include "email-debug-log.h"

#define READ_MODE "r"
#define WRITE_MODE "w"

#define EMAIL_ERROR_CERTIFICATE_LOAD_FAIL 2300
#define EMAIL_ERROR_FILE_NOT_OPEN 2301
#define EMAIL_ERROR_INVALID_CERTIFICATE 2302

#define TRUSTED_USER "trusteduser/email/"

typedef enum {
	CERT_TYPE_ETC          = 0,
	CERT_TYPE_PKCS12,	
	CERT_TYPE_PKCS7,
	CERT_TYPE_P7S
} cert_type;

static int emcore_get_certificate_type(char *extension, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("extensiong is [%s]", extension);
	int index = 0;
	int type = 0;
	int err = EMAIL_ERROR_NONE;
	char *supported_file_type[] = {"pfx", "p12", "p7s", "pem", "der", "crt", "cer", NULL};

	if (!extension) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}
	
	while(supported_file_type[index]) {
		EM_DEBUG_LOG("certificate extension[%d]:[%s]", index, supported_file_type[index]);
		if (strcasecmp(extension, supported_file_type[index]) == 0) {
			switch (index) {
			case 0:
			case 1:
				type = CERT_TYPE_PKCS12;
				err = EMAIL_ERROR_INVALID_CERTIFICATE;
				break;
			case 2:
				type = CERT_TYPE_P7S;
				break;
			case 3:
			case 4:
			case 5:
			case 6:
				type = CERT_TYPE_PKCS7;
				break;
			default:
				type = CERT_TYPE_ETC;
				err = EMAIL_ERROR_INVALID_CERTIFICATE;
				break;			
			}
		}
		index++;
	}

FINISH_OFF:

	if (err_code) {
		*err_code = err;
	}

	EM_DEBUG_FUNC_END("File type is [%d]", type);
	return type;
}
/*	
static GList *emcore_make_glist_from_string(char *email_address_list)
{
	EM_DEBUG_FUNC_BEGIN("email_address list : [%s]", email_address_list);
	int index = 0;
	const gchar seperator = 0x01;
	GList *email_list = NULL;
	gchar *p_email_address_list = NULL;
	gchar **token_list = NULL;

	p_email_address_list = g_strdup(email_address_list);

	token_list = g_strsplit(p_email_address_list, &seperator, -1);
	while (token_list[index] != NULL) {
		email_list = g_list_append(email_list, token_list[index]);
		index++;
	}
	
	if (p_email_address_list)
		g_free(p_email_address_list);

	return email_list;	
}
*/
static int emcore_load_public_of_p7s(CERT_CONTEXT *context, char *public_cert_path, STACK_OF(X509) **certificates, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int cert_size = 0;
	unsigned char *cert_string = NULL;
	PKCS7 *pkcs7 = NULL;
	BIO *infile = NULL;
	X509 *cert = NULL;
	STACK_OF(X509) *certs = NULL;
	
	if (public_cert_path == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	OpenSSL_add_all_algorithms();
	
	/* Open p7s file */
	infile = BIO_new_file(public_cert_path, READ_MODE);
	if (infile == NULL) {
		EM_DEBUG_EXCEPTION("File open failed [%s]", public_cert_path);
		err = EMAIL_ERROR_FILE_NOT_FOUND;
		goto FINISH_OFF;
	}

	/* Load the p7s file */
	pkcs7 = d2i_PKCS7_bio(infile, NULL);
	if (pkcs7 == NULL) {
		EM_DEBUG_EXCEPTION("smime.p7s load failed");
		err = EMAIL_ERROR_CERTIFICATE_LOAD_FAIL;
		goto FINISH_OFF;
	}

	/* Extract the signature in pkcs7 */
	certs = PKCS7_get0_signers(pkcs7, NULL, 0);
	if (certs == NULL) {
		EM_DEBUG_EXCEPTION("smime.p7s extract failed");
		err = EMAIL_ERROR_CERTIFICATE_LOAD_FAIL;
		goto FINISH_OFF;
	}
	
	/* Change from the p7s to the pem file */
	if (sk_X509_num(certs) > 1) {
		EM_DEBUG_EXCEPTION("Has one or more signature.\nIt is not support");
		err = EMAIL_ERROR_UNKNOWN;
		goto FINISH_OFF;
	}

	cert = sk_X509_value(certs, 0);
	cert_size = i2d_X509(cert, &cert_string);
	if (cert_size < 0) {
		EM_DEBUG_EXCEPTION("Fail to convert certificate");
		err = EMAIL_ERROR_UNKNOWN;
		goto FINISH_OFF;
	}

	/* Initilize the certificate and CERT_CONTEXT buffer */
	cert_string = (unsigned char *)em_malloc(sizeof(unsigned char *) * cert_size);
	if (cert_string == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	context->certBuf = (cert_svc_mem_buff *)em_malloc(sizeof(cert_svc_mem_buff));
	if (context->certBuf == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	/* Set CERT_CONTEXT structure */
	context->certBuf->data = cert_string;
	context->certBuf->size = cert_size;
	
	ret = true;

FINISH_OFF:

	if (infile) 
		BIO_free(infile);
	
	if (pkcs7)
		PKCS7_free(pkcs7);
	
	if (cert)
		X509_free(cert);

	if (ret) {
		if (certificates != NULL)
			*certificates = certs;

		if (err_code != NULL)
			*err_code = err;
	} else {
		if (certs)
			sk_X509_free(certs);

		EM_SAFE_FREE(cert_string);
	}

	EVP_cleanup();

	EM_DEBUG_FUNC_END();

	return ret;
}
/*
static char *emcore_store_public_certificate(STACK_OF(X509) *certificates, char *email_address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int index = 0;
	int err = EMAIL_ERROR_NONE;
	char *file_path = NULL;
	BIO *outfile = NULL;

	file_path = (char *)em_malloc(256);
	if (file_path == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	SNPRINTF(file_path, 256, "%s%s%s", CERT_SVC_STORE_PATH, TRUSTED_USER, email_address);
	outfile = BIO_new_file(file_path, WRITE_MODE);
	if (outfile == NULL) {
		EM_DEBUG_EXCEPTION("File open failed[write mode]");
		err = EMAIL_ERROR_FILE_NOT_OPEN;
		goto FINISH_OFF;
	}

	for (index = 0; index < sk_X509_num(certificates); index++) {
		EM_DEBUG_LOG("Write the certificate in pem file : [%d]", index);
		PEM_write_bio_X509(outfile, sk_X509_value(certificates, index));
	}
	
FINISH_OFF:

	if (outfile)
		BIO_free(outfile);

	EM_DEBUG_FUNC_END();

	return file_path;
}
*/
INTERNAL_FUNC int emcore_load_PFX_file(char *certificate, char *password, EVP_PKEY **pri_key, X509 **cert, STACK_OF(X509) **ca, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("Certificate path : [%s], password : [%s]", certificate, password);

	int err = EMAIL_ERROR_NONE;
	int ret = false;
	FILE *fp = NULL;
	PKCS12 *p12 = NULL;

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();
	if (!(fp = fopen(certificate, "rb"))) {
		EM_DEBUG_EXCEPTION("fopen failed : [%s]", certificate);
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}
	
	p12 = d2i_PKCS12_fp(fp, NULL);
	if (!p12) {
		EM_DEBUG_EXCEPTION("d2i_PKCS12_fp failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	if (!PKCS12_parse(p12, password, pri_key, cert, ca)) {
		EM_DEBUG_EXCEPTION("PKCS12_parse failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (fp)
		fclose(fp);

	if (p12)
		PKCS12_free(p12);

	if (err_code)
		*err_code = err;

	return ret;	
}

INTERNAL_FUNC int emcore_add_public_certificate(char *public_cert_path, char *save_name, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("Path [%s], filename [%s]", public_cert_path, save_name);
	int err = EMAIL_ERROR_NONE;	
	int ret = false;
	int validity = 0;
	int cert_type = 0;
	char temp_file[512] = {0, };
	char temp_save_name[512] = {0, };
	char filepath[512] = {0, };
	char *extension = NULL;
	emstorage_certificate_tbl_t *cert = NULL;
	CERT_CONTEXT *context = NULL;

	if (public_cert_path == NULL || save_name == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;		
	}
	
	/* Initilize the structure of certificate */
	context = cert_svc_cert_context_init();

	/* Parse the file type */
	extension = em_get_extension_from_file_path(public_cert_path, NULL);
	if (extension == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	/* Get the file type information */
	cert_type = emcore_get_certificate_type(extension, &err);
	if (!cert_type || err == EMAIL_ERROR_INVALID_CERTIFICATE) {
		EM_DEBUG_EXCEPTION("Invalid certificate");
		goto FINISH_OFF;
	}

	/* Create temp file and rename */
	if (cert_type == CERT_TYPE_P7S) {
		extension = "der";
	}
	
	SNPRINTF(temp_file, sizeof(temp_file), "%s%s%s%s%s.%s", MAILHOME, DIR_SEPERATOR, MAILTEMP, DIR_SEPERATOR, save_name, extension);
	EM_DEBUG_LOG("temp cert path : [%s]", temp_file);

	if (!emstorage_copy_file(public_cert_path, temp_file, false, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_copy_file failed [%d]", err);
		goto FINISH_OFF;
	}
	
	/* Load the public certificate */
	err = cert_svc_load_file_to_context(context, temp_file);
	if (err != CERT_SVC_ERR_NO_ERROR) {
		EM_DEBUG_EXCEPTION("Load cert failed : [%d]", err);
		err = EMAIL_ERROR_INVALID_CERTIFICATE;
		goto FINISH_OFF;
	}

	/* Verify the certificate */
	if (cert_svc_verify_certificate(context, &validity) != CERT_SVC_ERR_NO_ERROR) {
		EM_DEBUG_EXCEPTION("cert_svc_verify_certificate failed");
//		err = EMAIL_ERROR_INVALID_CERTIFICATE;
//		goto FINISH_OFF;
	} 

	if (validity <= 0) {
		EM_DEBUG_LOG("Invalid certificate");
	}

	/* Load the certificate information */
	if (cert_svc_extract_certificate_data(context) != CERT_SVC_ERR_NO_ERROR) {
		EM_DEBUG_EXCEPTION("Extract the certificate failed");
		err = EMAIL_ERROR_UNKNOWN;
		goto FINISH_OFF;
	}

	/* Store the certificate file to trusted folder */
	if (cert_svc_add_certificate_to_store(temp_file, TRUSTED_USER) != CERT_SVC_ERR_NO_ERROR) {
		EM_DEBUG_EXCEPTION("Add certificate to trusted folder");
		err = EMAIL_ERROR_UNKNOWN;
		goto FINISH_OFF;
	}

	/* Store the certificate to DB */
	SNPRINTF(filepath, sizeof(filepath), "%s%s%s.%s", CERT_SVC_STORE_PATH, TRUSTED_USER, save_name, extension);
	SNPRINTF(temp_save_name, sizeof(temp_save_name), "<%s>", save_name);

	cert = (emstorage_certificate_tbl_t *)em_malloc(sizeof(emstorage_certificate_tbl_t));
	if (cert == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	cert->issue_year = context->certDesc->info.validPeriod.firstYear;
	cert->issue_year = context->certDesc->info.validPeriod.firstYear;
	cert->issue_month = context->certDesc->info.validPeriod.firstMonth;
	cert->issue_day = context->certDesc->info.validPeriod.firstDay;		
	cert->expiration_year= context->certDesc->info.validPeriod.secondYear;		
	cert->expiration_month = context->certDesc->info.validPeriod.secondMonth;		
	cert->expiration_day = context->certDesc->info.validPeriod.secondDay;				
	cert->issue_organization_name = EM_SAFE_STRDUP(context->certDesc->info.issuer.organizationName);
	cert->email_address = EM_SAFE_STRDUP(temp_save_name);
	cert->subject_str = EM_SAFE_STRDUP(context->certDesc->info.issuerStr);		
	cert->filepath = EM_SAFE_STRDUP(filepath);				

	if (emstorage_add_certificate(cert, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_add_certificate failed");
		goto FINISH_OFF;
	}

	if (!emstorage_delete_file(public_cert_path, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_file failed [%d]", err);
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:
	
	emstorage_free_certificate(&cert, 1, NULL);

	cert_svc_cert_context_final(context);

	if (err_code != NULL) {
		*err_code = err;
	}

	EM_DEBUG_FUNC_END();

	return ret;

}

INTERNAL_FUNC int emcore_delete_public_certificate(char *email_address, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	emstorage_certificate_tbl_t *certificate = NULL;

	if (email_address == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	if (!emstorage_get_certificate_by_email_address(email_address, &certificate, false, 0, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_get_certificate failed");
		goto FINISH_OFF;
	}

	if (remove(certificate->filepath) < 0) {
		EM_DEBUG_EXCEPTION("remove failed : [%s]", certificate->filepath);
		goto FINISH_OFF;
	}

	if (!emstorage_delete_certificate(certificate->certificate_id, true, &err)) {
		EM_DEBUG_EXCEPTION("emstorage_delete_certificate failed");
		goto FINISH_OFF;
	}

	ret = true;
FINISH_OFF:

	if (certificate != NULL)
		emstorage_free_certificate(&certificate, 1, NULL);

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();

	return ret;
}

INTERNAL_FUNC int emcore_verify_signature(char *p7s_file_path, char *mime_entity, int *validity, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("path[%s], mime_entiy[%s]", p7s_file_path, mime_entity);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int i = 0, j = 0;

	int p_validity = 0;
	int msg_len = 0;
	unsigned char *msg = NULL;
	int sig_len = 0;
	unsigned char *sig = NULL;
	unsigned char *tmp_sig = NULL;
	FILE *fp_msg = NULL;
	FILE *fp_sig = NULL;
	size_t f_size = 0;

	CERT_CONTEXT *context = NULL;

	context = cert_svc_cert_context_init();

	if (!emcore_load_public_of_p7s(context, p7s_file_path, NULL, &err)) {
		EM_DEBUG_EXCEPTION("emcore_load_public_of_p7s failed");
		goto FINISH_OFF;
	}

	fp_msg = fopen(mime_entity, "rb");
	fseek(fp_msg, 0L, SEEK_END);
	msg_len = ftell(fp_msg);
	fseek(fp_msg, 0L, SEEK_SET);
	msg = (unsigned char *)em_malloc(sizeof(unsigned char) * (msg_len + 1));
	if (msg == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if ((f_size = fread(msg, sizeof(unsigned char), msg_len, fp_msg)) < 0) {
		EM_DEBUG_EXCEPTION("File read failed");
		goto FINISH_OFF;		
	}

	fp_sig = fopen(p7s_file_path, "rb");
	fseek(fp_sig, 0L, SEEK_END);
	sig_len = ftell(fp_sig);
	fseek(fp_sig, 0L, SEEK_SET);
	sig = (unsigned char *)em_malloc(sizeof(unsigned char) * (sig_len + 1));
	if (sig == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}
	tmp_sig = (unsigned char *)em_malloc(sizeof(unsigned char) * (sig_len + 1));
	if (tmp_sig == NULL) {
		EM_DEBUG_EXCEPTION("em_malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if ((f_size = fread(sig, sizeof(unsigned char), sig_len, fp_sig)) < 0) {
		EM_DEBUG_EXCEPTION("File read failed");
		goto FINISH_OFF;
	}

	for(i = 0; i < sig_len; i++) {
		if(sig[i] != '\n') {
			tmp_sig[j] = sig[i];
			j++;
		}
	}
	
	err = cert_svc_verify_signature(context, msg, msg_len, tmp_sig, NULL, &p_validity);
	if (err != CERT_SVC_ERR_NO_ERROR) {
		EM_DEBUG_EXCEPTION("Verify signature failed");
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	EM_SAFE_FREE(tmp_sig);

	EM_SAFE_FREE(sig);

	EM_SAFE_FREE(msg);

	if (fp_sig != NULL)
		fclose(fp_sig);
	
	if (fp_msg != NULL)
		fclose(fp_msg);

	cert_svc_cert_context_final(context);

	

	if (err_code != NULL)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_verify_certificate(char *certificate, int *validity, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN();
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int p_validity = 0;

	CERT_CONTEXT *context = NULL;
	
	context = cert_svc_cert_context_init();

	err = cert_svc_load_file_to_context(context, certificate);
	if (err != CERT_SVC_ERR_NO_ERROR) {
		EM_DEBUG_EXCEPTION("Certificate load failed");
		goto FINISH_OFF;
	}

	err = cert_svc_verify_certificate(context, &p_validity);
	if (err != CERT_SVC_ERR_NO_ERROR) {
		EM_DEBUG_EXCEPTION("Certificate verify failed");
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (validity != NULL) 
		*validity = p_validity;

	if (err_code != NULL) {
		*err_code = err;
	}

	EM_DEBUG_FUNC_END();
	return ret;
}

INTERNAL_FUNC int emcore_free_certificate(email_certificate_t **certificate, int count, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN("certificate [%p], count [%d]", certificate, count);
	
	if (count <= 0 || !certificate || !*certificate) {
		EM_DEBUG_EXCEPTION("EMAIL_ERROR_INVALID_PARAM");
		if (err_code)	
			*err_code = EMAIL_ERROR_INVALID_PARAM;
		return false;
	}

	email_certificate_t *p_certificate = *certificate;
	int i;
	
	for (i=0;i<count;i++) {
		EM_SAFE_FREE(p_certificate[i].issue_organization_name);
		EM_SAFE_FREE(p_certificate[i].email_address);
		EM_SAFE_FREE(p_certificate[i].subject_str);
		EM_SAFE_FREE(p_certificate[i].filepath);
	}

	EM_SAFE_FREE(p_certificate);
	*certificate = NULL;

	if (err_code)
		*err_code = EMAIL_ERROR_NONE;

	EM_DEBUG_FUNC_END();
	return true;
}
