/**
 *
 * @ingroup   TIZEN_PG
 * @defgroup   EMAILSVC Email Service
@{
<h1 class="pg">Introduction</h1>
	<h2 class="pg">Overview </h2>
Electronic mail, most commonly abbreviated email or e-mail, is a method of exchanging digital messages. E-mail systems are based on a store-and-forward model in which e-mail server computer systems accept, forward, deliver and store messages on behalf of users, who only need to connect to the e-mail infrastructure, typically an e-mail server, with a network-enabled device for the duration of message submission or retrieval.

	<h2 class="pg">Purpose of Programming Guide</h2>
This document is mainly aimed at the core functionality of the Email Service.  The EMail Service component is implemented to provide EMail service to applications that make use of EMail Engine. Email Engine provides functionality for the user like composing mail, saving mail, sending mail, and creating user mailbox according to the settings. Mobile subscribers can use the Email Engine to perform storage opearations such as save, update, get, delete and transport operations such as send, download and other email operations.

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
@n email_count_message()  
@n email_delete_message()  
@n email_delete_all_message_in_mailbox()
@n email_clear_mail_data()
@n email_add_attachment()
@n email_delete_attachment() 
@n email_get_info()
@n email_free_mail_info()
@n email_get_header_info()  
@n email_free_header_info()  
@n email_get_body_info()
@n email_free_body_info()
@n email_get_attachment_info() 
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
@n email_retry_send_mail()
@n email_create_db_full()
@n email_get_mailbox_name_by_mail_id()
@n email_cancel_send_mail()
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
@n email_get_imap_mailbox_list()
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
emf_account_t 
<table>
<tr><td>API</td>
<td>Return Value / Exceptions</td></tr>

<tr><td>int email_add_account(emf_account_t* account)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param emf_account_t* account should be allocated and deallocated by Application</td></tr>

<tr><td>int email_delete_account(int account_id) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
  
<tr><td>int email_update_account(int account_id , emf_account_t* new_account) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks:  - Memory for param emf_account_t* new_account should be allocated and deallocated by Application</td></tr>
  
<tr><td>int email_get_account(int account_id, int pulloption, emf_account_t** account) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param account will happen in email_get_account (). To free this memory, application should call email_free_account ()</td></tr>
  
<tr><td>int email_get_account_list(emf_account_t** account_list, int* count) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: Memory allocation for param ccount_list will happen in email_get_account_list (). To free this memory, application should call email_free_account () </td></tr>
  
<tr><td>int email_free_account(emf_account_t** account_list, int count) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
  
<tr><td>int email_validate_account(int account_id, unsigned* handle)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
</table> 

<b>Sample Code</b>
@li Add account
@code
/* Add account */       

/*  Assign values for new account */
emf_account_t *account = NULL;
 
account = malloc(sizeof(emf_account_t));
memset(account, 0x00, sizeof(emf_account_t));
 
account->account_bind_type      = 1;                           
account->retrieval_mode         = 1;                                                   
account->use_security           = 1;                                                   
account->sending_server_type    = EMF_SERVER_TYPE_SMTP; 
account->sending_port_num       = EMF_SMTP_PORT;                          
account->sending_auth           = 1;   
account->flag1 = 2;
account->account_bind_type      = 1;                           
account->account_name           = strdup("gmail"); 
account->display_name           = strdup("Tom"); 
account->email_addr             = strdup("tom@gmail.com"); 
account->reply_to_addr          = strdup("tom@gmail.com"); 
account->return_addr            = strdup("tom@gmail.com"); 
account->receiving_server_type  = EMF_SERVER_TYPE_POP3; 
account->receiving_server_addr  = strdup("pop3.gmail.com"); 
account->port_num               = 995;        
account->use_security           = 1;                                                   
account->retrieval_mode         = EMF_IMAP4_RETRIEVAL_MODE_ALL;
account->user_name              = strdup("tom"); 
account->password               = strdup("tioimi"); 
account->sending_server_type    = EMF_SERVER_TYPE_SMTP;                               
account->sending_server_addr    = strdup("smtp.gmail.com"); 
account->sending_port_num       = 587;                                                        
account->sending_security       = 0x02;  
account->sending_auth           = 1;   
account->sending_user           = strdup("tom@gmail.com"); 
account->sending_password       = strdup("tioimi");
account->pop_before_smtp        = 0;
account->apop                   = 0;
account->flag1                  = 2;                
account->flag2                  = 1; 
account->preset_account         = 1;       
account->logo_icon_path         = strdup("Logo Icon Path"); 
account->target_storage         = 0;                            
account->options.priority = 3;   
account->options.keep_local_copy = 0;   
account->options.req_delivery_receipt = 0;   
account->options.req_read_receipt = 0;   
account->options.download_limit = 0;   
account->options.block_address = 0;   
account->options.block_subject = 0;   
account->options.display_name_from = strdup("Display name from"); 
account->options.reply_with_body = 0;   
account->options.forward_with_files = 0;   
account->options.add_myname_card = 0;   
account->options.add_signature = 0;   
account->options.signature= strdup("Signature");    
account->check_interval = 0;  
      
