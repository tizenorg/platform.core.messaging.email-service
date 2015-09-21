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
 * File :  email-core-mail.h
 * Desc :  Mail Operation Header
 *
 * Auth : 
 *
 * History : 
 *    2006.08.16  :  created
 *****************************************************************************/
#ifndef __EMAIL_CORE_MESSAGE_H__
#define __EMAIL_CORE_MESSAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "email-storage.h"

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__

#define MAX_SUBSET_STRING_SIZE 260	
#define MAX_IMAP_COMMAND_LENGTH 1000
#define MAX_TAG_SIZE 16


typedef struct _emf_uid_range_set
{
	char *uid_range;
	unsigned long lowest_uid;
	unsigned long highest_uid;
	
	struct _emf_uid_range_set *next;
	
} email_uid_range_set;

#endif

struct attachment_info
{
	int   type;                 /*  1 : inline 2 : attachment */
	char *name;                 /*  attachment filename */
	int   size;                 /*  attachment size */
	char *save;                 /*  content saving filename */
	int   drm;                  /*  0 : none 1 : object 2 : rights 3 : dcf */
	int   drm2;                 /*  0 : none 1 : FL 2 : CD 3 : SSD 4 : SD */
	char *attachment_mime_type; /*  attachment mime type */
	char *content_id;           /*  mime content id */
	int   save_status;
#ifdef __ATTACHMENT_OPTI__
	int   encoding;         /*  encoding  */
	char *section;          /*  section number */
#endif
	struct attachment_info *next;
};

/*
	MIME Structure Example

	(part 0)	multipart/mixed
	(part 1)		multipart/alternative
	(part 1.1)			text/plain		<- text message
	(part 1.2)			text/html		<- html message
	(part 2)		text/plain			<- text attachment


	(part 0)	multipart/related
	(part 1)		multipart/alternative
	(part 1.1)			text/plain			<- text message
	(part 1.2)			text/html			<- html message
	(part 2)		image/png				<- inline image
	(part 2)		image/png				<- inline image


	(part 0)	multipart/mixed
	(part 1.1)		multipart/related
	(part 2.1)			multipart/alternative
	(part 3.1)				text/plain(body)  <- text message
	(part 3.2)				text/html(body)	<- html message
	(part 2.2)			image/png(related)	<- inline image
	(part 1.2)		image/png(attachment)	<- image attachment
*/

/*  Text and Attachment Holde */
/* struct _m_content_info  */
/* 	int grab_type;	*/		/*  1 :  text and attachment list */
						/*  2 :  attachmen */
/* 	int file_no; */			/*  attachment sequence to be downloaded (min : 1 */
/* 	struct text_data  */
/* 		char *plain;	*/	/*  body plain tex */
/* 		char *plain_charset */ /*  charset of plai */
/* 		char *html; */ /*  body html tex */
/* 	} text */

/* 	struct attachment_info  */
/* 		int   type;	*/		/*  1 : inline 2 : attachmen */
/* 		char *name;	*/		/*  attachment filenam */
/* 		int   size;		*/	/*  attachment siz */
/* 		char *save;	*/		/*  content saving filenam */
/* 		struct attachment_info *next */
/* 	} *file */
/* } */

/* --------------------- MIME Structure --------------------------------- */
/*  MIME Header Parameter (Content-Type, Content-Disposition, ... */
struct _parameter {
	char				*name;			/*  parameter name */
	char				*value;			/*  parameter value */
	struct _parameter	*next;			/*  next paramete */
};

/*  Content-Disposition */
struct _disposition {
	char				*type;			/*  "inline" "attachment */
	struct _parameter	*parameter;		/*  "filename", .. */
};

/*  RFC822 Header */
struct _rfc822header {
	char				*return_path;	/*  error return path */
	char				*received;
	char				*date;
	char				*from;
	char				*subject;
	char				*sender;
	char				*to;
	char				*cc;
	char				*bcc;
	char				*reply_to;
	char				*priority;
	char				*ms_priority;
	char				*dsp_noti_to;
	char                            *message_id;
	char                            *content_type;
	char				*others;
};

/*  MIME Part Header */
struct _m_part_header {
	char				*type;			    /* text, image, audio, video, application, multipart, message */
	char				*subtype;		    /* plain, html, jpeg, .. */
	char				*encoding;		    /* encoding typ */
	struct _parameter	*parameter;		    /* content-type parameter  :  "boundary" "charset" .. */
	char				*desc;			    /* description */
	char				*disp_type;		    /* disposition type  :  "inline" "attachment", */
	struct _parameter	*disp_parameter;    /* disposition parameter  :  "filename", .. */
	char				*content_id;	    /* content id  :  it is inline  */
	char				*content_location;	/* content location  :  "inline" location  */
	char                            *priority;              /* Priority : 1, 3, 5 */
	char                            *ms_priority;           /* MS-Priority : HIGH, NORMAL, LOW */
};

