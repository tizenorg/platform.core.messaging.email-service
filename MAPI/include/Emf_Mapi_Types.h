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


#ifndef __EMF_MAPI_TYPES_H__
#define __EMF_MAPI_TYPES_H__

#include "emf-types.h"

/**
* @defgroup EMAIL_FRAMEWORK Email Service
* @{
*/

/**
* @ingroup EMAIL_FRAMEWORK
* @defgroup EMAIL_MAPI_TYPES Email Types API
* @{
*/


/**
 *
 * This file contains the data structures and interfaces needed for application,
 * to interact with Email Engine.
 * @file		Emf_Mapi_Types.h
 * @author	Kyuho Jo <kyuho.jo@samsung.com>
 * @author	Sunghyun Kwon <sh0701.kwon@samsung.com>
 * @version	0.1
 * @brief 	This file contains interfaces for using data structures of email-service . 
 */
 


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


// emf_mail_t ------------------------------------------------------------------
/**
 * @open
 * @fn  email_get_info_from_mail(emf_mail_t *mail, emf_mail_info_t **info)
 * @brief	Gets the object of emf_mail_info_t. If mail is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for info object, but just returns the pointer to the emf_mail_info_t object in mail object.
 *			The info objet will be freed together with mail , when mail is freed by calling email_free_mail()
 *			So don't free info object itself if it is the object in mail object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mail	Specifies the pointer to emf_mail_t. This contains emf_mail_info_t object.
 * @param[out] info	Specifies the pointer to emf_mail_info_t. The info will point the info object in emf_mail_t(mail)
 * @exception 		none
 * @see                 emf_mail_t, emf_mail_info_t	
 */
EXPORT_API int email_get_info_from_mail(emf_mail_t *mail, emf_mail_info_t **info) DEPRECATED;

/**
 * @open
 * @fn  email_set_info_to_mail(emf_mail_t *mail, emf_mail_info_t *info)
 * @brief	Sets the object of emf_mail_info_t to emf_mail_obejct(mail). If mail is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mail	Specifies the pointer to emf_mail_t.
 * @param[in] info	Specifies the pointer to emf_mail_info_t.
 * @exception 		none
 * @see                 emf_mail_t, emf_mail_info_t	
 */
EXPORT_API int email_set_info_to_mail(emf_mail_t *mail, emf_mail_info_t *info) DEPRECATED;

/**
 * @open
 * @fn  email_get_head_from_mail(emf_mail_t *mail, emf_mail_head_t **head)
 * @brief	Gets the object of emf_mail_head_t. If mail is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for head object, but just returns the pointer to the emf_mail_head_t object in mail object.
 *			The head objet will be freed together with mail , when mail is freed by calling email_free_mail()
 *			So don't free head object if it is the object in mail object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mail	Specifies the pointer to emf_mail_t. This contains emf_mail_head_t object.
 * @param[out] head	Specifies the pointer to emf_mail_head_t. This  will point the head object in emf_mail_t(mail).
 * @exception 		none
 * @see                 emf_mail_t, emf_mail_head_t	
 */
EXPORT_API int email_get_head_from_mail(emf_mail_t *mail, emf_mail_head_t **head) DEPRECATED;

/**
 * @open
 * @fn  email_set_head_to_mail(emf_mail_t *mail, emf_mail_head_t *head)
 * @brief	Sets the object of emf_mail_head_t to emf_mail_obejct(mail). If mail is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mail	Specifies the pointer to emf_mail_t.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @exception 		none
 * @see                 emf_mail_t, emf_mail_head_t	
 */
EXPORT_API int email_set_head_to_mail(emf_mail_t *mail, emf_mail_head_t *head) DEPRECATED;