if(EMF_ERROR_NONE != email_add_account(account))
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
 
emf_account_t *account = NULL;
int account_id = 1;		/*  account id to be gotten */
 
if(EMF_ERROR_NONE != email_get_account(account_id,GET_FULL_DATA,&account))
	/* failure */
else
	/* success */
 
/* free account */
email_free_account(&account, 1);
 
@endcode

@li Update account
@code
/* Update account */               

emf_account_t *new_account = NULL;
int account_id = 1;		/*  account id to be updated */
 
/*  Get account to be updated */
if(EMF_ERROR_NONE != email_get_account(account_id,GET_FULL_DATA,&new_account))
	/* failure */
else
	/* success */
 
/*  Set the new values */
new_account->flag1 = 1;
new_account->account_name           = strdup("gmail"); 
new_account->display_name           = strdup("Tom001");             
new_account->options.keep_local_copy = 1;   
new_account->check_interval = 55;  
 
if(EMF_ERROR_NONE != email_update_account(account_id,new_account))
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
 
if(EMF_ERROR_NONE != email_delete_account(account_id))
      /* failure */
else
      /* success */
 
@endcode


@li Get list of accounts
@code
/* Get list of accounts */

emf_account_t *account_list = NULL;
int count = 0;		
int i;
 
if(EMF_ERROR_NONE != email_get_account_list(&account_list,&count))
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
 
if(EMF_ERROR_NONE != email_validate_account(account_id,&account_handle))
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
emf_mailbox_t 

<table>
<tr><td>API</td><td>Return Value / Exceptions</td></tr>
 
<tr><td>int email_add_mailbox(emf_mailbox_t* new_mailbox, int on_server, unsigned* handle) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: -  Memory for param emf_mailbox_t* new_mailbox should be allocated and deallocated by Application </td></tr>
 
<tr><td>int email_delete_mailbox(emf_mailbox_t* mailbox, int on_server,  unsigned* handle) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param emf_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>

<tr><td>int email_update_mailbox(emf_mailbox_t*old_mailbox, emf_mailbox_t* new_mailbox)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for params emf_mailbox_t* old_mailbox and  emf_mailbox_t* new_mailbox should be allocated and deallocated by Application</td></tr>

<tr><td>int email_get_mailbox_list(int account_id, int local_yn, emf_mailbox_t** mailbox_list, int* count)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param mailbox_list will happen in email_get_mailbox_list (). To free this memory application should call email_free_mailbox</td></tr>
 
<tr><td>int email_get_mailbox_by_name(int account_id, const char *pMailboxName, emf_mailbox_t **pMailbox)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param pMailbox will happen in email_get_mailbox_by_name (). To free this memory application should call email_free_mailbox</td></tr>
 
<tr><td>int email_get_child_mailbox_list(int account_id, const char *parent_mailbox,  emf_mailbox_t** mailbox_list, int* count)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param mailbox_list will happen in email_get_child_mailbox_list (). To free this memory application should call email_free_mailbox</td></tr>
 
