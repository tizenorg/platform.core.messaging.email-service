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


#ifndef __EMAIL_API_MAIL_H__
#define __EMAIL_API_MAIL_H__

#include "email-types.h"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * @file email-api-mail.h
 */

/**
 * @internal
 * @ingroup EMAIL_SERVICE_FRAMEWORK
 * @defgroup EMAIL_SERVICE_MAIL_MODULE Mail API
 * @brief Mail API is a set of operations to manage mail like add, update, delete or get mail related details.
 *
 * @section EMAIL_SERVICE_MAIL_MODULE_HEADER Required Header
 *   \#include <email-api-mail.h>
 *
 * @section EMAIL_SERVICE_MAIL_MODULE_OVERVIEW Overview
 * Mail API is a set of operations to manage mail like add, update, delete or get mail related details.
 */

/**
 * @internal
 * @addtogroup EMAIL_SERVICE_MAIL_MODULE
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Saves a mail.
 * @details This function is invoked when a user wants to add a mail.\n
 *          If the option from_eas is 1 then this will save the message on server as well as on locally.\n
 *          If the incoming_server_type is EMAIL_SERVER_TYPE_POP3 then from_eas value will be 0.\n
 *          If the incoming_server_type is EMAIL_SERVER_TYPE_IMAP4 then from_eas value will be 1/0.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] input_mail_data             The mail to be saved
 * @param[in] input_attachment_data_list  The mailbox structure for saving email
 * @param[in] input_attachment_count      The mail attachment count
 * @param[in] input_meeting_request       Specifies if the mail comes from composer
 * @param[in] input_from_eas              Specifies if the mail will be saved on server
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_data_t and #email_mailbox_t
 *
 * @code
 * #include "email-api-mail.h"
 * int _test_add_mail ()
 * {
 *  int                    i = 0;
 *  int                    account_id = 0;
 *  int                    from_eas = 0;
 *  int                    attachment_count = 0;
 *  int                    err = EMAIL_ERROR_NONE;
 *  char                   arg[50] = { 0 , };
 *  char                  *body_file_path = MAILHOME"/tmp/mail.txt";
 *  email_mailbox_t         *mailbox_data = NULL;
 *  email_mail_data_t       *test_mail_data = NULL;
 *  email_attachment_data_t *attachment_data = NULL;
 *  email_meeting_request_t *meeting_req = NULL;
 *  FILE                  *body_file;
 *
 *  printf("\n > Enter account id : ");
 *  scanf("%d", &account_id);
 *
 *
 *  memset(arg, 0x00, 50);
 *  printf("\n > Enter mailbox name : ");
 *  scanf("%s", arg);
 *
 *  email_get_mailbox_by_name(account_id, arg, &mailbox_data);
 *
 *  test_mail_data = malloc(sizeof(email_mail_data_t));
 *  memset(test_mail_data, 0x00, sizeof(email_mail_data_t));
 *
 *  printf("\n From EAS? [0/1]> ");
 *  scanf("%d", &from_eas);
 *
 *  test_mail_data->account_id        = account_id;
 *  test_mail_data->save_status       = 1;
 *  test_mail_data->flags_seen_field  = 1;
 *  test_mail_data->file_path_plain   = strdup(body_file_path);
 *  test_mail_data->mailbox_id        = mailbox_data->mailbox_id;
 *  test_mail_data->mailbox_type      = mailbox_data->mailbox_type;
 *  test_mail_data->full_address_from = strdup("<test1@test.com>");
 *  test_mail_data->full_address_to   = strdup("<test2@test.com>");
 *  test_mail_data->full_address_cc   = strdup("<test3@test.com>");
 *  test_mail_data->full_address_bcc  = strdup("<test4@test.com>");
 *  test_mail_data->subject           = strdup("Meeting request mail");
 *
 *  body_file = fopen(body_file_path, "w");
 *
 *  for(i = 0; i < 500; i++)
 *      fprintf(body_file, "X2 X2 X2 X2 X2 X2 X2");
 *  fflush(body_file);
 *  fclose(body_file);
 *
 *  printf(" > Attach file? [0/1] : ");
 *  scanf("%d",&attachment_count);
 *
 *  if ( attachment_count )  {
 *      memset(arg, 0x00, 50);
 *      printf("\n > Enter attachment name : ");
 *      scanf("%s", arg);
 *
 *      attachment_data = malloc(sizeof(email_attachment_data_t));
 *
 *      attachment_data->attachment_name  = strdup(arg);
 *
 *      memset(arg, 0x00, 50);
 *      printf("\n > Enter attachment absolute path : ");
 *      scanf("%s",arg);
 *
 *      attachment_data->attachment_path  = strdup(arg);
 *      attachment_data->save_status      = 1;
 *      test_mail_data->attachment_count  = attachment_count;
 *  }
 *
 *  printf("\n > Meeting Request? [0: no, 1: yes (request from server), 2: yes (response from local)]");
 *  scanf("%d", &(test_mail_data->meeting_request_status));
 *
 *  if ( test_mail_data->meeting_request_status == 1
 *      || test_mail_data->meeting_request_status == 2 ) {
 *      time_t current_time;
 *      meeting_req = malloc(sizeof(email_meeting_request_t));
 *      memset(meeting_req, 0x00, sizeof(email_meeting_request_t));
 *
 *      meeting_req->meeting_response     = 1;
 *      current_time = time(NULL);
 *      gmtime_r(&current_time, &(meeting_req->start_time));
 *      gmtime_r(&current_time, &(meeting_req->end_time));
 *      meeting_req->location = malloc(strlen("Seoul") + 1);
 *      memset(meeting_req->location, 0x00, strlen("Seoul") + 1);
 *      strcpy(meeting_req->location, "Seoul");
 *      strcpy(meeting_req->global_object_id, "abcdef12345");
 *
 *      meeting_req->time_zone.offset_from_GMT = 9;
 *      strcpy(meeting_req->time_zone.standard_name, "STANDARD_NAME");
 *      gmtime_r(&current_time, &(meeting_req->time_zone.standard_time_start_date));
 *      meeting_req->time_zone.standard_bias = 3;
 *
 *      strcpy(meeting_req->time_zone.daylight_name, "DAYLIGHT_NAME");
 *      gmtime_r(&current_time, &(meeting_req->time_zone.daylight_time_start_date));
 *      meeting_req->time_zone.daylight_bias = 7;
 *
 *  }
 *
 *  if((err = email_add_mail(test_mail_data, attachment_data, attachment_count, meeting_req, from_eas)) != EMAIL_ERROR_NONE)
 *      printf("email_add_mail failed. [%d]\n", err);
 *  else
 *      printf("email_add_mail success.\n");
 *
 *  if(attachment_data)
 *      email_free_attachment_data(&attachment_data, attachment_count);
 *
 *  if(meeting_req)
 *      email_free_meeting_request(&meeting_req, 1);
 *
 *  email_free_mail_data(&test_mail_data, 1);
 *  email_free_mailbox(&mailbox_data, 1);
 *
 *  printf("saved mail id = [%d]\n", test_mail_data->mail_id);
 *
 *  return 0;
 * }
 * @endcode
 */
