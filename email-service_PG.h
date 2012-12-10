/**
 *
 * @ingroup   SLP_PG
 * @defgroup   EMAILSVC Email Service
@{
<h1 class="pg">Introduction</h1>
	<h2 class="pg">Overview </h2>
Electronic mail, most commonly abbreviated email or e-mail, is a method of exchanging digital messages. E-mail systems are based on a store-and-forward model in which e-mail server computer systems accept, forward, deliver and store messages on behalf of users, who only need to connect to the e-mail infrastructure, typically an e-mail server, with a network-enabled device for the duration of message submission or retrieval.

	<h2 class="pg">Purpose of Programming Guide</h2>
This document is mainly aimed at the core functionality of the Email Service.  The EMail Service component is implemented by Samsung to provide EMail service to applications that make use of EMail Engine. Email Engine provides functionality for the user like composing mail, saving mail, sending mail, and creating user mailbox according to the settings. Mobile subscribers can use the Email Engine to perform storage opearations such as save, update, get, delete and transport operations such as send, download and other email operations.

This programming guide is prepared for application developers who will use the email-service. It contains: 
- List of features offered by email-service
- Information on How to use APIs provided by the email-service
- Examples 

	<h2 class="pg">Target Domain / Application</h2>
The Email Service Layer can be utilized by any component in the application layer which allow the end user to perform the email related operations such as save, send, download email message and others.

For Example, the Email Service APIs shall be invoked by 
@li Multimedia application when user opts to send media file through email 
@li Email application when user tries to send an email message 

	<h2 class="pg">Terminology & Acronyms</h2>
<table>
<tr><th>Terminology</th><th>Description</th></tr>
<tr><td>Email </td><td>Electronic mail</td></tr>
<tr><td>IMAP</td><td>Internet Message Access Protocol</td></tr>
<tr><td>SMTP</td><td>Simple mail transfer protocol for sending mails</td></tr>
<tr><td>POP3</td><td>Post office protocol for receiving mails</td></tr>
<tr><td>RFC822</td><td>Describes mail header, to address, cc, bcc etc. formats and decoding and encoding standards. </td></tr>
<tr><td>OMA </td><td>Open Moblie Alliance</td></tr>
</table> 


@}

@defgroup Email_Architecture 1. Email Service Architecture
@ingroup EMAILSVC
@{
<h1 class="pg">Email-service Architecture</h1>
	<h2 class="pg">System Architecture</h2>
@image html email_image001.png
 
	<h2 class="pg">Process Architecture</h2>
@image html email_image002.png email-service Process view

@image html email_image003.png email-service Process architecture

Whenever an application wants to use email-service, it will call APIs from Email MAPI layer. Email MAPI layer APIs will internally call APIs provided by email framework module.
@}

@defgroup Email_Feature 2. Email Service Feature
@ingroup EMAILSVC
@{
<h1 class="pg">Email-service Features</h1>
<table>
<tr><th>Feature </th><th>API Reference</th></tr>
<tr><td>Account Operation</td>
<td>
@n email_add_account()
@n email_delete_account() 
@n email_update_account() 
@n email_get_account() 
@n email_get_account_list() 
@n email_free_account() 
@n email_validate_account()
</td></tr>
 
<tr><td>mailbox Operation </td>
<td>
@n email_add_mailbox()
@n email_delete_mailbox()
@n email_update_mailbox()
@n email_get_mailbox_list()
@n email_get_mailbox_by_name()
@n email_get_child_mailbox_list()
@n email_get_mailbox_by_mailbox_type()
</td></tr>
 
<tr><td>Message Operation </td>
<td>
@n email_add_message()  
@n email_update_message()
@n email_count_mail()  
@n email_delete_mail()  
@n email_delete_all_mails_in_mailbox()
@n email_clear_mail_data()
@n email_add_attachment()
@n email_delete_attachment() 
@n email_get_info()
@n email_free_mail_info()
@n email_get_header_info()  
@n email_free_header_info()  
@n email_get_body_info()
@n email_free_body_info()
@n email_get_attachment_data() 
@n email_free_attachment_info()  
@n email_get_mail() 
@n email_modify_mail_flag()
@n email_modify_seen_flag()
@n email_modify_extra_mail_flag()
@n email_move_mail_to_mailbox()
@n email_move_all_mails_to_mailbox()
@n email_count_message_with_draft_flag()
@n email_count_message_on_sending()
@n email_get_mailbox_list()
@n email_free_mailbox()
@n email_free_mail()
@n email_get_mail_flag()
@n email_free_mail_list()
@n email_release_mail()
@n email_retry_sending_mail()
@n email_make_db_full()
@n email_get_mailbox_name_by_mail_id()
@n email_cancel_sending_mail()
@n email_count_message_all_mailboxes()
@n email_get_latest_unread_mail_id()
@n email_get_max_mail_count()
@n email_get_disk_space_usage()
</td></tr>
 
<tr><td>Network Operation </td>
<td>
@n email_send_mail()
@n email_sync_header()
@n email_download_body()
@n email_download_attachment()
@n email_cancel_job()
@n email_get_pending_job()
@n email_get_network_status()
@n email_send_report()
@n email_send_saved()
@n email_sync_imap_mailbox_list()
@n email_sync_local_activity()
</td></tr>
 
<tr><td>Rule Operation </td>
<td>
@n email_get_rule()
@n email_get_rule_list()
@n email_add_rule()
@n email_update_rule()
@n email_delete_rule()
@n email_free_rule()
</td></tr>
 
<tr><td>Control Operation </td>
<td>
@n email_init_storage()
@n email_open_db()
@n email_close_db()
@n email_service_begin()
@n email_service_end()
</td></tr>
</table>
@}



@defgroup Use_Case1_account Account Operation
@ingroup EMAIL_USECASES
@{
	<h2 class="pg">Account Operation </h2>
Account Operations are a set of operations to manage email accounts like add, update, delete or get account related details.

Structure:
email_account_t - refer to doxygen (SLP-SDK: http:/* slp-sdk.sec.samsung.net) */
<table>
<tr><td>API</td>
<td>Return Value / Exceptions</td></tr>