<tr><td>int email_get_mailbox_by_mailbox_type(int account_id, emf_mailbox_type_e mailbox_type,  emf_mailbox_t** mailbox)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param mailbox_list will happen in email_get_mailbox_by_mailbox_type (). To free this memory application should call email_free_mailbox</td></tr>
</table>
 
<b>Sample Code</b>

@li Create new mailbox
@code
emf_mailbox_t *mailbox = NULL, *new_mailbox = NULL;
unsigned handle = 0;
int on_server = 0;
 
mailbox = malloc(sizeof(emf_mailbox_t));
memset(mailbox, 0x00, sizeof(emf_mailbox_t));
 
mailbox->name           = strdup("Personal"); 
mailbox->alias          = strdup("selfuse");            
mailbox->account_id     = 1; 
mailbox->local          = on_server; 
mailbox->mailbox_type    = 7;  
      
/* create new mailbox */           
if(EMF_ERROR_NONE != email_add_mailbox(mailbox,on_server,&handle))
      /* failure */
else
      /* success   */

@endcode


@li Update and Delete mailbox
@code
emf_mailbox_t *mailbox = NULL, *new_mailbox = NULL;
int on_server = 0;
unsigned handle = 0;

new_mailbox = malloc(sizeof(emf_mailbox_t));
memset(new_mailbox, 0x00, sizeof(emf_mailbox_t));
 
new_mailbox->name =  strdup("Personal001");
 
/* update mailbox */
if(EMF_ERROR_NONE != email_update_mailbox(mailbox,new_mailbox))
      /* failure */
else
      /* success   */
 
/* delete mailbox */
if(EMF_ERROR_NONE != email_delete_mailbox(mailbox,on_server,&handle))
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
emf_mailbox_t* mailbox_list = NULL;
int count = 0;
 
/*get list of mailboxes */
if(EMF_ERROR_NONE != email_get_mailbox_list(account_id, local_yn, &mailbox_list, &count))
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
emf_mail_t 
<table>
<tr><td>API</td>
<td>Return Value / Exceptions</td></tr>
 
<tr><td>int email_add_message(emf_mail_t* mail, emf_mailbox_t* mailbox, int with_server)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: -  Memory for params emf_mail_t* mail and emf_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_update_message(int mail_id, emf_mail_t* mail)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param emf_mail_t* mail should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_count_message(emf_mailbox_t* mailbox, int* total, int* unseen)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param emf_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>

<tr><td>int email_delete_message(emf_mailbox_t* mailbox, int *mail_ids, int num, int from_server)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for params int *mail_ids and mf_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_delete_all_message_in_mailbox(emf_mailbox_t* mailbox, int from_server)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param mf_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_clear_mail_data()  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>

<tr><td>int email_add_attachment(emf_mailbox_t* mailbox, int mail_id, emf_attachment_info_t* attachment)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param mf_mailbox_t* mailbox and mf_attachment_info_t* attachment hould be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_delete_attachment(emf_mailbox_t * mailbox, int mail_id, const char * attachment_id)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param emf_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>

<tr><td>int email_get_info(emf_mailbox_t* mailbox, int mail_id, emf_mail_info_t** info)   </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.@n Remarks: 
-# Memory for param emf_mailbox_t* mailbox should be allocated and deallocated by Application 
-# Memory allocation for param emf_mail_info_t** info will happen in email_get_info (). To free this memory application should call email_free_mail_info () </td></tr>
 
<tr><td>int email_free_mail_info(emf_mail_info_t** info_list, int count)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_get_header_info(emf_mailbox_t* mailbox, int mail_id, emf_mail_head_t** head)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.@n Remarks: 
-# Memory for param emf_mailbox_t* mailbox should be allocated and deallocated by Application 
-# Memory allocation for param emf_mail_head_t** head will happen in email_get_header_info (). To free this memory, application should call email_free_header_info () </td></tr>
 