/**
 * @open
 * @fn  email_get_body_from_mail(emf_mail_t *mail, emf_mail_body_t **body)
 * @brief	Gets the object of emf_mail_body_t. If mail is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for emf_mail_body_t object, but just returns the pointer to the emf_mail_body_t object in mail object.
 *			The body objet will be freed together with mail , when mail is freed by calling email_free_mail()
 *			So don't free body object if it is the object in mail object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mail	Specifies the pointer to emf_mail_t. This contains emf_mail_body_t object.
 * @param[out] body	Specifies the pointer to emf_mail_body_t. This will point the object in emf_mail_t(mail)
 * @exception 		none
 * @see                 emf_mail_t, emf_mail_body_t	
 */
EXPORT_API int email_get_body_from_mail(emf_mail_t *mail, emf_mail_body_t **body) DEPRECATED;

/**
 * @open
 * @fn  email_set_body_to_mail(emf_mail_t *mail, emf_mail_body_t *body)
 * @brief	Sets the object of emf_mail_body_t to emf_mail_obejct(mail). If mail is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] mail	Specifies the pointer to emf_mail_t.
 * @param[in] body	Specifies the pointer to emf_mail_body_t.
 * @exception 		none
 * @see                 emf_mail_t, emf_mail_head_t	
 */
EXPORT_API int email_set_body_to_mail(emf_mail_t *mail, emf_mail_body_t *body) DEPRECATED;

// emf_mail_info_t -------------------------------------------------------------
/**
 * @open
 * @fn  email_get_account_id_from_info(emf_mail_info_t *info, int *account_id)
 * @brief	Gets account id from the info object . If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for account id. but just returns account id in info object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t. 
 * @param[out] account_id	Specifies the pointer to the memory to save account id
 * @exception 		none
 * @see                 emf_mail_info_t	
 */
EXPORT_API int email_get_account_id_from_info(emf_mail_info_t *info, int *account_id) DEPRECATED;

/**
 * @open
 * @fn  email_set_account_id_to_info(emf_mail_info_t *info, int account_id)
 * @brief	Sets account id to emf_mail_info_t(info). If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t.
 * @param[in] account_id	Specifies the account id.
 * @exception 		none
 * @see                 emf_mail_info_t	
 */
EXPORT_API int email_set_account_id_to_info(emf_mail_info_t *info, int account_id) DEPRECATED;

/**
 * @open
 * @fn  email_get_uid_from_info(emf_mail_info_t *info, int *uid)
 * @brief	Gets uid from the info object . If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for uid, but just returns uid in info object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t. 
 * @param[out] uid	Specifies the pointer to the memory to save uid
 * @exception 		none
 * @see                 emf_mail_info_t	
 */
EXPORT_API int email_get_uid_from_info(emf_mail_info_t *info, int *uid) DEPRECATED;

/**
 * @open
 * @fn  email_set_uid_to_info(emf_mail_info_t *info, int uid)
 * @brief	Sets uid to emf_mail_info_t(info). If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t.
 * @param[in] uid	Specifies the uid.
 * @exception 		none
 * @see                 emf_mail_info_t	
 */
EXPORT_API int email_set_uid_to_info(emf_mail_info_t *info, int uid) DEPRECATED;

/**
 * @open
 * @fn  email_get_rfc822_size_from_info(emf_mail_info_t *info, int *rfc822_size)
 * @brief	Gets rfc822 size from the info object . If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for rfc822 size, but just returns rfc822 size in info object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t. 
 * @param[out] rfc822_size	Specifies the pointer to the memory to save rfc822 size
 * @exception 		none
 * @see                 emf_mail_info_t	
 */
EXPORT_API int email_get_rfc822_size_from_info(emf_mail_info_t *info, int *rfc822_size) DEPRECATED;

/**
 * @open
 * @fn  email_set_rfc822_size_to_info(emf_mail_info_t *info, int rfc822_size)
 * @brief	Sets rfc822 size to emf_mail_info_t(info). If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t.
 * @param[in] rfc822_size	Specifies a rfc822 size.
 * @exception 		none
 * @see                 emf_mail_info_t	
 */
EXPORT_API int email_set_rfc822_size_to_info(emf_mail_info_t *info, int rfc822_size) DEPRECATED;