/*  MIME Message Header */
struct _m_mesg_header {
	char				   *version;          /* MIME Version */
	struct _m_part_header  *part_header;	  /* MIME Part Header */
	/* char                   *message_context; */  /* Message-Context : Voice-message, Video-message, Fax-message... */
	/* int                     content_duration; */ /* Content-Duration */
	/* int                     x_content_pages;  */ /* X-Content-Pages */
	/* char                   *sensitivity; */     /* Sensitivity */
};

/*  MIME Multipart Body linked list */
typedef struct _m_body _m_body_t;
struct _m_part{
	_m_body_t              *body;             /* part body */
	struct _m_part         *next;             /* the next found part */
};

/*  MIME Multipart Body */
struct _m_body {
	struct _m_part_header  *part_header;	  /* MIME Part Header */
	struct _m_part          nested;           /* nested structure if contain multipart */
	char                   *text;             /* text if not contain multipart */
	int                     size;             /* text size if not contain multipart */
	char                   *holdingfile;
};

/*  MIME Message */
struct _m_mesg {
	struct _rfc822header    *rfc822header;    /* RFC822 Header */
	struct _m_mesg_header   *header;          /* MIME Message Header */
	struct _m_part           nested;          /* nested structure if contain multipart */
	char                    *text;            /* text if not contain multipart */
	int                      size;            /* text size if not contain multipart */
};

struct _m_content_info 
{
	int grab_type;	/*  1 :  download text and get attachment names (no saving attachment) -
							#define GRAB_TYPE_TEXT retrieve text and attachment names */
					/*  2 :  download attachment - #define GRAB_TYPE_ATTACHMENT retrieve only attachment */
	int file_no;	/*  attachment no to be download (min : 1) */
	int report;		/*  0 : Non 1 : DSN mail 2 : MDN mail 3 : mail to require MDN */
	int total_mail_size;
	int total_body_size;
	int total_attachment_size;
	int attachment_only;
	int content_type; /* 1 : signed */
	char *sections;

	struct text_data 
	{
		int   plain_save_status; 
		char *plain;             /*  body plain text */
		char *plain_charset;     /*  charset of body text */
		int   html_save_status;
		char *html;              /*  body html text */
		char *html_charset;      /*  charset of html text */
		char *mime_entity;
	} text;

	struct attachment_info *file;
	struct attachment_info *inline_file; /* only used for IMAP partial body download */
};

typedef enum {
	IMAP4_CMD_EXPUNGE,
	IMAP4_CMD_NOOP
} imap4_cmd_t;

typedef enum {
	POP3_CMD_NOOP
} pop3_cmd_t;

/**
 * Download a email nth-attachment from server.
 *
 * @param[in] mailbox		Specifies the mailbox to contain account ID.
 * @param[in] mail_id			Specifies the mail ID.
 * @param[in] nth				Specifies the buffer that a attachment number been saved. the minimum number is "1".
 * @param[in] callback		Specifies the callback function for retrieving download status.
 * @param[in] handle			Specifies the handle for stopping downloading.
 * @param[out] err_code	Specifies the error code returned.
 * @remarks This function is used for only IMAP mail.
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emcore_add_attachment(char *multi_user_name, int mail_id, email_attachment_data_t *attachment, int *err_code);        /* TODO : Remove duplicated function */
INTERNAL_FUNC int emcore_add_attachment_data(char *multi_user_name, int input_mail_id, email_attachment_data_t *input_attachment_data); /* TODO : Remove duplicated function */
INTERNAL_FUNC int emcore_delete_mail_attachment(char *multi_user_name, int attachment_id, int *err_code);
INTERNAL_FUNC int emcore_get_attachment_info(char *multi_user_name, int attachment_id, email_attachment_data_t **attachment, int *err_code);
INTERNAL_FUNC int emcore_get_attachment_data_list(char *multi_user_name, int input_mail_id, email_attachment_data_t **output_attachment_data, int *output_attachment_count);
INTERNAL_FUNC int emcore_free_attachment_data(email_attachment_data_t **attachment_data_list, int attachment_data_count, int *err_code);