<tr><td>int email_add_account(email_account_t* account)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param email_account_t* account should be allocated and deallocated by Application</td></tr>

<tr><td>int email_delete_account(int account_id) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
  
<tr><td>int email_update_account(int account_id , email_account_t* new_account) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks:  - Memory for param email_account_t* new_account should be allocated and deallocated by Application</td></tr>
  
<tr><td>int email_get_account(int account_id, int pulloption, email_account_t** account) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param account will happen in email_get_account (). To free this memory, application should call email_free_account ()</td></tr>
  
<tr><td>int email_get_account_list(email_account_t** account_list, int* count) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: Memory allocation for param ccount_list will happen in email_get_account_list (). To free this memory, application should call email_free_account () </td></tr>
  
<tr><td>int email_free_account(email_account_t** account_list, int count) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
  
<tr><td>int email_validate_account(int account_id, int *handle)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
</table> 

<b>Sample Code</b>
@li Add account
@code
/* Add account */       

/*  Assign values for new account */
email_account_t *account = NULL;
 
account = malloc(sizeof(email_account_t));
memset(account, 0x00, sizeof(email_account_t));
 
account->retrieval_mode               = 1;
account->incoming_server_secure_connection                 = 1;
account->outgoing_server_type          = EMAIL_SERVER_TYPE_SMTP;
account->outgoing_server_port_number             = EMAIL_SMTP_PORT;
account->outgoing_server_need_authentication                 = 1;
account->account_name                 = strdup("gmail");
account->display_name                 = strdup("Tom");
account->user_email_address                   = strdup("tom@gmail.com");
account->reply_to_addr                = strdup("tom@gmail.com");
account->return_addr                  = strdup("tom@gmail.com");
account->incoming_server_type        = EMAIL_SERVER_TYPE_POP3;
account->incoming_server_address        = strdup("pop3.gmail.com");
account->incoming_server_port_number                     = 995;
account->incoming_server_secure_connection                 = 1;
account->retrieval_mode               = EMAIL_IMAP4_RETRIEVAL_MODE_ALL;
account->incoming_server_user_name                    = strdup("tom");
account->password                     = strdup("tioimi");
account->outgoing_server_type          = EMAIL_SERVER_TYPE_SMTP;
account->outgoing_server_address          = strdup("smtp.gmail.com");
account->outgoing_server_port_number             = 587;
account->outgoing_server_secure_connection             = 0x02;
account->outgoing_server_need_authentication                 = 1;
account->outgoing_server_user_name                 = strdup("tom@gmail.com");
account->sending_password             = strdup("tioimi");
account->pop_before_smtp              = 0;
account->incoming_server_requires_apop                         = 0;
account->flag1                        = 2;
account->flag2                        = 1;
account->is_preset_account            = 1;
account->logo_icon_path               = strdup("Logo Icon Path");
account->options.priority             = 3;
account->options.keep_local_copy      = 0;
account->options.req_delivery_receipt = 0;   
account->options.req_read_receipt     = 0;
account->options.download_limit       = 0;
account->options.block_address        = 0;
account->options.block_subject        = 0;
account->options.display_name_from    = strdup("Display name from");
account->options.reply_with_body      = 0;
account->options.forward_with_files   = 0;
account->options.add_myname_card      = 0;
account->options.add_signature        = 0;
account->options.signature            = strdup("Signature");
account->check_interval               = 0;
      