EXPORT_API int email_add_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas);

/**
 * @brief Adds a read receipt mail.
 * @details This function is invoked when a user receives a mail with read report enable and wants to send a read report for the same.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]   input_read_mail_id      The ID of mail which has been read
 * @param[out]  output_receipt_mail_id  The receipt mail ID
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_add_read_receipt(int input_read_mail_id, int *output_receipt_mail_id);

/**
 * @brief Updates an existing email information.
 * @details This function is invoked when a user wants to change some existing email information with new email information.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] input_mail_data             The mail ID
 * @param[in] input_attachment_data_list  The pointer of attachment data
 * @param[in] input_attachment_count      The number of attachment data
 * @param[in] input_meeting_request       The meeting request data
 * @param[in] input_from_eas              Specifies whether sync server
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_data_t
 *
 * @code
 * #include "email-api-account.h"
 * int email_test_update_mail()
 * {
 *  int                    mail_id = 0;
 *  int                    err = EMAIL_ERROR_NONE;
 *  int                    test_attachment_data_count = 0;
 *  char                   arg[50];
 *  email_mail_data_t       *test_mail_data = NULL;
 *  email_attachment_data_t *test_attachment_data_list = NULL;
 *  email_meeting_request_t *meeting_req = NULL;
 *
 *  printf("\n > Enter mail id : ");
 *  scanf("%d", &mail_id);
 *
 *  email_get_mail_data(mail_id, &test_mail_data);
 *
 *  printf("\n > Enter Subject: ");
 *  scanf("%s", arg);
 *
 *  test_mail_data->subject= strdup(arg);
 *
 *
 *
 *  if (test_mail_data->attachment_count > 0) {
 *      if ( (err = email_get_attachment_data_list(mail_id, &test_attachment_data_list, &test_attachment_data_count)) != EMAIL_ERROR_NONE ) {
 *          printf("email_get_meeting_request() failed [%d]\n", err);
 *          return -1;
 *      }
 *  }
 *
 *  if ( test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_REQUEST
 *      || test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_RESPONSE
 *      || test_mail_data->meeting_request_status == EMAIL_MAIL_TYPE_MEETING_ORIGINATINGREQUEST) {
 *
 *      if ( (err = email_get_meeting_request(mail_id, &meeting_req)) != EMAIL_ERROR_NONE ) {
 *          printf("email_get_meeting_request() failed [%d]\n", err);
 *          return -1;
 *      }
 *
 *      printf("\n > Enter meeting response: ");
 *      scanf("%d", (int*)&(meeting_req->meeting_response));
 *  }
 *
 *  if ( (err = email_update_mail(test_mail_data, test_attachment_data_list, test_attachment_data_count, meeting_req, 0)) != EMAIL_ERROR_NONE)
 *          printf("email_update_mail failed.[%d]\n", err);
 *      else
 *          printf("email_update_mail success\n");
 *
 *  if(test_mail_data)
 *      email_free_mail_data(&test_mail_data, 1);
 *
 *  if(test_attachment_data_list)
 *      email_free_attachment_data(&test_attachment_data_list, test_attachment_data_count);
 *
 *  if(meeting_req)
 *      email_free_meeting_request(&meeting_req, 1);
 *
 *  return 0;
 * }

 * @endcode
 */
EXPORT_API int email_update_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas);

/**
 * @brief Updates an individual attribute of the mail data.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  input_account_id      The ID of account
 * @param[in]  input_mail_id_array   The array list of mail IDs
 * @param[in]  input_mail_id_count   The count of mail ID array
 * @param[in]  input_attribute_type  The attribute type to update
 * @param[in]  input_value           The value of attribute
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_update_mail_attribute(int input_account_id, int *input_mail_id_array, int input_mail_id_count, email_mail_attribute_type input_attribute_type, email_mail_attribute_value_t input_value);

/**
 * @brief Gets the mail count.
 * @details This function is invoked when a user wants to know how many total mails and out of that
 *          how many unseen mails are there in a given mailbox.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  mailbox  The pointer of mailbox structure
 * @param[out] total    The total email count
 * @param[out] unseen   The unread email count
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 *
 * @code
 *  #include "email-api-account.h"
 *  bool
 *  _api_sample_count_mail()
 *  {
 *      int total = 0;
 *      int unseen = 0;
 *      email_list_filter_t *filter_list = NULL;
 *      int err = EMAIL_ERROR_NONE;
 *      int i = 0;
 *      filter_list = malloc(sizeof(email_list_filter_t) * 3);
 *      memset(filter_list, 0 , sizeof(email_list_filter_t) * 3);
 *
 *      filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 *      filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
 *      filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 *      filter_list[0].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 *      filter_list[0].list_filter_item.rule.case_sensitivity              = false;
 *
 *      filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
 *      filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;
 *
 *      filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 *      filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_TO;
 *      filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 *      filter_list[2].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 *      filter_list[2].list_filter_item.rule.case_sensitivity              = false;
 *
 *      if(EMAIL_ERROR_NONE == email_count_mail(filter_list, 3, &total, &unseen))
 *          printf("\n Total: %d, Unseen: %d \n", total, unseen);
 *  }
 * @endcode
 */