/**
 * @open
 * @fn  email_get_body_downloaded_from_info(emf_mail_info_t *info, int *body_downloaded)
 * @brief	Gets whether body is downloaded  from the info object . If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for body_downloaded, but just returns whether body is downloaded in info object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t. 
 * @param[out] body_downloaded	Specifies the pointer to the memory to save whether body is downlaoded.
 * @exception 		none
 * @see                 emf_mail_info_t	
 */
EXPORT_API int email_get_body_downloaded_from_info(emf_mail_info_t *info, int *body_downloaded) DEPRECATED;

/**
 * @open
 * @fn  email_set_body_downloaded_to_info(emf_mail_info_t *info, int body_downloaded)
 * @brief	Sets the field which shows whether body is downloaded to emf_mail_info_t(info). If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t.
 * @param[in] body_downloaded	Specifies whether body is downloaded (0:no, 1:yes)
 * @exception 		none
 * @see                 emf_mail_info_t	
 */
EXPORT_API int email_set_body_downloaded_to_info(emf_mail_info_t *info, int body_downloaded) DEPRECATED;

/**
 * @open
 * @fn  email_get_flags_from_info(emf_mail_info_t *info, emf_mail_flag_t flags)
 * @brief	Gets flags from the info object . If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for flags, but just returns flags in info object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t. 
 * @param[out] flags	Specifies the pointer to the memory to save flags
 * @exception 		none
 * @see                 emf_mail_info_t, emf_mail_flag_t
 */
EXPORT_API int email_get_flags_from_info(emf_mail_info_t *info, emf_mail_flag_t *flags) DEPRECATED;


/**
 * @open
 * @fn  email_set_flags_to_info(emf_mail_info_t *info, emf_mail_flag_t flags)
 * @brief	Sets flags to emf_mail_info_t(info). If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t.
 * @param[in] flags	Specifies the flags.
 * @exception 		none
 * @see                 emf_mail_info_t, emf_mail_flag_t	
 */
EXPORT_API int email_set_flags_to_info(emf_mail_info_t *info, emf_mail_flag_t flags) DEPRECATED;

/**
 * @open
 * @fn  email_get_extra_flags_from_info(emf_mail_info_t *info, emf_extra_flag_t extra_flags)
 * @brief	Gets extra flags from the info object . If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for extra  flags, but just returns extra flags in info object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t. 
 * @param[out] extra_flags	Specifies the pointer to the memory to save extra_flags
 * @exception 		none
 * @see                 emf_mail_info_t, emf_extra_flag_t
 */
EXPORT_API int email_get_extra_flags_from_info(emf_mail_info_t *info, emf_extra_flag_t extra_flags) DEPRECATED;

/**
 * @open
 * @fn  email_set_extra_flags_to_info(emf_mail_info_t *info, emf_extra_flag_t extra_flags)
 * @brief	Sets extra flags to emf_mail_info_t(info). If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t.
 * @param[in] extra_flags	Specifies the extra flags.
 * @exception 		none
 * @see                 emf_mail_info_t, emf_extra_flag_t	
 */
EXPORT_API int email_set_extra_flags_to_info(emf_mail_info_t *info, emf_extra_flag_t extra_flags) DEPRECATED;

/**
 * @open
 * @fn  email_get_sid_from_info(emf_mail_info_t *info, char **sid)
 * @brief	Gets sid(the email's UID on server) from the info object . If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for sid, but just returns sid in info object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t. 
 * @param[out] sid	Specifies the pointer to the memory to save sid
 * @exception 		none
 * @see                 emf_mail_info_t
 */
EXPORT_API int email_get_sid_from_info(emf_mail_info_t *info, char **sid) DEPRECATED;

/**
 * @open
 * @fn  email_set_extra_flags_to_info(emf_mail_info_t *info, emf_extra_flag_t extra_flags)
 * @brief	Sets sid(the email's UID on server)  to emf_mail_info_t(info). If info is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] info	Specifies the pointer to emf_mail_info_t.
 * @param[in] sid	 	Specifies the extra flags.
 * @exception 		none
 * @see                 emf_mail_info_t
 */