if(EMAIL_ERROR_NONE != email_add_account(account))
	/* failure */
else
{
	/* success     */
	if(account_id)
		*account_id = account->account_id;		/*  Use this returned account id when calling APIs which need it */
}

/* free account */
email_free_account(&account, 1);
 
@endcode 

@li Get account
@code
/* Get account */
 
email_account_t *account = NULL;
int account_id = 1;		/*  account id to be gotten */
 
if(EMAIL_ERROR_NONE != email_get_account(account_id,GET_FULL_DATA,&account))
	/* failure */
else
	/* success */
 
/* free account */
email_free_account(&account, 1);
 
@endcode

@li Update account
@code
/* Update account */               

email_account_t *new_account = NULL;
int account_id = 1;		/*  account id to be updated */
 
/*  Get account to be updated */
if(EMAIL_ERROR_NONE != email_get_account(account_id,GET_FULL_DATA,&new_account))
	/* failure */
else
	/* success */
 
/*  Set the new values */
new_account->flag1                   = 1;
new_account->account_name            = strdup("gmail");
new_account->display_name            = strdup("Tom001");
new_account->options.keep_local_copy = 1;   
new_account->check_interval          = 55;
 
if(EMAIL_ERROR_NONE != email_update_account(account_id,new_account))
	/* failure */
else
	/* success */
 
/* free account */
email_free_account(&new_account, 1);

@endcode

@li Delete account
@code 
/* Delete account */

int account_id = 1;		/*  account id to be deleted */
 
if(EMAIL_ERROR_NONE != email_delete_account(account_id))
      /* failure */
else
      /* success */
 
@endcode


@li Get list of accounts
@code
/* Get list of accounts */

email_account_t *account_list = NULL;
int count = 0;		
int i;
 
if(EMAIL_ERROR_NONE != email_get_account_list(&account_list,&count))
	/* failure */
else
{
	/* success */
	for ( i = 0; i < count; i++ )
	{
		/*  Do something with each account */
		printf("account id : %d\n", account_list[i].account_id);
	}
}	
 
/* free account */
email_free_account(&account_list,count);

@endcode

@li Validate account - try to connect to server
@code
/* Validate account - try to connect to server */
unsigned account_handle = 0;
int account_id = 1;
 
if(EMAIL_ERROR_NONE != email_validate_account(account_id,&account_handle))
	/* failure */
else
	/* success */
@endcode
 
<b>Flow Diagram</b>
@image html email_image004.png
@}

@defgroup Use_Case2_folder mailbox Operation
@ingroup EMAIL_USECASES
@{
	<h2 class="pg">mailbox Operation </h2>
mailbox Operations are a set of operations to manage email mailboxes like add, update, delete or get mailbox related details.

Structure:
email_mailbox_t - refer to doxygen (SLP-SDK: http:/* slp-sdk.sec.samsung.net) */

<table>
<tr><td>API</td><td>Return Value / Exceptions</td></tr>
 
<tr><td>int email_add_mailbox(email_mailbox_t* new_mailbox, int on_server, int *handle) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: -  Memory for param email_mailbox_t* new_mailbox should be allocated and deallocated by Application </td></tr>
 
<tr><td>int email_delete_mailbox(email_mailbox_t* mailbox, int on_server,  int *handle) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param email_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>

<tr><td>int email_update_mailbox(email_mailbox_t*old_mailbox, email_mailbox_t* new_mailbox)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for params email_mailbox_t* old_mailbox and  email_mailbox_t* new_mailbox should be allocated and deallocated by Application</td></tr>

<tr><td>int email_get_mailbox_list(int account_id, int local_yn, email_mailbox_t** mailbox_list, int* count)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param mailbox_list will happen in email_get_mailbox_list (). To free this memory application should call email_free_mailbox</td></tr>
 
<tr><td>int email_get_mailbox_by_name(int account_id, const char *pMailboxName, email_mailbox_t **pMailbox)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param pMailbox will happen in email_get_mailbox_by_name (). To free this memory application should call email_free_mailbox</td></tr>
 
<tr><td>int email_get_child_mailbox_list(int account_id, const char *parent_mailbox,  email_mailbox_t** mailbox_list, int* count)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param mailbox_list will happen in email_get_child_mailbox_list (). To free this memory application should call email_free_mailbox</td></tr>
 
<tr><td>int email_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type,  email_mailbox_t** mailbox)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param mailbox_list will happen in email_get_mailbox_by_mailbox_type (). To free this memory application should call email_free_mailbox</td></tr>
</table>
 
<b>Sample Code</b>

@li Create new mailbox
@code
email_mailbox_t *mailbox = NULL, *new_mailbox = NULL;
int handle = 0;
int on_server = 0;
 
mailbox = malloc(sizeof(email_mailbox_t));
memset(mailbox, 0x00, sizeof(email_mailbox_t));
 
mailbox->mailbox_name           = strdup("Personal"); 
mailbox->alias          = strdup("selfuse");            
mailbox->account_id     = 1; 
mailbox->local          = on_server; 
mailbox->mailbox_type    = 7;  
      
/* create new mailbox */           
if(EMAIL_ERROR_NONE != email_add_mailbox(mailbox,on_server,&handle))
      /* failure */
else
      /* success   */

@endcode


@li Update and Delete mailbox
@code
email_mailbox_t *mailbox = NULL, *new_mailbox = NULL;
int on_server = 0;
int handle = 0;

new_mailbox = malloc(sizeof(email_mailbox_t));
memset(new_mailbox, 0x00, sizeof(email_mailbox_t));
 
new_mailbox->mailbox_name =  strdup("Personal001");
 
/* update mailbox */
if(EMAIL_ERROR_NONE != email_update_mailbox(mailbox,new_mailbox))
      /* failure */
else
      /* success   */
 
/* delete mailbox */
if(EMAIL_ERROR_NONE != email_delete_mailbox(mailbox,on_server,&handle))
      /* failure */
else
      /* success   */
 
email_free_mailbox(&mailbox, 1);
email_free_mailbox(&new_mailbox, 1);
 
@endcode

@li Get list of mailboxes
@code
int account_id = 1;
int local_yn = 0;
email_mailbox_t* mailbox_list = NULL;
int count = 0;
 
/*get list of mailboxes */
if(EMAIL_ERROR_NONE != email_get_mailbox_list(account_id, local_yn, &mailbox_list, &count))
      /* failure */
else
{
      /* success   */
      email_free_mailbox(&mailbox_list,count);
}
@endcode

<b>Flow Diagram</b>
@image html email_image005.png
@}