EXPORT_API int email_count_mail(email_list_filter_t *input_filter_list, int input_filter_count, int *output_total_mail_count, int *output_unseen_mail_count);

/**
 * @brief Gets the max count of mails which can be supported by email-service.
 *
 * @param[out]  count  The max count of mails which can be supported by email-service
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @code
 *      #include "email-api-account.h"
 *          bool
 *          _api_sample_get_max_mail_count()
 *          {
 *              int max_count = -1;
 *
 *              if(EMAIL_ERROR_NONE == email_get_max_mail_count(&max_count))
 *                  printf("\n\t>>>>> email_get_max_mail_count() returns [%d]\n\n", max_count);
 *      }
 * @endcode
 */
EXPORT_API int email_get_max_mail_count(int *count);

/**
 * @brief Deletes a mail or multiple mails.
 * @details Based on from_server value, this function will delete a mail or multiple mails from the server or locally.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @remarks If the incoming_server_type is #EMAIL_SERVER_TYPE_POP3 then from_server value will be @c 0. \n
 *          If the incoming_server_type is #EMAIL_SERVER_TYPE_IMAP4 then from_server value will be 1/0.
 *
 * @param[in] mailbox      Reserved
 * @param[in] mail_ids[]   The array of mail IDs
 * @param[in] num          The number of mail IDs
 * @param[in] from_server  The flag that specifies whether mails are deleted from server
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @code
 *  #include "email-api-account.h"
 *  bool
 *  _api_sample_delete_mail()
 *  {
 *      int count, i, mail_id=0, mailbox_id =0;
 *
 *      printf("\n > Enter Mail_id: ");
 *      scanf("%d",&mail_id);
 *      printf("\n > Enter Mailbox ID: ");
 *      scanf("%d",&mailbox_id);
 *      if(EMAIL_ERROR_NONE == email_delete_mail(mailbox_id, &mail_id, 1, 1))
 *          printf("\n email_delete_mail success");
 *      else
 *          printf("\n email_delete_mail failed");
 *  }
 * @endcode
 */
EXPORT_API int email_delete_mail(int input_mailbox_id, int *input_mail_ids, int input_num, int input_from_server);


/**
 * @brief Deletes all mails from a mailbox.
 * @details  If the incoming_server_type is #EMAIL_SERVER_TYPE_POP3 then @a from_server value will be @c 0. \n
 *           If the incoming_server_type is #EMAIL_SERVER_TYPE_IMAP4 then @a from_server value will be 1/0.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] mailbox      The structure of mailbox
 * @param[in] from_server  The flag that specifies whether mails are also deleted from server
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 
 * @code
 *  #include "email-api-account.h"
 *  bool
 *  _api_sample_delete_all_mails_in_mailbox()
 *  {
 *      int count, mailbox_id =0;
 *
 *      printf("\n > Enter mailbox_id: ");
 *      scanf("%d",&mailbox_id);
 *
 *      if (EMAIL_ERROR_NONE != email_delete_all_mails_in_mailbox(mailbox_id, 0))
 *          printf("email_delete_all_mails_in_mailbox failed");
 *      else
 *          printf("email_delete_all_mails_in_mailbox Success");
 *  }
 * @endcode
 */
EXPORT_API int email_delete_all_mails_in_mailbox(int input_mailbox_id, int input_from_server);

/**
 * @brief Deletes email data from the storage.
 * @details This API will be used by Settings Application.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return #EMAIL_ERROR_NONE on success,
 *         otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @code
 *  #include "email-api-account.h"
 *  bool
 *  _api_sample_clear_mail_data()
 *  {
 *      if(EMAIL_ERROR_NONE == email_clear_mail_data())
 *          //success
 *      else
 *          //failure
 *  }
 * @endcode
 */
EXPORT_API int  email_clear_mail_data();


/**
 * @brief Appends an attachment to an email.
 * @details This function is invoked when a user wants to add attachment to an existing mail.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] mail_id     The mail ID
 * @param[in] attachment  The structure of attachment
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_attachment_data_t
 *
 * @code
 *  #include "email-api-account.h"
 *  bool
 *  _api_sample_mail_add_attachment()
 *  {
 *      int mail_id = 0;
 *      email_attachment_data_t attachment;
 *
 *      printf("\n > Enter Mail Id: ");
 *      scanf("%d", &mail_id);
 *      memset(&attachment, 0x00, sizeof(email_attachment_data_t));
 *      printf("\n > Enter attachment name: ");
 *      attachment.name = strdup("Test");
 *      printf("\n > Enter attachment absolute path: ");
 *      attachment.savename = strdup("/tmp/test.txt");
 *      attachment.next = NULL;
 *      if(EMAIL_ERROR_NONE != email_add_attachment(mail_id, &attachment))
 *          printf("email_add_attachment failed\n");
 *      else
 *          printf(email_add_attachment success\n");
 *  }
 * @endcode
 */
EXPORT_API int email_add_attachment(int mail_id, email_attachment_data_t* attachment);


/**
 * @brief Deletes an attachment from email.
 * @details This function is invoked when a user wants to delete a attachment from an existing mail based on mail ID and attachment ID.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] mail_id        The mail ID
 * @param[in] attachment_id  The attachment ID
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 *
 * @code
 *  #include "email-api-account.h"
 *  bool
 *  _api_sample_mail_delete_attachment()
 *  {
 *
 *      if(EMAIL_ERROR_NONE != email_delete_attachment(1))
 *          //failure
 *      else
 *          //success
 *  }
 * @endcode
 */
EXPORT_API int email_delete_attachment(int attachment_id);

