#include "uts-email-dummy-utility.h"


int uts_Email_Add_Dummy_Account_01()
{
	email_account_t *account = NULL;
	int err_code = EMAIL_ERROR_NONE;

	tet_infoline("uts_Email_Add_Dummy_Account\n");
	account = (email_account_t  *)malloc(sizeof(email_account_t));
	if (account) {
		memset(account , 0x00, sizeof(email_account_t));

		account->account_name		= strdup("Gmail POP");
		account->user_display_name		= strdup("samsungtest09");
		account->user_email_address			= strdup("samsungtest09@gmail.com");
		account->reply_to_address		= strdup("samsungtest09@gmail.com");
		account->return_address			= strdup("samsungtest09@gmail.com");
		account->incoming_server_type	= EMAIL_SERVER_TYPE_POP3;
		account->incoming_server_address	= strdup("pop.gmail.com");
		account->incoming_server_port_number			= 995;
		account->incoming_server_secure_connection		= 1;
		account->retrieval_mode		= EMAIL_IMAP4_RETRIEVAL_MODE_NEW;
		account->incoming_server_user_name			= strdup("samsungtest09");
		account->incoming_server_password			= strdup("samsung09");
		account->outgoing_server_type		= EMAIL_SERVER_TYPE_SMTP;
		account->outgoing_server_address		= strdup("smtp.gmail.com");
		account->outgoing_server_port_number		= 465;
		account->outgoing_server_secure_connection		= 1;
		account->outgoing_server_need_authentication		= 1;
		account->outgoing_server_user_name		= strdup("samsungtest09");
		account->outgoing_server_password		= strdup("samsung09");
		account->pop_before_smtp			= 0;
		account->incoming_server_requires_apop			= 0;
		account->auto_download_size			= 0;			/*  downloading option, 0 is subject only, 1 is text body, 2 is normal */
		account->outgoing_server_use_same_authenticator			= 1;			/*  Specifies the 'Same as POP3' option, 0 is none, 1 is 'Same as POP3 */
		account->logo_icon_path		= NULL;
		account->options.priority		= 3;
		account->options.keep_local_copy      = 0;
		account->options.req_delivery_receipt = 0;
		account->options.req_read_receipt     = 0;
		account->options.download_limit	= 0;
		account->options.block_address	= 0;
		account->options.display_name_from    = NULL;
		account->options.reply_with_body      = 0;
		account->options.forward_with_files   = 0;
		account->options.add_myname_card      = 0;
		account->options.add_signature	= 0;
		account->options.signature = strdup("Gmail POP3 Signature");
		account->check_interval = 0;

		err_code = email_add_account(account);
		if (EMAIL_ERROR_NONE == err_code) {
			printf("email add account  :  success\n");
			tet_printf("email add account  :  success\n");
			return account->account_id;
		}
		else {
			printf("email add account  : failed err_code[%d]", err_code);
			tet_printf("email add account  : failed err_code[%d]", err_code);
		}
		email_free_account(&account, 1);
	}
	return 0;
}


int uts_Email_Add_Dummy_Message_02()
{
	int err_code = EMAIL_ERROR_NONE;
	int handle; 

	email_mail_data_t       *test_mail_data = NULL;
	email_attachment_data_t *attachment_data = NULL;
	email_meeting_request_t *meeting_req = NULL;
	email_mailbox_t *mailbox = NULL;

	FILE *fp;
	email_option_t option; 
	int count = 0;
	int i = 0;
	email_account_t *account = NULL ;

	memset(&option, 0x00, sizeof(email_option_t));
	option.keep_local_copy = 1;

	test_mail_data = malloc(sizeof(email_mail_data_t));
	memset(test_mail_data, 0x00, sizeof(email_mail_data_t));

	attachment_data = malloc(sizeof(email_attachment_data_t));
	memset(attachment_data, 0x00, sizeof(email_attachment_data_t));

	err_code = email_get_account_list(&account, &count);
	if (EMAIL_ERROR_NONE == err_code && count > 0) {

		test_mail_data->account_id = account[i].account_id;
		test_mail_data->flags_draft_field = 1;

		test_mail_data->save_status          = 1;
		test_mail_data->body_download_status = 1;

		test_mail_data->server_mail_id = strdup("TESTMAIL_1");

		test_mail_data->full_address_from = strdup("<samsungtest09@gmail.com>");
		test_mail_data->full_address_to = strdup("<samsungtest09@gmail.com>");
		test_mail_data->subject = strdup("Test");

		err_code = email_get_mailbox_by_mailbox_type(account[i].account_id , EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
		if (EMAIL_ERROR_NONE != err_code) {
			tet_printf("Email get mailbox by mailbox type failed : error_code[%d]\n", err_code);
			return err_code;
		}

		test_mail_data->mailbox_id           = mailbox->mailbox_id;
		test_mail_data->mailbox_type         = mailbox->mailbox_type;

		fp = fopen("/tmp/mail.txt", "w");
		fprintf(fp, "xxxxxxxxx");
		fclose(fp);
		test_mail_data->file_path_plain = strdup("/tmp/mail.txt");
		test_mail_data->attachment_count = 1;

		fp = fopen("/tmp/attach.txt", "w");
		fprintf(fp, "Simple Attachment");
		fclose(fp);

		attachment_data->attachment_name = strdup("Attach");
		attachment_data->attachment_path = strdup("/tmp/attach.txt");
		attachment_data->save_status      = 1;

		err_code = email_add_mail(test_mail_data, attachment_data, 1, NULL, 0);
		if (EMAIL_ERROR_NONE == err_code) {
			printf("email add mail : success\n");
		}
		else {
			printf("email add mail : failed err_code[%d]\n", err_code);
		}
	}
	else {
		printf("emaill get account list  :  failed err_code[%d]", err_code);
	}
	if (mailbox) {
		email_free_mailbox(&mailbox, 1);
		mailbox = NULL;
	}

	return err_code;
}

int uts_Email_Get_Mail_List_03(int account_id, email_mailbox_type_e mailbox_type, email_mail_list_item_t** mail_list,  int* result_count, email_sort_type_t sorting) {

	email_mailbox_t *mailbox = NULL;
	int err_code = EMAIL_ERROR_NONE;

	if(account_id != ALL_ACCOUNT && mailbox_type != 0)
		err_code = email_get_mailbox_by_mailbox_type(account_id , mailbox_type, &mailbox);

	if (EMAIL_ERROR_NONE == err_code) {
		err_code = email_get_mail_list(account_id , mailbox ? mailbox->mailbox_id : 0, 0, 0, EMAIL_GET_MAIL_LIST_MAX, sorting, mail_list, result_count);
	}

	if (mailbox) {
		email_free_mailbox(&mailbox, 1);
		mailbox = NULL;
	}

	return err_code;

}
