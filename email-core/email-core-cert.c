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
#include <glib.h>
#include <cert-svc/ccert.h>
#include <cert-svc/cstring.h>
#include <cert-svc/cpkcs12.h>
#include <cert-svc/cinstance.h>
#include <cert-svc/cprimitives.h>

#include "email-core-cert.h"
#include "email-core-mail.h"
#include "email-core-utils.h"
#include "email-utilities.h"
#include "email-storage.h"
#include "email-debug-log.h"

#define READ_MODE "r"
#define WRITE_MODE "w"

typedef enum {
	CERT_TYPE_ETC          = 0,
	CERT_TYPE_PKCS12,
	CERT_TYPE_PKCS7,
	CERT_TYPE_P7S
} cert_type;

INTERNAL_FUNC int emcore_load_PFX_file(char *certificate, EVP_PKEY **pri_key, X509 **cert, 
									STACK_OF(X509) **ca, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("certificate : [%s]", certificate);
	int err = EMAIL_ERROR_NONE;
	int ret = false;
	size_t key_size = 0;
	char *private_key = NULL;

	/* Variable for certificate */
	X509 *t_cert = NULL;
	BIO *bio_mem = NULL;
//	STACK_OF(X509) *t_ca = NULL;

	/* Variable for private key */
	EVP_PKEY *t_pri_key = NULL;

	CertSvcString csstring;
	CertSvcInstance cert_instance;
	CertSvcCertificate csc_cert;
	CertSvcCertificateList certificate_list;
	CertSvcStoreCertList* certList = NULL;
	CertSvcStoreCertList* tempList = NULL;
	char *alias =  NULL;
	size_t length = 0;

	if (certificate == NULL) {
		EM_DEBUG_EXCEPTION("Invalid parameter");
		err = EMAIL_ERROR_INVALID_PARAM;
		goto FINISH_OFF;
	}

	EM_DEBUG_EXCEPTION("emcore_load_PFX_file - certificate passed : %s", certificate);

	/* Create instance */
	err = certsvc_instance_new(&cert_instance);
	if (err != CERTSVC_SUCCESS) {
		EM_DEBUG_EXCEPTION("certsvc_instance_new failed : [%d]", err);
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	/* Make the pfxID string */
	err = certsvc_string_new(cert_instance, certificate, EM_SAFE_STRLEN(certificate), &csstring);
	if (err != CERTSVC_SUCCESS) {
		EM_DEBUG_EXCEPTION("certsvc_string_new failed : [%d]", err);
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	if (certsvc_pkcs12_get_end_user_certificate_list_from_store(
			cert_instance,
			EMAIL_STORE,
			&certList,
			&length) != CERTSVC_SUCCESS) {
		EM_DEBUG_EXCEPTION("certsvc_string_new failed : [%d]", err);
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	tempList = certList;
	while (tempList) {
		if (strcmp(tempList->title, certificate) == 0
				|| strcmp(tempList->gname, certificate) == 0) {
			alias = strdup(tempList->gname);
			break;
		}

		tempList = tempList->next;
	}

	certsvc_pkcs12_free_certificate_list_loaded_from_store(cert_instance, &certList);
	if (alias == NULL) {
		EM_DEBUG_EXCEPTION("Failed to strdup");
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	/* Make the pfxID string */
	err = certsvc_string_new(cert_instance, alias, EM_SAFE_STRLEN(alias), &csstring);
	if (err != CERTSVC_SUCCESS) {
		EM_DEBUG_EXCEPTION("certsvc_string_new failed : [%d]", err);
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	err = certsvc_pkcs12_load_certificate_list_from_store(cert_instance, EMAIL_STORE, csstring, &certificate_list);
	if (err != CERTSVC_SUCCESS) {
		certsvc_string_free(csstring);
		err = certsvc_string_new(cert_instance, certificate, EM_SAFE_STRLEN(certificate), &csstring);
		if (err != CERTSVC_SUCCESS) {
			EM_DEBUG_EXCEPTION("certsvc_string_new failed : [%d]", err);
			err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
			goto FINISH_OFF;
		}

		/* Load the certificate list of pkcs12 type */
		err = certsvc_pkcs12_load_certificate_list_from_store(cert_instance, EMAIL_STORE, csstring, &certificate_list);
		if (err != CERTSVC_SUCCESS) {
			EM_DEBUG_EXCEPTION("certsvc_pkcs12_load_certificate_list_from_store failed : [%d]", err);
			err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
			goto FINISH_OFF;
		}
	}


	/* Get a certificate */
	err = certsvc_certificate_list_get_one(certificate_list, 0, &csc_cert);
	if (err != CERTSVC_SUCCESS) {
		EM_DEBUG_EXCEPTION("certsvc_certificate_list_get_one failed : [%d]", err);
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	/* Convert from char to X509 */
	err = certsvc_certificate_dup_x509(csc_cert, &t_cert);
	if (t_cert == NULL || err != CERTSVC_SUCCESS) {
		EM_DEBUG_EXCEPTION("certsvc_certificate_dup_x509 failed");
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	/* Get the private key */
	err = certsvc_pkcs12_private_key_dup_from_store(cert_instance, EMAIL_STORE, csstring, &private_key, &key_size);
	if (err != CERTSVC_SUCCESS) {
		EM_DEBUG_EXCEPTION("certsvc_pkcs12_private_key_dup_from_store failed : [%d]", err);
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	EM_DEBUG_LOG_DEV("key_size : [%u], private_key : [%s]", key_size, private_key);

	/* Convert char to pkey */
	bio_mem = BIO_new(BIO_s_mem());
	if (bio_mem == NULL) {
		EM_DEBUG_EXCEPTION("malloc failed");
		err = EMAIL_ERROR_OUT_OF_MEMORY;
		goto FINISH_OFF;
	}

	if (BIO_write(bio_mem, private_key, (int)key_size) <= 0) {
		EM_DEBUG_EXCEPTION("BIO_write failed");
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	t_pri_key = PEM_read_bio_PrivateKey(bio_mem, NULL, 0, NULL);
	if (t_pri_key == NULL) {
		EM_DEBUG_EXCEPTION("PEM_read_bio_PrivateKey failed");
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	ret = true;

FINISH_OFF:

	if (bio_mem)
		BIO_free(bio_mem);

	if (true) {
		if (cert)
			*cert = t_cert;

		if (pri_key)
			*pri_key = t_pri_key;
	} else {
		X509_free(t_cert);
		EVP_PKEY_free(t_pri_key);
	}

	if (private_key)
		EM_SAFE_FREE(private_key);

	if (err_code)
		*err_code = err;

	if (alias)
		free(alias);

	certsvc_instance_free(cert_instance);

	return ret;
}

INTERNAL_FUNC int emcore_verify_signature(char *p7s_file_path, char *mime_entity, int *validity, int *err_code)
{
	EM_DEBUG_FUNC_BEGIN_SEC("path : [%s], mime_entity : [%s]", p7s_file_path, mime_entity);
	int ret = false;
	int err = EMAIL_ERROR_NONE;
	int t_validity = 0;
	int flags = PKCS7_NOVERIFY;

	BIO *bio_p7s = NULL;

	BIO *bio_indata = NULL;

	PKCS7 *pkcs7_p7s = NULL;

	/* Open p7s file */
	bio_p7s = BIO_new_file(p7s_file_path, INMODE);
	if (!bio_p7s) {
		EM_DEBUG_EXCEPTION("File open failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Open input data file */
	bio_indata = BIO_new_file(mime_entity, INMODE);
	if (!bio_p7s) {
		EM_DEBUG_EXCEPTION("File open failed");
		err = EMAIL_ERROR_SYSTEM_FAILURE;
		goto FINISH_OFF;
	}

	/* Load the p7s file */
	pkcs7_p7s = d2i_PKCS7_bio(bio_p7s, NULL);
	if (!pkcs7_p7s) {
		EM_DEBUG_EXCEPTION("d2i_PKCS7_bio failed");
		err = EMAIL_ERROR_LOAD_CERTIFICATE_FAILURE;
		goto FINISH_OFF;
	}

	if (!PKCS7_verify(pkcs7_p7s, NULL, NULL, bio_indata, NULL, flags)) {
		EM_DEBUG_EXCEPTION("PKCS7_verify failed");
		goto FINISH_OFF;
	}

	t_validity = 1;

	ret = true;

FINISH_OFF:

	if (pkcs7_p7s)
		PKCS7_free(pkcs7_p7s);

	if (bio_p7s)
		BIO_free(bio_p7s);

	if (bio_indata)
		BIO_free(bio_indata);

	if (validity)
		*validity = t_validity;

	if (err_code)
		*err_code = err;

	EM_DEBUG_FUNC_END();
	return ret;
}