<tr><td>int email_free_header_info(emf_mail_head_t** head_list, int count)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_get_body_info(emf_mailbox_t* mailbox, int mail_id ,emf_mail_body_t**  body)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.@n Remarks:
-# Memory for param emf_mailbox_t* mailbox should be allocated and deallocated by Application 
-# Memory allocation for param emf_mail_body_t** body will happen in email_get_body_info (). To free this memory, application should call email_free_body_info () </td></tr>
 
<tr><td>int email_free_body_info(emf_mail_body_t** body_list, int count)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_get_attachment_info(emf_mailbox_t* mailbox, int mail_id, const char* attachment_id, emf_attachment_info_t** attachment)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.@n Remarks:
-# Memory for param emf_mailbox_t* mailbox should be allocated and deallocated by Application 
-# Memory allocation for param emf_attachment_info_t** attachment will happen in email_get_attachment_info (). To free this memory, application should call email_free_attachment_info () </td></tr>
 
<tr><td>int email_free_attachment_info(emf_attachment_info_t** atch_info)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_get_mail(emf_mailbox_t* mailbox,  int mail_id, emf_mail_t** mail)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.@n Remarks:
-# Memory for param emf_mailbox_t* mailbox should be allocated and deallocated by Application 
-# Memory allocation for param emf_mail_t** mail will happen in email_get_mail (). To free this memory, application should call email_free_mail () </td></tr>
 
<tr><td>int email_modify_mail_flag(int mail_id, emf_mail_flag_t new_flag, int onserver)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_modify_seen_flag(int *mail_ids, int num, int seen_flag, int onserver)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param int *mail_ids should be allocated and deallocated by Application</td></tr>

<tr><td>int email_modify_extra_mail_flag(int mail_id, emf_extra_flag_t new_flag)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_move_mail_to_mailbox(int *mail_ids, int num, emf_mailbox_t* new_mailbox)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for params int *mail_ids and emf_mailbox_t* new_mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_move_all_mails_to_mailbox(emf_mailbox_t* src_mailbox, emf_mailbox_t* new_mailbox)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for params emf_mailbox_t* src_mailbox and emf_mailbox_t* new_mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_count_message_with_draft_flag(emf_mailbox_t* mailbox, int* total)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param emf_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>
 
<tr><td>int email_count_message_on_sending(emf_mailbox_t* mailbox, int* total)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory for param emf_mailbox_t* mailbox should be allocated and deallocated by Application</td></tr>
  
<tr><td>int email_get_mailbox_list(int account_id, emf_mailbox_t** mailbox_list, int* count ) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: - Memory allocation for param emf_mailbox_t** mailbox_list will happen in email_get_mailbox_list (). To free this memory, application should call email_free_mailbox ()</td></tr>
 
<tr><td>int email_free_mailbox(emf_mailbox_t** mailbox_list, int count)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_free_mail(emf_mail_t** mail_list, int count)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
   
<tr><td>int email_get_mail_flag(int account_id, int mail_id, emf_mail_flag_t* mail_flag)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_retry_send_mail( int mail_id, int timeout_in_sec)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_create_db_full()</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_get_mailbox_name_by_mail_id(int mail_id, char **pMailbox_name)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_cancel_send_mail( int mail_id)  </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_count_message_all_mailboxes(emf_mailbox_t* mailbox, int* total, int* unseen) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_get_latest_unread_mail_id(int account_id, int *pMailID) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_get_max_mail_count(int *Count) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_get_disk_space_usage(unsigned long *total_size)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
</table>
 
<b>Sample Code</b>

@li Add, Update, Count and Delete message
@code
emf_mailbox_t *mailbox = NULL;
emf_mail_t *mail = NULL;
int on_server = 0, account_id = 0, mail_id = 0;
char *pFilePath = "/tmp/mail.txt";
 
/* Fill mail data to save */
mailbox = malloc(sizeof(emf_mailbox_t));
memset(mailbox, 0x00, sizeof(emf_mailbox_t));
 
mail = malloc(sizeof(emf_mail_t));
memset(mail, 0x00, sizeof(emf_mail_t));
 