INTERNAL_FUNC int emcore_gmime_download_attachment(char *multi_user_name, int mail_id, int nth,
		int cancellable, int event_handle, int auto_download, int *err_code);

INTERNAL_FUNC int emcore_gmime_download_body_sections(char *multi_user_name, void *mail_stream,
		int account_id, int mail_id, int with_attach, int limited_size,
		int event_handle, int cancellable, int auto_download, int *err_code);

INTERNAL_FUNC int emcore_move_mail(char *multi_user_name, int mail_ids[], int num, int dst_mailbox_id, int noti_param_1, int noti_param_2, int *err_code);

#ifdef __FEATURE_PARTIAL_BODY_DOWNLOAD__
INTERNAL_FUNC int emcore_insert_pbd_activity(email_event_partial_body_thd *local_activity, int *activity_id, int *err_code) ;
INTERNAL_FUNC int emcore_delete_pbd_activity(char *multi_user_name, int account_id, int mail_id, int activity_id, int *err_code);
#endif 

INTERNAL_FUNC int emcore_get_mail_contact_info(char *multi_user_name, email_mail_contact_info_t *contact_info, char *full_address, int *err_code);
INTERNAL_FUNC int emcore_get_mail_contact_info_with_update(char *multi_user_name, email_mail_contact_info_t *contact_info, char *full_address, int mail_id, int *err_code);
INTERNAL_FUNC int emcore_free_contact_info(email_mail_contact_info_t *contact_info, int *err_code);
INTERNAL_FUNC int emcore_sync_contact_info(char *multi_user_name, int mail_id, int *err_code);
INTERNAL_FUNC GList *emcore_get_recipients_list(char *multi_user_name, GList *old_recipients_list, char *full_address, int *err_code);
INTERNAL_FUNC int emcore_get_mail_address_info_list(char *multi_user_name, int mail_id, email_address_info_list_t **address_info_list, int *err_code);

INTERNAL_FUNC int emcore_set_sent_contacts_log(char *multi_user_name, emstorage_mail_tbl_t *input_mail_data);
INTERNAL_FUNC int emcore_set_received_contacts_log(char *multi_user_name, emstorage_mail_tbl_t *input_mail_data);
INTERNAL_FUNC int emcore_delete_contacts_log(char *multi_user_name, int input_account_id);


INTERNAL_FUNC int emcore_get_mail_display_name(char *multi_user_name, char *email_address, char **contact_display_name);
INTERNAL_FUNC int emcore_get_mail_display_name_internal(char *multi_user_name, char *email_address, char **contact_display_name);
INTERNAL_FUNC int emcore_get_mail_data(char *multi_user_name, int input_mail_id, email_mail_data_t **output_mail_data);

INTERNAL_FUNC int emcore_update_mail(char *multi_user_name, email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int sync_server);

INTERNAL_FUNC int emcore_delete_mails_from_local_storage(char *multi_user_name, int account_id, int *mail_ids, int num, int noti_param_1, int noti_param_2, int *err_code);
INTERNAL_FUNC int emcore_get_mail_msgno_by_uid(email_account_t *account, email_internal_mailbox_t *mailbox, char *uid, int *msgno, int *err_code);
INTERNAL_FUNC int emcore_expunge_mails_deleted_flagged_from_local_storage(char *multi_user_name, int input_mailbox_id);
INTERNAL_FUNC int emcore_expunge_mails_deleted_flagged_from_remote_server(char *multi_user_name, int input_account_id, int input_mailbox_id);

/**
 * Delete mails.
 *
 * @param[in] account_id      Specifies the account id.
 * @param[in] mail_id         Specifies the array for mail id.
 * @param[in] num             Specifies the number of id.
 * @param[in] from_server     Specifies whether mails is deleted from server.
 * @param[in] callback        Specifies the callback function for delivering status during deleting.
 * @param[in] noti_param_1    Specifies the first parameter for notification.
 * @param[in] noti_param_2    Specifies the second parameter for notification.
 * @param[out] err_code       Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int emcore_delete_mail(char *multi_user_name, int account_id, int mail_id[], int num, int from_server, int noti_param_1, int noti_param_2, int *err_code);

/**
 * Delete mails.
 *
 * @param[in] input_mailbox_id	Specifies the id of mailbox.
 * @param[in] input_from_server	Specifies whether mails is also deleted from server.
 * @param[out] err_code		Specifies the error code returned.
 * @remarks N/A
 * @return This function returns true on success or false on failure.
 */