/**
 * @brief Gets a mail attachment.
 * @details This function is invoked when a user wants to get the attachment information based on an attachment ID for the specified mail ID.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  mail_id        The mail ID
 * @param[in]  attachment_id  The buffer that an attachment ID is saved
 * @param[out] attachment     The returned attachment
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t and #email_mail_attachment_info_t
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_mail_get_attachment_info()
 *      {
 *      email_mail_attachment_info_t *mail_attach_info = NULL;
 *      int mail_id = 0;
 *      char arg[10];
 *      int err = EMAIL_ERROR_NONE;
 *
 *
 *      printf("\n > Enter Mail Id: ");
 *      scanf("%d", &mail_id);
 *      printf("> attachment Id\n");
 *      scanf("%s",arg);
 *      if (EMAIL_ERROR_NONE != email_get_attachment_data(mail_id, &mail_attach_info))
 *          printf("email_get_attachment_data failed\n");
 *      else
 *      {
 *          printf("email_get_attachment_data SUCCESS\n");
 *          //do something
 *          email_free_attachment_data(&mail_attach_info,1);
 *      }
 *      }
 * @endcode
 */
EXPORT_API int email_get_attachment_data(int attachment_id, email_attachment_data_t** attachment);

/**
 * @brief Gets a list of mail attachments.
 * @details This function is invoked when a user wants to get the the attachment list information based on the mail ID.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  input_mail_id            The mail ID
 * @param[out] output_attachment_data   The returned attachment list is save here
 * @param[out] output_attachment_count  The returned count of attachment list
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_attachment_data_t
 */
EXPORT_API int email_get_attachment_data_list(int input_mail_id, email_attachment_data_t **output_attachment_data, int *output_attachment_count);

/**
 * @brief Frees the allocated memory for email attachments.
 *
 * @since_tizen 2.3
 *
 * @param[in] atch_info  The pointer of mail attachment structure pointer
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_attachment_info_t
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_mail_free_attachment_info()
 *      {
 *      email_mailbox_t mailbox;
 *      email_mail_attachment_info_t *mail_attach_info = NULL;
 *      int mail_id = 0,account_id = 0;
 *      char arg[10];
 *      int err = EMAIL_ERROR_NONE;
 *
 *      memset(&mailbox, 0x00, sizeof(email_mailbox_t));
 *
 *      printf("\n > Enter Mail Id: ");
 *      scanf("%d", &mail_id);
 *      printf("\n > Enter account Id: ");
 *      scanf("%d", &account_id);
 *      printf("> attachment Id\n");
 *      scanf("%s",arg);
 *      mailbox.account_id = account_id;
 *      if (EMAIL_ERROR_NONE != email_get_attachment_data(&mailbox, mail_id, &mail_attach_info))
 *          printf("email_get_attachment_data failed\n");
 *      else
 *      {
 *          printf("email_get_attachment_data SUCCESS\n");
 *          //do something
 *          email_free_attachment_info(&mail_attach_info,1);
 *      }
 *      }
 * @endcode
 */
EXPORT_API int email_free_attachment_data(email_attachment_data_t **attachment_data_list, int attachment_data_count);

/*-----------------------------------------------------------
                    Mail Information API
  -----------------------------------------------------------*/


/**
 * @brief Queries the mail list information from the DB based on the mailbox name.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]      conditional_clause_string  The where clause string
 * @param[in/out]  mail_list                  The pointer to the structure #email_mail_data_t
 * @param[in/out]  result_count               The number of mails returned
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_list_item_t
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_query_mail()
 *      {
 *      email_mail_data_t *mail_list = NULL;
 *      char conditional_clause_string[500];
 *      int result_count = 0;
 *
 *      memset(conditional_clause_string, 0x00, 10);
 *      printf("\n > Enter where clause: ");
 *      scanf("%s", conditional_clause_string);
 *
 *
 *      if (EMAIL_ERROR_NONE != email_query_mails(conditional_clause_string, &mail_list, &result_count))
 *          printf("email_query_mails failed \n");
 *      else {
 *          printf("Success\n");
 *          //do something
 *          free(mail_list);
 *      }
 *
 *      }
 * @endcode
 */
EXPORT_API int email_query_mails(char *conditional_clause_string, email_mail_data_t** mail_list,  int *result_count);

/**
 * @brief Queries the mail list information from the DB.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]      input_conditional_clause_string  The where clause string
 * @param[in/out]  output_mail_list                 The pointer to the structure #email_mail_list_item_t
 * @param[in/out]  output_result_count              The number of mails returned
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_list_item_t
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_query_mail_list()
 *      {
 *      email_mail_list_item_t *mail_list = NULL;
 *      char conditional_clause_string[500];
 *      int result_count = 0;
 *
 *      memset(conditional_clause_string, 0x00, 10);
 *      printf("\n > Enter where clause: ");
 *      scanf("%s", conditional_clause_string);
 *
 *
 *      if (EMAIL_ERROR_NONE != email_query_mail_list(conditional_clause_string, &mail_list, &result_count))
 *          printf("email_query_mail_list failed \n");
 *      else {
 *          printf("Success\n");
 *          //do something
 *          free(mail_list);
 *      }
 *
 *      }
 * @endcode
 */
EXPORT_API int email_query_mail_list(char *input_conditional_clause_string, email_mail_list_item_t** output_mail_list,  int *output_result_count);

/**
 * @brief Gets a mail by its mail ID.
 * @details This function is invoked when a user wants to get a mail based on mail ID existing in the DB.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  input_mail_id     The mail ID
 * @param[out] output_mail_data  The returned mail is save here
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_data_t
 *
 * @code
 *    #include "email-api-account.h"
 *      bool
 *      _api_sample_get_mail_data()
 *      {
 *      email_mail_data_t *mail = NULL;
 *      int mail_id = 0 ;
 *      int err = EMAIL_ERROR_NONE;
 *
 *      printf("\n > Enter mail id: ");
 *      scanf("%d", &mail_id);
 *
 *      if (EMAIL_ERROR_NONE != email_get_mail_data(mail_id, &mail))
 *          printf("email_get_mail_data failed\n");
 *      else
 *      {
 *          printf("email_get_mail_data SUCCESS\n");
 *          //do something
 *          email_free_mail_data(&mail,1);
 *      }
 *      }
 * @endcode
 */

EXPORT_API int email_get_mail_data(int input_mail_id, email_mail_data_t **output_mail_data);