mail->info = malloc(sizeof(emf_mail_info_t));
memset(mail->info, 0x00, sizeof(emf_mail_info_t));
mail->info->account_id = /*account_id*/;
 
mail->head = malloc(sizeof(emf_mail_head_t));
memset(mail->head, 0x00, sizeof(emf_mail_head_t));
      
mail->body = malloc(sizeof(emf_mail_body_t));
memset(mail->body, 0x00, sizeof(emf_mail_body_t));
 
mail->head->to = strdup("\"ActiveSync8\" <ActiveSync8@test.local>"); 
mail->head->cc = strdup("bapina@gawab.com");
mail->head->bcc= strdup("<tom@gmail.com>");      
mail->head->subject = strdup("save.mailbox...");
 
mail->body->plain = strdup(pFilePath);
 
mailbox->name = strdup("DRAFT");
mailbox->account_id = account_id;
 
/* Add message */
if(EMF_ERROR_NONE != email_add_message(mail, mailbox, on_server))
      /* failure */
else
{
      /* success   */
      mail_id = mail->info->uid;
}
 
/* Update message */
/*  variable 'mail' should be filled with data on DB. */
/*  And change values you want to update. */
mail->head->subject = strdup("save.mailbox again...");
 
if(EMF_ERROR_NONE != email_update_message(mail_id,mail))
	/* failure */
else
	/* success   */
 
/* Count message */
int total = 0, unseen = 0;
 
/*  Get the total number of mails and the number of unseen mails */
if(EMF_ERROR_NONE != email_count_message(mailbox,&total,&unseen))
	/* failure */
else
	/* success   */
 
/* Delete message */
int *mail_ids, num = 0;
 
if(EMF_ERROR_NONE != email_delete_message(mailbox,mail_ids,num,on_server))
	/* failure */
else
	/* success   */
@endcode 
 
@li Delete all message in a specific mailbox
@code 
/* Delete all message in mailbox */
emf_mailbox_t *mailbox = NULL;
int on_server = 0;

mailbox = malloc(sizeof(emf_mailbox_t));
memset(mailbox, 0x00, sizeof(emf_mailbox_t));

mailbox->account_id = 1;
mailbox->name = strdup("INBOX");		

if( EMF_ERROR_NONE != email_delete_all_message_in_mailbox(mailbox, on_server))
      /* failure */
else

      /* success   */
@endcode


@li Clear all messages
@code
/* clear mail data */
if(EMF_ERROR_NONE !=  email_clear_mail_data())
      /* failure */
else
      /* success   */
@endcode

@li Get mail information : information, header, body, mail
@code
emf_mailbox_t *mailbox = NULL;
emf_mail_info_t *mail_info = NULL;
int mail_id = 1, account_id = 1;		/*  mail id and its account id to be gotten  */
 
mailbox = malloc(sizeof(emf_mailbox_t));
memset(mailbox, 0x00, sizeof(emf_mailbox_t));

/* Get mail info */
mailbox->account_id = account_id;
 
if ( EMF_ERROR_NONE != email_get_info(mailbox, mail_id, &mail_info))
      /* failure */
else
      /* success   */
 
/* free mail info */
email_free_mail_info(&mail_info,1);
 
/* Get mail header info */
emf_mail_head_t *head = NULL;
 
mailbox->account_id = account_id;
 
if(EMF_ERROR_NONE !=  email_get_header_info(mailbox,mail_id,&head))
      /* failure */
else
      /* success   */

/* free mail header info */
email_free_header_info(&head,1);
 
/* Get mail body info */
emf_mail_body_t *head = NULL;
 
mailbox->account_id = account_id;
 
if(EMF_ERROR_NONE !=  email_get_body_info(mailbox,mail_id,&body))
       /* failure */
else
       /* success   */
 
/* free mail body info */
email_free_body_info(&body,1);
 
/* Get mail info*/
emf_mail_t *mail = NULL;
int mail_id = 0;
 
if(EMF_ERROR_NONE !=  email_get_mail(mailbox,mail_id,&mail))
       /* failure */