INTERNAL_FUNC int   emcore_delete_all_mails_of_acount(char *multi_user_name, int input_account_id);
INTERNAL_FUNC int   emcore_delete_all_mails_of_mailbox(char *multi_user_name, 
														int input_account_id, 
														int input_mailbox_id, 
														int input_mailbox_type,
														int input_from_server, 
														int *err_code);

INTERNAL_FUNC void  emcore_free_mail_data_list(email_mail_data_t **mail_list, int count);
INTERNAL_FUNC void  emcore_free_mail_data(email_mail_data_t *mail);
INTERNAL_FUNC void  emcore_free_content_info(struct _m_content_info *cnt_info);
INTERNAL_FUNC void  emcore_free_attachment_info(struct attachment_info *attchment);

INTERNAL_FUNC int   emcore_move_mail_on_server(char *multi_user_name, int account_id, int src_mailbox_id,  int mail_ids[], int num, char *dest_mailbox, int *error_code);
INTERNAL_FUNC int   emcore_move_mail_on_server_by_server_mail_id(void *mail_stream, char *server_mail_id, char *dest_mailbox_name);
INTERNAL_FUNC int   emcore_move_mail_to_another_account(char *multi_user_name, int input_mail_id, int input_source_mailbox_id, int input_target_mailbox_id, int input_task_id);
INTERNAL_FUNC int   emcore_sync_flag_with_server(char *multi_user_name, int mail_id, int event_handle, int *err_code);
INTERNAL_FUNC int   emcore_sync_seen_flag_with_server(char *multi_user_name, int mail_ids[], int num, int event_handle, int *err_code);

INTERNAL_FUNC int   emcore_set_flags_field(char *multi_user_name, int account_id, int mail_ids[], int num, email_flags_field_type field_type, int value, int *err_code);
INTERNAL_FUNC int   emcore_convert_string_to_structure(const char *encoded_string, void **struct_var, email_convert_struct_type_e type);
INTERNAL_FUNC int   emcore_save_mail_file(char *multi_user_name, int account_id, int mail_id, int attachment_id, char *src_file_path, char *file_name, char *full_path, char *virtual_path, int *err_code);

#ifdef __FEATURE_BULK_DELETE_MOVE_UPDATE_REQUEST_OPTI__
INTERNAL_FUNC int   emcore_sync_flags_field_with_server(char *multi_user_name, int mail_ids[], int num, email_flags_field_type field_type, int value, int *err_code);
INTERNAL_FUNC int   emcore_move_mail_on_server_ex(char *multi_user_name, int account_id, int src_mailbox_id,  int mail_ids[], int num, int dest_mailbox_id, int *error_code);
#endif

#ifdef __ATTACHMENT_OPTI__
INTERNAL_FUNC int emcore_download_attachment_bulk(/*email_mailbox_t *mailbox, */ int account_id, int mail_id, char *nth, int event_handle, int *err_code);
#endif
INTERNAL_FUNC int   emcore_mail_filter_by_rule(char *multi_user_name, email_rule_t *filter_info, int *err_code);
INTERNAL_FUNC int   emcore_add_rule(char *multi_user_name, email_rule_t *filter_info);
INTERNAL_FUNC int   emcore_update_rule(char *multi_user_name, int filter_id, email_rule_t *filter_info);
INTERNAL_FUNC int   emcore_delete_rule(char *multi_user_name, int filter_id);

/**
 * Search the mails on server
 *
 * @param[in] account_id                Specifies the id of account
 * @param[in] mailbox_id                Specifies the id of mailbox
 * @param[in] input_search_filter       Specifies the filter list for searching field
 * @param[in] input_search_filter_count Specifies the filter count
 * @param[in] cancellable               Specifies the cancellable
 * @param[in] handle                    Specifies the handle for searching mails
 * @remarks N/A
 * @return EMAIL_ERROR_NONE on success or an error code (refer to EMAIL_ERROR_XXX) on failure 
 */

INTERNAL_FUNC int   emcore_search_on_server(char *multi_user_name, 
											int account_id, 
											int mailbox_id, 
											email_search_filter_t *input_search_filter, 
											int input_search_filter_count, 
											int cancellable,
											int event_handle);
#ifdef __cplusplus
}
#endif /* __cplusplus */


#define	EMAIL_SIGNAL_FILE_DELETED	1
#define	EMAIL_SIGNAL_DB_DELETED	2
INTERNAL_FUNC int *emcore_init_pipe_for_del_account ();
INTERNAL_FUNC void emcore_send_signal_for_del_account (int signal);

#endif
/* EOF */