/**
 * @brief Frees the allocated memory for emails.
 *
 * @since_tizen 2.3
 *
 * @param[in] mail_list  The pointer of mail structure pointer
 * @param[in] count      The count of mails
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_data_t
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_free_mail()
 *      {
 *      email_mail_data_t *mail;
 *
 *      //fill the mail structure
 *      //count - number of mail structure a user want to free
 *      if(EMAIL_ERROR_NONE == email_free_mail_data(&mail,count))
 *          //success
 *      else
 *          //failure
 *
 *      }
 * @endcode
 */
EXPORT_API int email_free_mail_data(email_mail_data_t** mail_list, int count);

/*-----------------------------------------------------------
                     Mail Flag API
  -----------------------------------------------------------*/

/**
 * @brief Changes an email flags field.
 * @details If the incoming_server_type is EMAIL_SERVER_TYPE_POP3 then from_server value will be 0. \n
 *          If the incoming_server_type is EMAIL_SERVER_TYPE_IMAP4 then from_server value will be 1/0.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] account_id  The account ID
 * @param[in] mail_ids    The array of mail IDs
 * @param[in] num         The number of mail IDs
 * @param[in] field_type  The field type to be set \n 
 *                        See #email_flags_field_type.
 * @param[in] value       The value to be set
 * @param[in] onserver    The flag indicating whether mail flag updating is in server
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_set_flags_field()
 *      {
 *          int account_id = 0;
 *          int mail_id = 0;
 *          int err = EMAIL_ERROR_NONE;
 *
 *          printf("\n > Enter account id: ");
 *          scanf("%d", &account_id);
 *
 *          printf("\n > Enter mail id: ");
 *          scanf("%d", &mail_id);
 *          if (EMAIL_ERROR_NONE != email_set_flags_field(&account_id, &mail_id, EMAIL_FLAGS_SEEN_FIELD, 1, 0))
 *              printf("email_set_flags_field failed\n");
 *          else
 *          {
 *              printf("email_set_flags_field succeed\n");
 *              //do something
 *          }
 *      }
 * @endcode
 */
EXPORT_API int email_set_flags_field(int account_id, int *mail_ids, int num, email_flags_field_type field_type, int value, int onserver);

/* -----------------------------------------------------------
                    Mail Move API
   -----------------------------------------------------------*/

/**
 * @brief Moves an email to another mailbox.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] mail_id                  The array of mail ID
 * @param[in] num                      The count of mail IDs
 * @param[in] input_target_mailbox_id  The mailbox ID for moving email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_move_mail_to_mailbox()
 *      {
 *      int mail_id = 0;
 *      int mailbox_id = 0;
 *      int err = EMAIL_ERROR_NONE;

 *      printf("\n > Enter mail_id: ");
 *      scanf("%d",&mail_id);
 *
 *      printf("\n > Enter target mailbox_id: ");
 *      scanf("%d",&mailbox_id);
 *
 *      if(EMAIL_ERROR_NONE == email_move_mail_to_mailbox(&mail_id,	1, mailbox_id))
 *         printf("Success\n");
 *
 *      }
 * @endcode
 */
EXPORT_API int email_move_mail_to_mailbox(int *mail_ids, int num, int input_target_mailbox_id);

/**
 * @brief Moves all emails to another mailbox.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] input_source_mailbox_id  The source mailbox ID for moving email
 * @param[in] input_target_mailbox_id  The destination mailbox ID for moving email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mailbox_t
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_move_all_mails_to_mailbox()
 *      {
 *      int src_mailbox_id;
 *      int dst_mailbox_id;
 *      int total_count;
 *      int err = EMAIL_ERROR_NONE;
 *      char temp[128];
 *
 *
 *      // input mailbox information : need  account_id and name (src & dest)
 *      printf("src mail maibox id> ");
 *      scanf("%d", &src_mailbox_id);
 *
 *      // Destination mailbox
 *      printf("dest mailbox id> ");
 *      scanf("%d", &dst_mailbox_id);
 *
 *      if( EMAIL_ERROR_NONE == email_move_all_mails_to_mailbox(src_mailbox_id, dst_mailbox_id))
 *      {
 *          printf("  fail email_move_all_mails_to_mailbox: \n");
 *      }
 *      else
 *          //success
 * }
 * @endcode
 */
EXPORT_API int email_move_all_mails_to_mailbox(int input_source_mailbox_id, int input_target_mailbox_id);