else
       /* success   */
 
/* free mail body info */
email_free_mail(&mail,1);
 
/* free mailbox*/
email_free_mailbox(&mailbox,1);
 
@endcode



@li Modify flag
@code
emf_mail_flag_t newflag = {0};
int mail_id = 0;
int on_server = 0;
 
/* Modify mail flag*/
if(EMF_ERROR_NONE != email_modify_mail_flag(mail_id,newflag,on_server))
       /* failure */
else
       /* success   */

int mail_ids[] = {1, 2}; 
int num = 2;
int seen_flag = 0; 
int on_server = 0;
 
/* Modify seen flag*/
if(EMF_ERROR_NONE != email_modify_seen_flag(mail_ids, num, seen_flag,on_server))
       /* failure */
else
       /* success   */
 
/* Modify extra flag*/
int mail_id = 1;
if(EMF_ERROR_NONE != email_modify_extra_mail_flag(mail_id, newflag))
       /* failure */
else
       /* success   */
@endcode


@li Move mail
@code 
int mail_id[],account_id = 1;
emf_mailbox_t *mailbox = NULL;
char *mailbox_name = "INBOX";
 
mailbox = malloc(sizeof(emf_mailbox_t));
memset(mailbox, 0x00, sizeof(emf_mailbox_t));
 
mailbox->account_id = account_id;
mailbox->name = mailbox_name;
 
/* Move mail to given mailbox*/
if(EMF_ERROR_NONE !=  email_move_mail_to_mailbox(/*mail_id*/,/*num*/,mailbox))
      /* failure */
else
      /* success   */
 
/* free mailbox*/
email_free_mailbox(&mailbox,1);
 
emf_mailbox_t *src_mailbox = NULL,*dest_mailbox = NULL;
int src_account_id = 0, dest_account_id = 0;
char * src_mailbox_name = NULL, *dest_mailbox_name = NULL;
 
src_mailbox = malloc(sizeof(emf_mailbox_t));
memset(src_mailbox, 0x00, sizeof(emf_mailbox_t));
 
dest_mailbox = malloc(sizeof(emf_mailbox_t));
memset(dest_mailbox, 0x00, sizeof(emf_mailbox_t));
 
src_mailbox->account_id = /*src_account_id*/;
src_mailbox->name = /*src_mailbox_name*/
 
dest_mailbox->account_id = /*dest_account_id*/;
dest_mailbox->name = /*dest_mailbox_name*/
 
/*move all mails to given mailbox*/
if(EMF_ERROR_NONE !=  email_move_all_mails_to_mailbox(src_mailbox,dest_mailbox))
      /* failure */
else
      /* success   */
 
/* free mailbox*/
email_free_mailbox(&src_mailbox,1);
email_free_mailbox(&dest_mailbox,1);
 
int account_id = 0, total = 0;
emf_mailbox_t *mailbox = NULL;
char *mailbox_name = NULL;
 
mailbox = malloc(sizeof(emf_mailbox_t));
memset(mailbox, 0x00, sizeof(emf_mailbox_t));
 
mailbox->account_id = /*account_id*/;
mailbox->name = /*mailbox_name*/
 
/*count of draft msgs*/
if(EMF_ERROR_NONE !=  email_count_message_with_draft_flag(mailbox,&total))
      /* failure */
else
      /* success   */
 
/* free mailbox*/
email_free_mailbox(&mailbox,1);
@endcode
 
@li Count of msgs sent from given folde
@code
int account_id = 0, total = 0;
emf_mailbox_t *mailbox = NULL;
char *mailbox_name = NULL;
 
mailbox = malloc(sizeof(emf_mailbox_t));
memset(mailbox, 0x00, sizeof(emf_mailbox_t));
 
mailbox->account_id = /*account_id*/;
mailbox->name = /*mailbox_name*/
 
/*count of msgs sent from given mailbox*/
if(EMF_ERROR_NONE != email_count_message_on_sending(mailbox,&total))
      /* failure */
else
      /* success   */
 