@defgroup Use_Case3_message Message Operation
@ingroup EMAIL_USECASES
@{
	<h2 class="pg">Message Operation </h2>
Message Operations are a set of operations to manage email messages like add, update, delete or get message related details.

Structure:
email_mail_data_t
<table>
<tr><td>API</td>
<td>Return Value / Exceptions</td></tr>
 
<tr><td>int email_add_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: -  Memory for params email_mail_data_t* input_mail_data and email_attachment_data_t *input_attachment_data_list and email_meeting_request_t* input_meeting_request should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_update_mail(email_mail_data_t *input_mail_data, email_attachment_data_t *input_attachment_data_list, int input_attachment_count, email_meeting_request_t* input_meeting_request, int input_from_eas)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: -  Memory for params email_mail_data_t* input_mail_data and email_attachment_data_t *input_attachment_data_list and email_meeting_request_t* input_meeting_request should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_count_mail(email_mailbox_t* mailbox, int* total, int* unseen)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param email_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>

<tr><td>int email_delete_mail(email_mailbox_t* mailbox, int *mail_ids, int num, int from_server)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for params int *mail_ids and mf_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_delete_all_mails_in_mailbox(email_mailbox_t* mailbox, int from_server)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param mf_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_clear_mail_data()  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>

<tr><td>int email_add_attachment(email_mailbox_t* mailbox, int mail_id, email_attachment_data_t* attachment)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param mf_mailbox_t* mailbox and mf_attachment_info_t* attachment hould be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_delete_attachment(email_mailbox_t * mailbox, int mail_id, const char * attachment_id)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param email_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_get_attachment_data(email_mailbox_t* mailbox, int mail_id, const char* attachment_id, email_attachment_data_t** attachment)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure.@n Remarks:
-# Memory for param email_mailbox_t* mailbox should be allocated and deallocated by Application 
-# Memory allocation for param email_attachment_data_t** attachment will happen in email_get_attachment_data (). To free this memory, application should call email_free_attachment_info () </td></tr>
 
<tr><td>int email_free_attachment_info(email_attachment_data_t** atch_info)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_modify_seen_flag(int *mail_ids, int num, int seen_flag, int onserver)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param int *mail_ids should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_move_mail_to_mailbox(int *mail_ids, int num, email_mailbox_t* new_mailbox)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for params int *mail_ids and email_mailbox_t* new_mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_move_all_mails_to_mailbox(email_mailbox_t* src_mailbox, email_mailbox_t* new_mailbox)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for params email_mailbox_t* src_mailbox and email_mailbox_t* new_mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_count_message_with_draft_flag(email_mailbox_t* mailbox, int* total)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param email_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_count_message_on_sending(email_mailbox_t* mailbox, int* total)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param email_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>
  
<tr><td>int email_get_mailbox_list(int account_id, email_mailbox_t** mailbox_list, int* count ) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param email_mailbox_t** mailbox_list will happen in email_get_mailbox_list (). To free this memory, application should call email_free_mailbox ()</td></tr>
 
<tr><td>int email_free_mailbox(email_mailbox_t** mailbox_list, int count)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_retry_sending_mail( int mail_id, int timeout_in_sec)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_make_db_full()</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_get_mailbox_name_by_mail_id(int mail_id, char **pMailbox_name)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_cancel_sending_mail( int mail_id)  </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_count_message_all_mailboxes(email_mailbox_t* mailbox, int* total, int* unseen) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_get_latest_unread_mail_id(int account_id, int *pMailID) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_get_max_mail_count(int *Count) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_get_disk_space_usage(unsigned long *total_size)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
</table>
 
<b>Sample Code</b>

@li Add, Update, Count and Delete message
@code
email_mailbox_t *mailbox = NULL;
int on_server = 0, account_id = 0, mail_id = 0;
char *pFilePath = "/tmp/mail.txt";
int                    i = 0;
int                    account_id = 0;
int                    from_eas = 0;
int                    attachment_count = 0;
int                    err = EMAIL_ERROR_NONE;
char                   arg[50] = { 0 , };
char                  *body_file_path = MAILHOME"/tmp/mail.txt";
email_mailbox_t         *mailbox_data = NULL;
email_mail_data_t       *test_mail_data = NULL;
email_attachment_data_t *attachment_data = NULL;
email_meeting_request_t *meeting_req = NULL;
FILE                  *body_file;

printf("\n > Enter account id : ");
scanf("%d", &account_id);

memset(arg, 0x00, 50);
printf("\n > Enter mailbox name : ");
scanf("%s", arg);

email_get_mailbox_by_name(account_id, arg, &mailbox_data);

test_mail_data = malloc(sizeof(email_mail_data_t));
memset(test_mail_data, 0x00, sizeof(email_mail_data_t));

printf("\n From EAS? [0/1]> ");
scanf("%d", &from_eas);

test_mail_data->account_id        = account_id;
test_mail_data->save_status       = 1;
test_mail_data->flags_seen_field  = 1;
test_mail_data->file_path_plain   = strdup(body_file_path);
test_mail_data->mailbox_name      = strdup(mailbox_data->mailbox_name);
test_mail_data->mailbox_type      = mailbox_data->mailbox_type;
test_mail_data->full_address_from = strdup("<test1@test.com>");
test_mail_data->full_address_to   = strdup("<test2@test.com>");
test_mail_data->full_address_cc   = strdup("<test3@test.com>");
test_mail_data->full_address_bcc  = strdup("<test4@test.com>");
test_mail_data->subject           = strdup("Meeting request mail");

body_file = fopen(body_file_path, "w");

for(i = 0; i < 500; i++)
	fprintf(body_file, "X2 X2 X2 X2 X2 X2 X2");
fflush(body_file);
 fclose(body_file);


if((err = email_add_mail(test_mail_data, attachment_data, attachment_count, meeting_req, from_eas)) != EMAIL_ERROR_NONE)
	printf("email_add_mail failed. [%d]\n", err);
else
	printf("email_add_mail success.\n");
 
/* Update message */
/*  variable 'mail' should be filled with data on DB. */
/*  And change values you want to update. */
mail->head->subject = strdup("save.mailbox again...");
 
if(EMAIL_ERROR_NONE != email_update_message(mail_id,mail))
	/* failure */
else
	/* success   */
 
/* Count message */
int total = 0, unseen = 0;
 
/*  Get the total number of mails and the number of unseen mails */
if(EMAIL_ERROR_NONE != email_count_mail(mailbox,&total,&unseen))
	/* failure */
else
	/* success   */
 
/* Delete message */
int *mail_ids, num = 0;
 
if(EMAIL_ERROR_NONE != email_delete_mail(mailbox,mail_ids,num,on_server))
	/* failure */
else
	/* success   */
@endcode 
 
@li Delete all message in a specific mailbox
@code 
/* Delete all message in mailbox */
email_mailbox_t *mailbox = NULL;
int on_server = 0;

mailbox = malloc(sizeof(email_mailbox_t));
memset(mailbox, 0x00, sizeof(email_mailbox_t));

mailbox->account_id = 1;
mailbox->mailbox_name = strdup("INBOX");		

if( EMAIL_ERROR_NONE != email_delete_all_mails_in_mailbox(mailbox, on_server))
      /* failure */
else

      /* success   */
@endcode


@li Clear all messages
@code
/* clear mail data */
if(EMAIL_ERROR_NONE !=  email_clear_mail_data())
      /* failure */
else
      /* success   */
@endcode


@li Move mail
@code 
int mail_id[],account_id = 1;
email_mailbox_t *mailbox = NULL;
char *mailbox_name = "INBOX";
 
mailbox = malloc(sizeof(email_mailbox_t));
memset(mailbox, 0x00, sizeof(email_mailbox_t));
 
mailbox->account_id = account_id;
mailbox->mailbox_name = mailbox_name;
 
/* Move mail to given mailbox*/
if(EMAIL_ERROR_NONE !=  email_move_mail_to_mailbox(/*mail_id*/,/*num*/,mailbox))
      /* failure */
else
      /* success   */
 
/* free mailbox*/
email_free_mailbox(&mailbox,1);
 
email_mailbox_t *src_mailbox = NULL,*dest_mailbox = NULL;
int src_account_id = 0, dest_account_id = 0;
char * src_mailbox_name = NULL, *dest_mailbox_name = NULL;
 
src_mailbox = malloc(sizeof(email_mailbox_t));
memset(src_mailbox, 0x00, sizeof(email_mailbox_t));
 
dest_mailbox = malloc(sizeof(email_mailbox_t));
memset(dest_mailbox, 0x00, sizeof(email_mailbox_t));
 
src_mailbox->account_id = /*src_account_id*/;
src_mailbox->mailbox_name = /*src_mailbox_name*/
 
dest_mailbox->account_id = /*dest_account_id*/;
dest_mailbox->mailbox_name = /*dest_mailbox_name*/
 
/*move all mails to given mailbox*/
if(EMAIL_ERROR_NONE !=  email_move_all_mails_to_mailbox(src_mailbox,dest_mailbox))
      /* failure */
else
      /* success   */
 
/* free mailbox*/
email_free_mailbox(&src_mailbox,1);
email_free_mailbox(&dest_mailbox,1);
 
int account_id = 0, total = 0;
email_mailbox_t *mailbox = NULL;
char *mailbox_name = NULL;
 
mailbox = malloc(sizeof(email_mailbox_t));
memset(mailbox, 0x00, sizeof(email_mailbox_t));
 
mailbox->account_id = /*account_id*/;
mailbox->mailbox_name = /*mailbox_name*/
 
/*count of draft msgs*/
if(EMAIL_ERROR_NONE !=  email_count_message_with_draft_flag(mailbox,&total))
      /* failure */
else
      /* success   */
 
/* free mailbox*/
email_free_mailbox(&mailbox,1);
@endcode
 
@li Count of msgs sent from given folde
@code
int account_id = 0, total = 0;
email_mailbox_t *mailbox = NULL;
char *mailbox_name = NULL;
 
mailbox = malloc(sizeof(email_mailbox_t));
memset(mailbox, 0x00, sizeof(email_mailbox_t));
 
mailbox->account_id = /*account_id*/;
mailbox->mailbox_name = /*mailbox_name*/
 
/*count of msgs sent from given mailbox*/
if(EMAIL_ERROR_NONE != email_count_message_on_sending(mailbox,&total))
      /* failure */
else
      /* success   */
 
/* free mailbox*/
email_free_mailbox(&mailbox,1);

@endcode


@li Get mailbox list 
@code
 
email_mailbox_t* mailbox_list = NULL;
int account_id = 1, count = 0;
 
/* Get mailbox list*/
if(EMAIL_ERROR_NONE != email_get_mailbox_list(account_id,&mailbox_list,&count))
      /* failure */
else
      /* success   */
 
/* free mailbox list*/
email_free_mailbox(&mailbox,count);
@endcode


@li Get mailBox name by mailID
@code
/* Get mailBox name by mailID*/
int mail_id = 1;
char *pMailbox_name=strdup("INBOX");
err = email_get_mailbox_name_by_mail_id(mail_id,&pMailbox_name);

free(pMailbox_name);

@endcode


@li Cancel sending mail
@code 
 
/* email_cancel_sending_mail*/
int mail_id = 1;	/*  mail id of a mail which is on sending */
err = email_cancel_sending_mail(mail_id);
 
@endcode



@li Get the Total count and Unread count of all mailboxes
@code 
/* Get the Total count and Unread count of all mailboxes */
email_mailbox_t* mailbox = NULL;
int account_id = 1, total = 0, unseen = 0;
char *mailbox_name = NULL;
 
mailbox = malloc(sizeof(email_mailbox_t));
memset(mailbox, 0x00, sizeof(email_mailbox_t));
 
mailbox->account_id = /*account_id*/;
mailbox->mailbox_name = /*mailbox_name*/
 
err = email_count_message_all_mailboxes(mailbox,&total,&unseen);
 
@endcode

<b>Flow Diagram</b>s
@image html email_image006.png

@image html email_image007.png
 
@image html email_image008.png 
@}