/**
 * @brief Moves mails to the mailbox of an another account.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  input_source_mailbox_id  The source mailbox ID for moving email
 * @param[in]  mail_id_array            The source mail ID array for moving email
 * @param[in]  mail_id_count            The count of source mail for moving email
 * @param[in]  input_target_mailbox_id  The destination mailbox ID for moving email
 * @param[out] output_task_id           The Task ID for handling the task
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          or an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_move_mails_to_mailbox_of_another_account(int input_source_mailbox_id, int *mail_id_array, int mail_id_count, int input_target_mailbox_id, int *output_task_id);


/**
 * @brief Gets the Mail List information from the DB.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]     input_filter_list         The filter list
 * @param[in]     input_filter_count        The number of filter
 * @param[in]     input_sorting_rule_list   The sorting rule list
 * @param[in]     input_sorting_rule_count  The number of sorting rule
 * @param[in]     input_start_index         The start index for LIMIT clause of SQL query
 * @param[in]     input_limit_count         The limit count for LIMIT clause of SQL query
 * @param[in/out] output_mail_list          The pointer to the structure #email_mail_list_item_t
 * @param[in/out] output_result_count       The number of mails returned
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_list_item_t
 *
 * @code
 * email_list_filter_t *filter_list = NULL;
 * email_list_sorting_rule_t *sorting_rule_list = NULL;
 * email_mail_list_item_t *result_mail_list = NULL;
 * int result_mail_count = 0;
 * int err = EMAIL_ERROR_NONE;
 * int i = 0;
 * filter_list = malloc(sizeof(email_list_filter_t) * 9);
 * memset(filter_list, 0 , sizeof(email_list_filter_t) * 9);
 *
 * filter_list[0].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * filter_list[0].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
 * filter_list[0].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * filter_list[0].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * filter_list[0].list_filter_item.rule.case_sensitivity              = false;
 *
 * filter_list[1].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
 * filter_list[1].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;
 *
 * filter_list[2].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * filter_list[2].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_TO;
 * filter_list[2].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * filter_list[2].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * filter_list[2].list_filter_item.rule.case_sensitivity              = false;
 *
 * filter_list[3].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
 * filter_list[3].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;
 *
 * filter_list[4].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * filter_list[4].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_CC;
 * filter_list[4].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * filter_list[4].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * filter_list[4].list_filter_item.rule.case_sensitivity              = false;
 *
 * filter_list[5].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
 * filter_list[5].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;
 *
 * filter_list[6].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * filter_list[6].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_BCC;
 * filter_list[6].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * filter_list[6].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * filter_list[6].list_filter_item.rule.case_sensitivity              = false;
 *
 * filter_list[7].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_OPERATOR;
 * filter_list[7].list_filter_item.operator_type                      = EMAIL_LIST_FILTER_OPERATOR_OR;
 *
 * filter_list[8].list_filter_item_type                               = EMAIL_LIST_FILTER_ITEM_RULE;
 * filter_list[8].list_filter_item.rule.target_attribute              = EMAIL_MAIL_ATTRIBUTE_FROM;
 * filter_list[8].list_filter_item.rule.rule_type                     = EMAIL_LIST_FILTER_RULE_INCLUDE;
 * filter_list[8].list_filter_item.rule.key_value.string_type_value   = strdup("RE");
 * filter_list[8].list_filter_item.rule.case_sensitivity              = false;
 *
 * sorting_rule_list = malloc(sizeof(email_list_sorting_rule_t) * 2);
 * memset(sorting_rule_list, 0 , sizeof(email_list_sorting_rule_t) * 2);
 *
 * sorting_rule_list[0].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD;
 * sorting_rule_list[0].sort_order                                    = EMAIL_SORT_ORDER_ASCEND;
 *
 * sorting_rule_list[1].target_attribute                              = EMAIL_MAIL_ATTRIBUTE_DATE_TIME;
 * sorting_rule_list[1].sort_order                                    = EMAIL_SORT_ORDER_ASCEND;
 *
 * err = email_get_mail_list_ex(filter_list, 9, sorting_rule_list, 2, -1, -1, &result_mail_list, &result_mail_count);
 *
 * if(err == EMAIL_ERROR_NONE) {
 * printf("email_get_mail_list_ex succeed.\n");
 *
 * for(i = 0; i < result_mail_count; i++) {
 * 	printf("mail_id [%d], subject [%s], from [%s]\n", result_mail_list[i].mail_id, result_mail_list[i].subject, result_mail_list[i].from);
 * 	}
 * }
 * else {
 * 	printf("email_get_mail_list_ex failed.\n");
 * }
 *
 * email_free_list_filter(&filter_list, 9);
 *
 * return FALSE;
 * @endcode
 */
EXPORT_API int email_get_mail_list_ex(email_list_filter_t *input_filter_list, int input_filter_count, email_list_sorting_rule_t *input_sorting_rule_list, int input_sorting_rule_count, int input_start_index, int input_limit_count, email_mail_list_item_t** output_mail_list, int *output_result_count);

/**
 * @brief Frees the allocated memory for filters.
 *
 * @since_tizen 2.3
 * @privlevel N/P
 *
 * @param[in] input_filter_list   The pointer of filter structure
 * @param[in] input_filter_count  The count of filter
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_list_filter_t
 */
EXPORT_API int email_free_list_filter(email_list_filter_t **input_filter_list, int input_filter_count);

/**
 * @brief Gets the Mail List information from the DB based on the mailbox name.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]     account_id    The account ID
 * @param[in]     mailbox_id    The mailbox ID
 * @param[in]     thread_id     The thread ID \n
 *                              It can be #EMAIL_LIST_TYPE_THREAD, #EMAIL_LIST_TYPE_NORMAL or thread ID.
 * @param[in]     start_index   The start index for LIMIT clause of SQL query
 * @param[in]     limit_count   The limit count for LIMIT clause of SQL query
 * @param[in]     sorting       The sorting type
 * @param[in/out] mail_list     The pointer to the structure #email_mail_data_t
 * @param[in/out] result_count  The number of mails returned
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_data_t
 *
 * @code
 *     #include "email-api-account.h"
 *      bool
 *      _api_sample_get_mail()
 *      {
 *      email_mail_data_t *mail_list = NULL;
 *      int mailbox_id;
 *      int result_count = 0;
 *      int account_id = 0;
 *      int err_code = EMAIL_ERROR_NONE;
 *
 *      printf("\n > Enter Mailbox id: ");
 *      scanf("%d", mailbox_id);
 *
 *      printf("\n > Enter Account_id: ");
 *      scanf("%d",&account_id);
 *
 *      if (EMAIL_ERROR_NONE != email_get_mails(account_id, mailbox_id, EMAIL_LIST_TYPE_NORMAL, 0, 100, EMAIL_SORT_DATETIME_HIGH,  &mail_list, &result_count)) {
 *          printf("email_get_mails failed \n");
 *      }
 *      else {
 *          printf("Success\n");
 *          //do something
 *          free(mail_list);
 *      }
 *
 *      }
 * @endcode
 */

EXPORT_API int email_get_mails(int account_id , int mailbox_id, int thread_id, int start_index, int limit_count, email_sort_type_t sorting, email_mail_data_t** mail_list,  int* result_count);