/* free mailbox*/
email_free_mailbox(&mailbox,1);

@endcode


@li Get mailbox list 
@code
 
emf_mailbox_t* mailbox_list = NULL;
int account_id = 1, count = 0;
 
/* Get mailbox list*/
if(EMF_ERROR_NONE != email_get_mailbox_list(account_id,&mailbox_list,&count))
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
 
/* email_cancel_send_mail*/
int mail_id = 1;	/*  mail id of a mail which is on sending */
err = email_cancel_send_mail(mail_id);
 
@endcode



@li Get the Total count and Unread count of all mailboxes
@code 
/* Get the Total count and Unread count of all mailboxes */
emf_mailbox_t* mailbox = NULL;
int account_id = 1, total = 0, unseen = 0;
char *mailbox_name = NULL;
 
mailbox = malloc(sizeof(emf_mailbox_t));
memset(mailbox, 0x00, sizeof(emf_mailbox_t));
 
mailbox->account_id = /*account_id*/;
mailbox->name = /*mailbox_name*/
 
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
emf_option_t 
<table>
<tr><td>API</td>
<td>Return Value / Exceptions</td></tr>
 
<tr><td>int email_send_mail( emf_mailbox_t* mailbox, int mail_id, emf_option_t* sending_option, unsigned* handle)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: 
-# Memory allocation and de-allocation for input param is to be done by application.</td></tr>
 
<tr><td>int email_sync_header(emf_mailbox_t* mailbox, unsigned* handle)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation for input param is to be done by application.</td></tr>
 
<tr><td>int email_download_body(emf_mailbox_t* mailbox, int mail_id, int with_attachment, unsigned* handle) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation for input param is to be done by application.</td></tr>
 
<tr><td>int email_download_attachment(emf_mailbox_t* mailbox, int mail_id, const char* nth,  unsigned* handle)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>

<tr><td>int email_cancel_job(int account_id, int handle)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_get_pending_job(emf_action_t action, int account_id, int mail_id, emf_event_status_type_t * status)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>void email_get_network_status(int* on_sending, int* on_receiving) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_send_report(emf_mail_t* mail,  unsigned* handle) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation for input param is to be done by application.</td></tr>
 
<tr><td>int email_send_saved(int account_id, emf_option_t* sending_option, unsigned* handle)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation for input param is to be done by application.</td></tr>
 
<tr><td>int email_get_imap_mailbox_list(int account_id, const char* mailbox, unsigned* handle)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_sync_local_activity(int account_id)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>
</table>

<b>Sample Code</b>
@li Send a mail
@code
/* Send a mail */
emf_mailbox_t mbox;
emf_mail_t *mail = NULL;
emf_mail_head_t *head =NULL;
emf_mail_body_t *body =NULL;
emf_attachment_info_t attachment;
emf_option_t option;
int account_id = 1;
int err = EMF_ERROR_NONE;
 
mail =( emf_mail_t *)malloc(sizeof(emf_mail_t));
head =( emf_mail_head_t *)malloc(sizeof(emf_mail_head_t));
body =( emf_mail_body_t *)malloc(sizeof(emf_mail_body_t));
mail->info =( emf_mail_info_t*) malloc(sizeof(emf_mail_info_t));
memset(mail->info, 0x00, sizeof(emf_mail_info_t));
 
memset(&mbox, 0x00, sizeof(emf_mailbox_t));
memset(&option, 0x00, sizeof(emf_option_t));
memset(mail, 0x00, sizeof(emf_mail_t));
memset(head, 0x00, sizeof(emf_mail_head_t));
memset(body, 0x00, sizeof(emf_mail_body_t));
memset(&attachment, 0x00, sizeof(emf_attachment_info_t));
 