EXPORT_API int email_set_sid_to_info(emf_mail_info_t *info, char *sid) DEPRECATED;


// emf_mail_head_t -------------------------------------------------------------
/**
 * @open
 * @fn  email_get_mid_from_head(emf_mail_head_t *head, char **mid)
 * @brief	Gets mid(the message ID) from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for mid but just returns mid in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] mid	Specifies the pointer to the memory to save mid
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_mid_from_head(emf_mail_head_t *head, char **mid) DEPRECATED;

/**
 * @open
 * @fn  email_set_mid_to_head(emf_mail_head_t *head, char *mid)
 * @brief	Sets  mid(the message ID)  to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] mid	 	Specifies the mid(the message ID).
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_mid_to_head(emf_mail_head_t *head, char *mid) DEPRECATED;

/**
 * @open
 * @fn  email_get_subject_from_head(emf_mail_head_t *head, char **subject)
 * @brief	Gets subject of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for subject but just returns subject in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] subject	Specifies the pointer to the memory to save subject
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_subject_from_head(emf_mail_head_t *head, char **subject) DEPRECATED;

/**
 * @open
 * @fn  email_set_subject_to_head(emf_mail_head_t *head, char *subject)
 * @brief	Sets  subject of a mail  to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] subject	 	Specifies the subject.
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_subject_to_head(emf_mail_head_t *head, char *subject) DEPRECATED;

/**
 * @open
 * @fn  email_get_to_from_head(emf_mail_head_t *head, char **to)
 * @brief	Gets to field of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for to field but just returns to field in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] to	Specifies the pointer to the memory to save to field
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_to_from_head(emf_mail_head_t *head, char **to) DEPRECATED;

/**
 * @open
 * @fn  email_set_to_to_head(emf_mail_head_t *head, char *to)
 * @brief	Sets  to field of a mail  to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] to	 	Specifies the to field of a mail.
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_to_to_head(emf_mail_head_t *head, char *to) DEPRECATED;

/**
 * @open
 * @fn  email_get_from_from_head(emf_mail_head_t *head, char **from)
 * @brief	Gets from field of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for from field but just returns from field in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] from	Specifies the pointer to the memory to save from field
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_from_from_head(emf_mail_head_t *head, char **from) DEPRECATED;

/**
 * @open
 * @fn  email_set_from_to_head(emf_mail_head_t *head, char *from)
 * @brief	Sets  from field of a mail  to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] from	 	Specifies the from field of a mail.
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_from_to_head(emf_mail_head_t *head, char *from) DEPRECATED;

/**
 * @open
 * @fn  email_get_cc_from_head(emf_mail_head_t *head, char **cc)
 * @brief	Gets cc field of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for cc field but just returns cc field in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] cc	Specifies the pointer to the memory to save cc field
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_cc_from_head(emf_mail_head_t *head, char **cc) DEPRECATED;

/**
 * @open
 * @fn  email_set_cc_to_head(emf_mail_head_t *head, char *cc)
 * @brief	Sets  cc field of a mail  to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] cc	 	Specifies the cc field of a mail.
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_cc_to_head(emf_mail_head_t *head, char *cc) DEPRECATED;

/**
 * @open
 * @fn  email_get_bcc_from_head(emf_mail_head_t *head, char **bcc)
 * @brief	Gets bcc field of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for bcc field but just returns bcc field in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] bcc	Specifies the pointer to the memory to save bcc field
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_bcc_from_head(emf_mail_head_t *head, char **bcc) DEPRECATED;

/**
 * @open
 * @fn  email_set_bcc_to_head(emf_mail_head_t *head, char *bcc)
 * @brief	Sets  bcc field of a mail  to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] bcc	 	Specifies the bcc field of a mail.
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_bcc_to_head(emf_mail_head_t *head, char *bcc) DEPRECATED;

/**
 * @open
 * @fn  email_get_reply_from_head(emf_mail_head_t *head, char **reply)
 * @brief	Gets a reply address of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for a reply address but just returns a reply address in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] reply	Specifies the pointer to the memory to save a reply address
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_reply_from_head(emf_mail_head_t *head, char **reply) DEPRECATED;

/**
 * @open
 * @fn  email_set_reply_to_head(emf_mail_head_t *head, char *reply)
 * @brief	Sets  a reply address of a mail  to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] reply	 	Specifies the reply address  of a mail.
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_reply_to_head(emf_mail_head_t *head, char *reply) DEPRECATED;

/**
 * @open
 * @fn  email_get_return_path_from_head(emf_mail_head_t *head, char **return_path)
 * @brief	Gets a return path of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for a return path but just returns a return path in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] return_path	Specifies the pointer to the memory to save a return path
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_return_path_from_head(emf_mail_head_t *head, char **return_path) DEPRECATED;

/**
 * @open
 * @fn  email_set_return_path_to_head(emf_mail_head_t *head, char *return_path)
 * @brief	Sets  a return path of a mail  to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] return_path	 	Specifies the return path  of a mail.
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_return_path_to_head(emf_mail_head_t *head, char *return_path) DEPRECATED;

/**
 * @open
 * @fn  email_get_datetime_from_head(emf_mail_head_t *head, emf_datetime_t *datetime)
 * @brief	Gets datetime of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for datetime but just returns datetime in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] datetime	Specifies the pointer to the memory to save datetime
 * @exception 		none
 * @see                 emf_mail_head_t, emf_datetime_t
 */