@defgroup Use_Case4_network Network Operation
@ingroup EMAIL_USECASES
@{
	<h2 class="pg">Network Operation </h2>
Network Operations are a set of operations to manage email send, receive and cancel related details.

Structure:
email_option_t - refer to doxygen (SLP-SDK: http:/* slp-sdk.sec.samsung.net) */
<table>
<tr><td>API</td>
<td>Return Value / Exceptions</td></tr>
 
<tr><td>int email_send_mail( email_mailbox_t* mailbox, int mail_id, int *handle)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: 
-# Memory allocation and de-allocation for input param is to be done by application.</td></tr>
 
<tr><td>int email_sync_header(email_mailbox_t* mailbox, int *handle)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation for input param is to be done by application.</td></tr>
 
<tr><td>int email_download_body(email_mailbox_t* mailbox, int mail_id, int with_attachment, int *handle) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation for input param is to be done by application.</td></tr>
 
<tr><td>int email_download_attachment(email_mailbox_t* mailbox, int mail_id, const char* nth,  int *handle)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure.</td></tr>

<tr><td>int email_cancel_job(int account_id, int handle)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_get_pending_job(email_action_t action, int account_id, int mail_id, email_event_status_type_t * status)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>void email_get_network_status(int* on_sending, int* on_receiving) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_send_saved(int account_id, email_option_t* sending_option, int *handle)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation for input param is to be done by application.</td></tr>
 
<tr><td>int email_sync_imap_mailbox_list(int account_id, const char* mailbox, int *handle)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_sync_local_activity(int account_id)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure.</td></tr>
</table>

<b>Sample Code</b>
@li Send a mail
@code
/* TODO : Write sample to send a mail. */
@endcode

@li Download header of new emails from mail server
@code
/* Download header of new emails from mail server*/
email_mailbox_t mailbox;
int account_id = 1;
int err = EMAIL_ERROR_NONE;
int handle = 0;
 
memset(&mailbox, 0x00, sizeof(email_mailbox_t));
 
mailbox.account_id = account_id;
mailbox.mailbox_name = strdup("INBOX");
err = email_sync_header (&mailbox,&handle);
@endcode


@li Download email body from server
@code
 
/*Download email body from server*/
email_mailbox_t mailbox;
int mail_id = 1;
int account_id = 1;
int handle = 0;
int err = EMAIL_ERROR_NONE;
 
memset(&mailbox, 0x00, sizeof(email_mailbox_t));
mailbox.account_id = account_id;
mailbox.mailbox_name = strdup("INBOX");
err= email_download_body (&mailbox,mail_id,0,&handle);
 
@li Download a email nth-attachment from server
@code
/*Download a email nth-attachment from server*/
email_mailbox_t mailbox;
int mail_id = 1;
int account_id = 1;     
char arg[50]; /* Input attachment number need to be download */
int handle = 0;
int err = EMAIL_ERROR_NONE;
 
memset(&mailbox, 0x00, sizeof(email_mailbox_t));
mailbox.mailbox_name = "INBOX";
mailbox.account_id = account_id;
err=email_download_attachment(&mailbox,mail_id,arg,&handle);
@endcode

<b>Flow Diagram</b>
@image html email_image009.png
@}

@defgroup Use_Case5_rule Rule Operation
@ingroup EMAIL_USECASES
@{
	<h2 class="pg">Rule Operation</h2>
Rule Operations are a set of operations to manage email rules like add, get, delete or update rule related details.

Structure:
email_rule_t- refer to doxygen (SLP-SDK: http:/* slp-sdk.sec.samsung.net) */
<table>
<tr><td>API</td>
<td>Return Value / Exceptions</td></tr>
 
<tr><td>int email_get_rule(int filter_id, email_rule_t** filtering_set)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks: 
-# Memory allocation for the param email_rule_t** filtering_set will be done in this api.
-# De-allocation is to be done by application.</td></tr>
 
<tr><td>int email_get_rule_list(email_rule_t** filtering_set, int* count)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation for the param email_rule_t** filtering_set will be done in this api.
-# De-allocation is to be done by application.</td></tr>
 
<tr><td>int email_add_rule(email_rule_t* filtering_set) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation is to be done by application.
-# Use email_free_rule to free allocated memory.</td></tr>
 
<tr><td>int email_update_rule(int filter_id, email_rule_t* new_set) </td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation is to be done by application.</td></tr>
-# Use email_free_rule to free allocated memory.
 
<tr><td>int email_delete_rule(int filter_id)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_free_rule(email_rule_t** filtering_set, int count)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
</table>

<b>Sample Code</b>
@li Filter Operation
@code
int err = EMAIL_ERROR_NONE;
email_rule_t*  rule = NULL;
int filter_id = 1;
 
/* Get a information of filtering*/
err = email_get_rule (filter_id,&rule);
err = email_free_rule (&rule,1);
 
/* Get all filterings */
int count = 0;
err = email_get_rule_list(&rule,&count);
 
 
/* Add a filter information */
err = email_add_rule (rule);
err = email_free_rule (&rule,1);
 
/* Change a filter information */
err = email_update_rule (filter_id,rule);
err = email_free_rule (&rule,1);
 
/* Delete a filter information*/
err = email_delete_rule (filter_id);
 
/* Free allocated memory */
err = email_free_rule (&rule,1);
@endcode

<b>Flow Diagram</b>
@image html email_image010.png
@}

@defgroup Use_Case6_control Control Operation
@ingroup EMAIL_USECASES
@{
	<h2 class="pg">Control Operation</h2>
Control Operations are a set of operations to manage Email MAPI Layer  initialization.
The Application which will use the MAPIs MUST initialize IPC proxy and conntect to Email FW database before calling other APIs.
And it MUST finalize IPC proxy and disconnect to the DB if the application doesn't use APIs.

<table>
<tr><td>API</td>
<td>Return Value / Exceptions</td></tr>
 
<tr><td>int email_init_storage(void)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_open_db(void)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure
@n Remarks:
@n Application should call email_close_db once db operation is over</td></tr>
 
<tr><td>int email_close_db(void)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure
@n Remarks: - 
@n This API should be called only if email_open_db () is called.</td></tr>
 
<tr><td>int email_service_begin(void)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_service_end(void)</td>
<td>Returns EMAIL_ERROR_NONE on success or negative value on failure
@n Remarks:
@n This API should be called only if email_service_begin () is called.</td></tr>
</table>
 
<b>Sample Code</b>
@li Initialize and Finalize Email MAPI Layer
@code
int err = EMAIL_ERROR_NONE;
 
/*  Initialize Email MAPI Layer before calling other MAPIs */
if(EMAIL_ERROR_NONE == email_service_begin())
{
	if(EMAIL_ERROR_NONE != email_open_db())
	{
		return false;
	}
	if(EMAIL_ERROR_NONE != email_init_storage())      
	{
		return false;
	}
}

/*  Call other MAPIs */

......

/*  Finalize Email MAPI Layer when finishing application */
err = email_close_db();
err = email_service_end();
@endcode

<b>Flow Diagram</b>
@image html email_image011.png
@}

@addtogroup Email_Feature
@{
<h1 class="pg">System Configuration</h1>
	<h2 class="pg">Files to be included</h2>
email-api.h
@n email-types.h

	<h2 class="pg">System Initialization and De-Initialization</h2>
email_service_begin is used to initialize email-service at boot time.
@n email_service_end is used to deinitialize email-service at shutdown. 
@n These two are separate executables.

	<h2 class="pg">Variable Configuration</h2>
NA

	<h2 class="pg">Build Environment</h2>
If the Application wants to use email-service Module, make sure that the following package should be included in the Makefile.

email-service-dev

	<h2 class="pg">Runtime Environment</h2>
NA
@}

@defgroup EMAL_Appendix 4. Reference
@ingroup EMAILSVC
@{
<h1 class="pg">Appendix</h1>
	<h2 class="pg">Email</h2>
@image html email_image012.png

- Alice composed a message using MUA (Mail User Agent). Alice enters the e-mail address of her correspondent, and hits the "send" button.
- MUA format the message using MIME and uses Simple mail Transfer Protocol to send the message to local MTA (Mail Transfer Agent) i,e smtp.a.org run by Alices ISP (Internet Service provider).
- The MTA looks at the destination address provided in the SMTP protocol i,e bob@b.org. An Internet e-mail address is a string of the form localpart@exampledomain. The part before the @ sign is the local part of the address, often the username of the recipient, and the part after the @ sign is a domain name. The MTA resolves a domain name to determine the fully qualified domain name of the mail exchange server in the Domain Name System (DNS).
- The DNS Server for the b.org domain, ns.b.org, responds with an MX Records listing the mail exchange servers for that domain, in this case mx.b.org, a server run by Bob's ISP. 
- smtp.a.org sends the message to mx.b.org using SMTP, which delivers it to the mailbox of the user bob. 
- Bob presses the "get mail" button in his MUA, which picks up the message using the Post Office Protocol (POP3).

	<h2 class="pg">RFC</h2>
-# RFC 2821-SMTP(Simple Mail Transfer Protocol)
-# RFC 1939-POP3(Post Office Protocol)
-# RFC 3501-IMAP4(Internate message protocol)
@}

*/

/**
* @defgroup  EMAILSVC
 @{
* 	@defgroup EMAIL_USECASES 3. API Description
*
@{
<h1 class="pg">API Description</h1>
This section describes APIs and shows the example of using them.
@}
 @}
*/