option.keep_local_copy = 1;
mbox.account_id = account_id;
mbox.name = strdup("OUTBOX");
head->to=strdup("test@test.com";
head->subject =strdup("test");
body->plain = strdup("/tmp/mail.txt");
mail->info->account_id = account_id;
mail->info->flags.draft = 1;
mail->body = body;
mail->head = head;
attachment.name = strdup("attach");
attachment.savename = strdup("tmp/mail.txt");
attachment.next = NULL;
mail->body->attachment = &attachment;
mail->body->attachment_num = 1;
if(EMF_ERROR_NONE  == email_add_message(mail,&mbox,1))
{
      err= email_send_mail(&mbox, mail->info->uid,&option,&handle);
}
@endcode

@li Download header of new emails from mail server
@code
/* Download header of new emails from mail server*/
emf_mailbox_t mbox;
int account_id = 1;
int err = EMF_ERROR_NONE;
unsigned handle = 0;
 
memset(&mbox, 0x00, sizeof(emf_mailbox_t));
 
mbox.account_id = account_id;
mbox.name = strdup("INBOX");
err = email_sync_header (&mbox,&handle);
@endcode


@li Download email body from server
@code
 
/*Download email body from server*/
emf_mailbox_t mbox;
int mail_id = 1;
int account_id = 1;
int handle = 0;
int err = EMF_ERROR_NONE;
 
memset(&mailbox, 0x00, sizeof(emf_mailbox_t));
mbox.account_id = account_id;
mbox.name = strdup("INBOX");
err= email_download_body (&mbox,mail_id,0,&handle);
 
@li Download a email nth-attachment from server
@code
/*Download a email nth-attachment from server*/
emf_mailbox_t mailbox;
int mail_id = 1;
int account_id = 1;     
char arg[50]; /* Input attachment number need to be download */
unsigned handle = 0;
int err = EMF_ERROR_NONE;
 
memset(&mailbox, 0x00, sizeof(emf_mailbox_t));
mailbox.name = "INBOX";
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
emf_rule_t
<table>
<tr><td>API</td>
<td>Return Value / Exceptions</td></tr>
 
<tr><td>int email_get_rule(int filter_id, emf_rule_t** filtering_set)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks: 
-# Memory allocation for the param emf_rule_t** filtering_set will be done in this api.
-# De-allocation is to be done by application.</td></tr>
 
<tr><td>int email_get_rule_list(emf_rule_t** filtering_set, int* count)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation for the param emf_rule_t** filtering_set will be done in this api.
-# De-allocation is to be done by application.</td></tr>
 
<tr><td>int email_add_rule(emf_rule_t* filtering_set) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation is to be done by application.
-# Use email_free_rule to free allocated memory.</td></tr>
 
<tr><td>int email_update_rule(int filter_id, emf_rule_t* new_set) </td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure@n Remarks:
-# Memory allocation and de-allocation is to be done by application.</td></tr>
-# Use email_free_rule to free allocated memory.
 
<tr><td>int email_delete_rule(int filter_id)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure.</td></tr>
 
<tr><td>int email_free_rule(emf_rule_t** filtering_set, int count)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
</table>

<b>Sample Code</b>
@li Filter Operation
@code
int err = EMF_ERROR_NONE;
emf_rule_t*  rule = NULL;
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
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_open_db(void)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure
@n Remarks:
@n Application should call email_close_db once db operation is over</td></tr>
 
<tr><td>int email_close_db(void)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure
@n Remarks: - 
@n This API should be called only if email_open_db () is called.</td></tr>
 
<tr><td>int email_service_begin(void)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure</td></tr>
 
<tr><td>int email_service_end(void)</td>
<td>Returns EMF_ERROR_NONE on success or negative value on failure
@n Remarks:
@n This API should be called only if email_service_begin () is called.</td></tr>
</table>
 
<b>Sample Code</b>
@li Initialize and Finalize Email MAPI Layer
@code
int err = EMF_ERROR_NONE;
 
/*  Initialize Email MAPI Layer before calling other MAPIs */
if(EMF_ERROR_NONE == email_service_begin())
{
	if(EMF_ERROR_NONE != email_open_db())
	{
		return false;
	}
	if(EMF_ERROR_NONE != email_init_storage())      
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
Emf_Mapi.h
@n Emf_Mapi_Types.h

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