/**
 * @brief Gets the Mail List information from the DB based on the mailbox name.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]     account_id    The account ID
 * @param[in]     mailbox_id    The mailbox ID
 * @param[in]     thread_id     The thread ID \n
 *                              It can be #EMAIL_LIST_TYPE_THREAD, #EMAIL_LIST_TYPE_NORMAL or thread ID.
 * @param[in]     start_index   The start index for LIMIT clause of SQL query
 * @param[in]     limit_count   The limit count for LIMIT clause of SQL query
 * @param[in]     sorting       The sorting type
 * @param[in/out] mail_list     The pointer to the structure #email_mail_list_item_t
 * @param[in/out] result_count  The number of mails returned
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_list_item_t
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_get_mail()
 *      {
 *      email_mail_list_item_t *mail_list = NULL;
 *      int mailbox_id;
 *      int result_count = 0;
 *      int account_id = 0;
 *      int err_code = EMAIL_ERROR_NONE;
 *
 *      printf("\n > Enter Mailbox id: ");
 *      scanf("%d", mailbox_id);
 *
 *      printf("\n > Enter Account_id: ");
 *      scanf("%d",&account_id);
 *
 *      if (EMAIL_ERROR_NONE != email_get_mail_list(account_id, mailbox_id, EMAIL_LIST_TYPE_NORMAL, 0, 100, EMAIL_SORT_DATETIME_HIGH,  &mail_list, &result_count))
 *      {
 *          printf("email_get_mail_list_ex failed \n");
 *      }
 *      else
 *      {
 *          printf("Success\n");
 *          //do something
 *          free(mail_list);
 *      }
 *
 *      }
 * @endcode
 */
EXPORT_API int email_get_mail_list(int account_id, int mailbox_id, int thread_id, int start_index, int limit_count, email_sort_type_t sorting, email_mail_list_item_t** mail_list,  int* result_count);

/**
 * @brief Gets the Mail List information from the DB based on the mailbox name account_id and sender addresses.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]     account_id    The Account ID
 * @param[in]     mailbox_id    The Mailbox ID
 * @param[in]     addr_list     The addresses of senders \n
 *                              See #email_email_address_list_t.
 * @param[in]     start_index   The first mail index of searched mail \n
 *                              This function will return mails whose index in the result list are from @a start_index to @a start_index + @a limit_count.
 * @param[in]     limit_count   The max number of returned mails
 * @param[in]     search_type   The search type
 * @param[in]     search_value  The search value
 * @param[in]     sorting       The sorting order \n 
 *                              See #email_sort_type_t.
 * @param[in/out] mail_list     The Pointer to the structure #email_mail_list_item_t
 * @param[in/out] result_count  The number of searched mails
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_email_address_list_t, #email_sort_type_t, #email_mail_list_item_t
 */
EXPORT_API int email_get_mail_by_address(int account_id , int mailbox_id, email_email_address_list_t* addr_list,
                                         int start_index, int limit_count, int search_type, const char *search_value, email_sort_type_t sorting, email_mail_list_item_t** mail_list,  int* result_count);

/**
 * @brief Gets thread information for a specific thread from DB based on the mailbox name.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]     thread_id    The thread ID
 * @param[in/out] thread_info  The pointer to the structure #email_mail_data_t
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_data_t
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_get_thread_information()
 *      {
 *      email_mail_data_t *thread_info = NULL;
 *      int thread_id = 0;
 *      int err_code = EMAIL_ERROR_NONE;
 *
 *      printf("\n > Enter thread_id: ");
 *      scanf("%d",&thread_id);
 *
 *      if ( EMAIL_ERROR_NONE != email_get_thread_information_by_thread_id(thread_id, &thread_info))
 *      {
 *          printf("email_get_thread_information_by_thread_id failed :\n"); *
 *      }
 *      else
 *      {
 *          printf("Success\n");
 *          //do something
 *      }
 *
 *      }
 * @endcode
 */
EXPORT_API int email_get_thread_information_by_thread_id(int thread_id, email_mail_data_t** thread_info);

/**
 * @brief Gets Mail List information for a specific thread from the DB based on the mailbox name.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]     thread_id    The thread ID
 * @param[in/out] thread_info  The pointer to the structure #email_mail_list_item_t
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_mail_list_item_t
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_get_thread_information_ex()
 *      {
 *      email_mail_list_item_t *thread_info = NULL;
 *      int thread_id = 0;
 *      int err_code = EMAIL_ERROR_NONE;
 *
 *      printf("\n > Enter thread_id: ");
 *      scanf("%d",&thread_id);
 *
 *      if ( EMAIL_ERROR_NONE != email_get_thread_information_ex(thread_id, &thread_info))
 *      {
 *          printf("email_get_mail_list_of_thread failed :\n"); *
 *      }
 *      else
 *      {
 *          printf("Success\n");
 *          //do something
 *      }
 *
 *      }
 * @endcode
 */
EXPORT_API int email_get_thread_information_ex(int thread_id, email_mail_list_item_t** thread_info);

/**
 * @brief Retries to send a mail.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] mail_id         The mail ID for the mail to resend
 * @param[in] timeout_in_sec  The timeout value in seconds
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_retry_send_mail()
 *      {
 *      int mail_id = 1;
 *      int timeout_in_sec = 2;
 *
 *      if(EMAIL_ERROR_NONE == email_retry_sending_mail(mail_id,timeout_in_sec))
 *          //success
 *      else
 *          //failure
 *
 *      }
 * @endcode
 */
EXPORT_API int email_retry_sending_mail(int mail_id, int timeout_in_sec);

/**
 * @brief Creates a DB and fill it with dummy data.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_create_db_full();

/**
 * @brief Callback function for cm popup. 
 *        We set the status as EMAIL_MAIL_STATUS_SEND_CANCELED <<need to be updated>>.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] mail_id  The mail ID
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_cancel_send_mail()
 *      {
 *          int mail_id = 10;
 *
 *
 *       if(EMAIL_ERROR_NONE == email_cancel_sending_mail(mail_id,))
 *          //success
 *       else
 *          //failure
 *
 *      }
 * @endcode
 */
EXPORT_API int email_cancel_sending_mail(int mail_id) ;

/**
 * @brief Gets the total disk usage of emails in KB.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[out] total_size  The total disk usage of emails
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an  error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @code
 *     #include "email-api-account.h"
 *     bool
 *     _api_sample_get_disk_space_usage()
 *     {
 *           unsigned long total_size = 0;
 *
 *           if ( EMAIL_ERROR_NONE  != email_get_disk_space_usage(&total_size))
 *               printf("email_get_disk_space_usage failed err : %d\n",err_code);
 *           else
 *               printf("email_get_disk_space_usage SUCCESS, total disk usage in KB : %ld \n", total_size);
 *     }
 * @endcode
 */