EXPORT_API int email_get_datetime_from_head(emf_mail_head_t *head, emf_datetime_t *datetime) DEPRECATED;

/**
 * @open
 * @fn  email_set_datetime_to_head(emf_mail_head_t *head, emf_datetime_t datetime)
 * @brief	Sets  datetime to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] datetime	 	Specifies the datetime  of a mail.
 * @exception 		none
 * @see                 emf_mail_head_t, emf_datetime_t
 */
EXPORT_API int email_set_datetime_to_head(emf_mail_head_t *head, emf_datetime_t datetime) DEPRECATED;

/**
 * @open
 * @fn  email_get_from_contact_name_from_head(emf_mail_head_t *head, char **from_contact_name)
 * @brief	Gets contact names of from address of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for contact names but just returns contact names in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] from_contact_name	Specifies the pointer to the memory to save contact names 
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_from_contact_name_from_head(emf_mail_head_t *head, char **from_contact_name) DEPRECATED;

/**
 * @open
 * @fn  email_set_from_contact_name_to_head(emf_mail_head_t *head, char *from_contact_name)
 * @brief	Sets  contact names of from address of a mail to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] from_contact_name	 	Specifies the contact names
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_from_contact_name_to_head(emf_mail_head_t *head, char *from_contact_name) DEPRECATED;

/**
 * @open
 * @fn  email_get_to_contact_name_from_head(emf_mail_head_t *head, char **to_contact_name)
 * @brief	Gets contact names of to addresses of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for contact names but just returns contact names in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] to_contact_name	Specifies the pointer to the memory to save contact names 
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_to_contact_name_from_head(emf_mail_head_t *head, char **to_contact_name) DEPRECATED;

/**
 * @open
 * @fn  email_set_to_contact_name_to_head(emf_mail_head_t *head, char *to_contact_name)
 * @brief	Sets  contact names of to address of a mail to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] to_contact_name	 	Specifies the contact names
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_to_contact_name_to_head(emf_mail_head_t *head, char *to_contact_name) DEPRECATED;

/**
 * @open
 * @fn  email_get_cc_contact_name_from_head(emf_mail_head_t *head, char **cc_contact_name)
 * @brief	Gets contact names of cc addresses of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for contact names but just returns contact names in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] cc_contact_name	Specifies the pointer to the memory to save contact names 
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_cc_contact_name_from_head(emf_mail_head_t *head, char **cc_contact_name) DEPRECATED;

/**
 * @open
 * @fn  email_set_cc_contact_name_to_head(emf_mail_head_t *head, char *cc_contact_name)
 * @brief	Sets  contact names of cc addresses of a mail to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] cc_contact_name	 	Specifies the contact names
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_cc_contact_name_to_head(emf_mail_head_t *head, char *cc_contact_name) DEPRECATED;

/**
 * @open
 * @fn  email_get_bcc_contact_name_from_head(emf_mail_head_t *head, char **bcc_contact_name)
 * @brief	Gets contact names of bcc address of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for contact names but just returns contact names in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] from_contact_name	Specifies the pointer to the memory to save contact names 
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_bcc_contact_name_from_head(emf_mail_head_t *head, char **bcc_contact_name) DEPRECATED;

/**
 * @open
 * @fn  email_set_bcc_contact_name_to_head(emf_mail_head_t *head, char *bcc_contact_name)
 * @brief	Sets  contact names of bcc addresses of a mail to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] bcc_contact_name	 	Specifies the contact names
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_bcc_contact_name_to_head(emf_mail_head_t *head, char *bcc_contact_name) DEPRECATED;

/**
 * @open
 * @fn  email_get_preview_body_text_from_head(emf_mail_head_t *head, char **preview_body_text)
 * @brief	Gets preview body text of a mail from the head object . If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for preview text but just returns preview text in head object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t. 
 * @param[out] preview_body_text	Specifies the pointer to the memory to save text
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_get_preview_body_text_from_head(emf_mail_head_t *head, char **preview_body_text) DEPRECATED;

/**
 * @open
 * @fn  email_set_preview_body_text_to_head(emf_mail_head_t *head, char *preview_body_text)
 * @brief	Sets preview body text of a mail to emf_mail_head_t(head). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] head	Specifies the pointer to emf_mail_head_t.
 * @param[in] preview_body_text	 	Specifies the preview body text
 * @exception 		none
 * @see                 emf_mail_head_t
 */