EXPORT_API int email_get_disk_space_usage(unsigned long *total_size);

/**
 * @brief Gets the address info list.
 * @details The address info list contains From, To, CC, BCC addresses and their display name, contact ID and etc (see #email_address_info_list_t).\n
 *          Each GList (From, To, CC, BCC) is the list of #email_address_info_t data. \n
 *          "data" variable of GList structure contains #email_address_info_t data. \n
 *          To get #email_address_info_t data from GList, use type casting from GList node.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  mail_id            The mail ID to get the list
 * @param[out] address_info_list  The pointer to the structure #email_address_info_list_t \n
 *                                Memory for a new address info list will be allocated to this. 
 *                                You must call email_free_address_info_list() to free the memory allocated to this.
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_address_info_list_t, #email_address_info_t, email_free_address_info_list()
 *
 * @code
 *      #include "email-api-account.h"
 *      bool
 *      _api_sample_get_address_info_list()
 *      {
 *      email_address_info_list_t *p_address_info_list= NULL;
 *      email_address_info_t *p_address_info = NULL;
 *      int mai_id;
 *      int err_code = EMAIL_ERROR_NONE;
 *
 *      mail_id = 1;
 *
 *      if ( EMAIL_ERROR_NONE != (err_code = email_get_address_info_list(mail_id, &p_address_info_list)) )
 *      {
 *          printf("email_get_address_info_list failed :\n"); *
 *          return false;
 *      }
 *      else
 *      {
 *          printf("Success\n");
 *          //do something with p_address_info_list
 *          GList *list = p_address_info_list->from;
 *          GList *node = g_list_first(list);
 *          while ( node != NULL )
 *          {
 *              p_address_info = (email_address_info_t*)node->data;
 *              printf("%d,  %s, %s, %d\n", p_address_info->address_type, p_address_info->address, p_address_info->display_name, p_address_info->contact_id);
 *
 *              node = g_list_next(node);
 *          }
 *      }
 *
 *      // Free sender list
 *      if ( p_address_info_list )
 *      {
 *          email_free_address_info_list(&p_address_info_list);
 *      }
 *      }
 * @endcode
 */
EXPORT_API int email_get_address_info_list(int mail_id, email_address_info_list_t** address_info_list);

/**
 * @brief Frees the address info list allocated by email_get_address_info_list().
 * @details This function will free the memory which is allocated to address_info_list itself.
 *
 * @since_tizen 2.3
 * @privlevel N/P
 *
 * @param[in]  address_info_list  The pointer to the structure #email_address_info_list_t to be freed
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_address_info_list_t, email_get_address_info_list()
 */
EXPORT_API int email_free_address_info_list(email_address_info_list_t **address_info_list);

/**
 * @brief Queries the information of a meeting request.
 * @details The information of a meeting request is corresponding to only one mail. \n
 *          For this reason, the information of a meeting request can be added by using email_add_message_with_meeting_request() with the matched mail information.
 *
 * @param[in]  input_conditional_clause_string  The where clause string
 * @param[out] output_meeting_req               The pointer to the structure #email_meeting_request_t
 * @param[out] output_count                     The number of meeting request returned
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          #EMAIL_ERROR_DATA_NOT_FOUND if there is no matched mail, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_meeting_request_t
 */
EXPORT_API int email_query_meeting_request(char *input_conditional_clause_string, email_meeting_request_t **output_meeting_req, int *output_count);

/**
 * @brief Gets the information of a meeting request.
 * @details The information of a meeting request is based on the Mail ID. \n
 *          The information of the meeting request is corresponding to only one mail. \n
 *          For this reason, the meeting request information can be added by using email_add_message_with_meeting_request() with the matched mail information.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]     mail_id      The mail ID for which  meeting request details need to be fetched
 * @param[in/out] meeting_req  The pointer to the structure #email_meeting_request_t
 *
 * @return  #EMAIL_ERROR_NONE on success, 
 *          #EMAIL_ERROR_DATA_NOT_FOUND if there is no matched mail, 
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_meeting_request_t
 */
EXPORT_API int email_get_meeting_request(int mail_id, email_meeting_request_t **meeting_req);

/**
 * @brief Frees a meeting request allocated by email_get_meeting_request() or allocated to add.
 * @details This function will free the memory which is allocated to meeting_req (= *meeting_req) itself.
 *
 * @since_tizen 2.3
 * @privlevel N/P
 *
 * @param[in] email_meeting_request_t  The pointer to the structure #email_meeting_request_t to be freed
 * @param[in] count                    The number of elements in #meeting_req array (usually @c 1)
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 *
 * @see #email_meeting_request_t, email_get_meeting_request()
 */
EXPORT_API int email_free_meeting_request(email_meeting_request_t** meeting_req, int count);

/**
 * @brief Moves a thread of mails to the target mailbox.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] thread_id          The thread ID to move
 * @param[in] target_mailbox_id  The mailbox ID in which to move
 * @param[in] move_always_flag   The move always flag
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_move_thread_to_mailbox(int thread_id, int target_mailbox_id, int move_always_flag);

/**
 * @brief Deletes a thread of mails.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] thread_id           The thread ID to delete
 * @param[in] delete_always_flag  The delete always flag
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_delete_thread(int thread_id, int delete_always_flag);

/**
 * @brief Modifies seen flags of the thread.
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in] thread_id  The thread ID to modify
 * @param[in] seen_flag  The seen flag
 * @param[in] on_server  The flag to sync with server
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_modify_seen_flag_of_thread(int thread_id, int seen_flag, int on_server);

/**
 * @brief Deletes mails flagged to "delete".
 *
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/email
 *
 * @param[in]  input_mailbox_id  The mailbox ID
 * @param[in]  input_on_server   The flag to sync with server
 * @param[out] output_handle     The handle of task
 *
 * @return  #EMAIL_ERROR_NONE on success,
 *          otherwise an error code (see #EMAIL_ERROR_XXX) on failure
 */
EXPORT_API int email_expunge_mails_deleted_flagged(int input_mailbox_id, int input_on_server, int *output_handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */


#endif /* __EMAIL_API_MAIL_H__ */