EXPORT_API int email_set_preview_body_text_to_head(emf_mail_head_t *head, char *preview_body_text) DEPRECATED;


// emf_mail_body_t -------------------------------------------------------------
/**
 * @open
 * @fn  email_get_plain_from_body(emf_mail_body_t *body, char **plain)
 * @brief	Gets the absolute path of file to contain email body (Plain Text) from the body object . If body is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for the path but just returns the path in body object.<br>
 *			If the body doesn't have a plain text file, *plain will point NULL.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] body	Specifies the pointer to emf_mail_body_t. 
 * @param[out] plain	Specifies the pointer to the memory to save the path of plain text file
 * @exception 		none
 * @see                 emf_mail_body_t
 */
EXPORT_API int email_get_plain_from_body(emf_mail_body_t *body, char **plain) DEPRECATED;

/**
 * @open
 * @fn  email_set_plain_to_body(emf_mail_body_t *body, char *plain)
 * @brief	Sets  the absolute path of file to contain email body (Plain Text)  to emf_mail_body_t(body). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] body	Specifies the pointer to emf_mail_body_t.
 * @param[in] plain	 	Specifies the path of plain text file
 * @exception 		none
 * @see                 emf_mail_body_t
 */
EXPORT_API int email_set_plain_to_body(emf_mail_body_t *body, char *plain) DEPRECATED;


/**
 * @open
 * @fn   email_get_plain_charset_from_body(emf_mail_body_t *body, char **plain_charset)
 * @brief	Gets a character set of plain text from the body object . If body is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for a character set of plain, but just returns the character set in body object.
 *			If the body doesn't have a plain text file, *plain_charset will point NULL.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] body	Specifies the pointer to emf_mail_body_t. 
 * @param[out] plain_charset	Specifies the pointer to the memory to save a character set of plain text
 * @exception 		none
 * @see                 emf_mail_body_t
 */
EXPORT_API int email_get_plain_charset_from_body(emf_mail_body_t *body, char **plain_charset) DEPRECATED;

/**
 * @open
 * @fn  email_set_plain_charset_to_body(emf_mail_body_t *body, char *plain_charset)
 * @brief	Sets  a character set of plain text to emf_mail_body_t(body). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] body	Specifies the pointer to emf_mail_body_t.
 * @param[in] plain_charset	 	Specifies the character set of plain text
 * @exception 		none
 * @see                 emf_mail_body_t
 */
EXPORT_API int email_set_plain_charset_to_body(emf_mail_body_t *body, char *plain_charset) DEPRECATED;


/**
 * @open
 * @fn  email_get_html_from_body(emf_mail_body_t *body, char **html)
 * @brief	Gets the absolute path of file to contain email body (HTML) from the body object . If body is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for the path but just returns the path in body object.<br>
 *			If the body doesn't have a html file, *html will point NULL.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] body	Specifies the pointer to emf_mail_body_t. 
 * @param[out] html	Specifies the pointer to the memory to save the path of html file
 * @exception 		none
 * @see                 emf_mail_body_t
 */
EXPORT_API int email_get_html_from_body(emf_mail_body_t *body, char **html) DEPRECATED;

/**
 * @open
 * @fn  email_set_html_to_body(emf_mail_body_t *body, char *html)
 * @brief	Sets  the absolute path of file to contain email body (HTML) to emf_mail_body_t(body). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] body	Specifies the pointer to emf_mail_body_t.
 * @param[in] html	 	Specifies the path of html file
 * @exception 		none
 * @see                 emf_mail_body_t
 */
EXPORT_API int email_set_html_to_body(emf_mail_body_t *body, char *html) DEPRECATED;


/**
 * @open
 * @fn  email_get_attachment_num_from_body(emf_mail_body_t *body, int *attachment_num)
 * @brief	Gets the number of attachments from the body object . If body is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for the  attachment_num, but just returns the number of attachments in body object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] body	Specifies the pointer to emf_mail_body_t. 
 * @param[out] attachment_num	Specifies the pointer to the memory to save the number of attachments
 * @exception 		none
 * @see                 emf_mail_body_t
 */
EXPORT_API int email_get_attachment_num_from_body(emf_mail_body_t *body, int *attachment_num) DEPRECATED;

/**
 * @open
 * @fn  email_set_attachment_num_to_body(emf_mail_body_t *body, int attachment_num)
 * @brief	Sets  the number of attachments to emf_mail_body_t(body). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] body	Specifies the pointer to emf_mail_body_t.
 * @param[in] attachment_num	 	Specifies the number of attachments
 * @exception 		none
 * @see                 emf_mail_body_t
 */
EXPORT_API int email_set_attachment_num_to_body(emf_mail_body_t *body, int attachment_num) DEPRECATED;


/**
 * @open
 * @fn  email_get_attachment_from_body(emf_mail_body_t *body, emf_attachment_info_t **attachment)
 * @brief	Gets the structure of attachment information from the body object . If body is NULL,  this returns EMF_ERROR_INVALID_PARAM. <br>
 *			This function doesn't allocate new memory for the attachment information, but just returns attachment information in body object.
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] body	Specifies the pointer to emf_mail_body_t. 
 * @param[out] attachment_num	Specifies the pointer to the memory to save attachment structure
 * @exception 		none
 * @see                 emf_mail_body_t, emf_attachment_info_t
 */
EXPORT_API int email_get_attachment_from_body(emf_mail_body_t *body, emf_attachment_info_t **attachment) DEPRECATED;

/**
 * @open
 * @fn  email_set_attachment_to_body(emf_mail_body_t *body, emf_attachment_info_t *attachment)
 * @brief	Sets   the structure of attachment information to emf_mail_body_t(body). If head is NULL,  this returns EMF_ERROR_INVALID_PARAM. 
 *
 * @return This function returns EMF_ERROR_NONE on success or error code (refer to EMF_ERROR_XXX) on failure.
 * @param[in] body	Specifies the pointer to emf_mail_body_t.
 * @param[in] attachment	 	Specifies the attachment structure
 * @exception 		none
 * @see                 emf_mail_body_t
 */
EXPORT_API int email_set_attachment_to_body(emf_mail_body_t *body, emf_attachment_info_t *attachment) DEPRECATED;



#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
* @} @}
*/


#endif /* __EMF_MAPI_RULE_H__ */

