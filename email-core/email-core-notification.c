
#include "email-core-notification.h"
#include "email-internal-types.h"

#define PKG "org.tizen.email"

static email_mail_person_info_t *_person_find(const char *email)
{
	contacts_query_h query;
	contacts_filter_h filter;
	contacts_list_h list = NULL;
	contacts_record_h record;
	int person_id = 0, ret;
	unsigned int count = 0;
	email_mail_person_info_t *person_info = NULL;
	char *display_name;

	unsigned int projection[] = {
		_contacts_person_email.person_id,
		_contacts_person_email.display_name
	};

	ret = contacts_query_create(_contacts_person_email._uri, &query);
	if (ret != CONTACTS_ERROR_NONE)
	{
		EM_DEBUG_LOG("contacts_query_create() error: %d", ret);
		goto query_error;
	}

	ret = contacts_query_set_projection (query, projection,
                                   sizeof(projection)/sizeof(unsigned int));
	if (ret != CONTACTS_ERROR_NONE)
	{
		EM_DEBUG_LOG("contacts_query_set_projection() error: %d", ret);
		goto projection_error;
	}

	contacts_filter_create(_contacts_person_email._uri, &filter);
	contacts_filter_add_str(filter, _contacts_person_email.email, CONTACTS_MATCH_EXACTLY, email);
	contacts_query_set_filter(query, filter);

	ret = contacts_db_get_records_with_query(query, 0, 0, &list);
	if (ret != CONTACTS_ERROR_NONE)
	{
		EM_DEBUG_LOG("contacts_db_get_records_with_query() error: %d", ret);
		goto error;
	}

	contacts_list_get_count(list, &count);
	if (count > 0)
	{
		ret = contacts_list_get_current_record_p(list, &record);
		if (ret != CONTACTS_ERROR_NONE)
		{
			EM_DEBUG_LOG("contacts_list_get_current_record_p() error: %d", ret);
			goto error;
		}

		ret = contacts_record_get_int(record, _contacts_person_email.person_id, &person_id);
		if (ret != CONTACTS_ERROR_NONE)
		{
			EM_DEBUG_LOG("contacts_record_get_int() error: %d", ret);
			goto error;
		}

		ret = contacts_record_get_str(record, _contacts_person_email.display_name, &display_name);
		if (ret != CONTACTS_ERROR_NONE)
		{
			EM_DEBUG_LOG("contacts_record_get_int() error: %d", ret);
			goto error;
		}

		person_info = calloc(1, sizeof(email_mail_person_info_t));
		if (person_info == NULL)
		{
			EM_DEBUG_LOG("could not allocate memory to email_mail_person_info_t");
			goto error;
		}

		person_info->display_name = strdup(display_name);
		person_info->person_id = person_id;
	}

error:
	contacts_filter_destroy(filter);
projection_error:
	contacts_query_destroy(query);
	contacts_list_destroy(list, true);
query_error:
	return person_info;
}

static unsigned char _is_valid_box(email_mailbox_type_e type)
{
	if (type == EMAIL_MAILBOX_TYPE_SENTBOX ||
		type == EMAIL_MAILBOX_TYPE_TRASH ||
		type == EMAIL_MAILBOX_TYPE_DRAFT ||
		type == EMAIL_MAILBOX_TYPE_SPAMBOX ||
		type == EMAIL_MAILBOX_TYPE_OUTBOX ||
		type == EMAIL_MAILBOX_TYPE_SEARCH_RESULT)
		return 0;
	return 1;
}

static void _notification_create(email_mail_data_t *mail, email_mail_person_info_t *person_info)
{
	notification_h noti = NULL;
	emstorage_account_tbl_t *account_tbl = NULL;
	bundle *args;
	char buf[1024];
	int ret;

	if (!emstorage_get_account_by_id(mail->account_id, EMAIL_ACC_GET_OPT_ACCOUNT_NAME, &account_tbl, true, &ret))
	{
		EM_DEBUG_EXCEPTION("emstorage_get_account_by_id failed - %d", ret);
		notification_free(noti);
		return;
	}

	if (!account_tbl->notification) {
		EM_DEBUG_LOG("account has disabled email notification.");
		emstorage_free_account(&account_tbl, 1, NULL);
		return;
	}

	noti = notification_create(NOTIFICATION_TYPE_NOTI);
	if (!noti)
	{
		EM_DEBUG_LOG("notification_create failed.");
		return;
	}

	notification_set_layout(noti, NOTIFICATION_LY_NOTI_EVENT_SINGLE);
	notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, "Email", NULL, NOTIFICATION_VARIABLE_TYPE_NONE);

	notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, account_tbl->account_name, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
	emstorage_free_account(&account_tbl, 1, NULL);

	notification_set_text(noti, NOTIFICATION_TEXT_TYPE_INFO_1, person_info->display_name, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);

	notification_set_text(noti, NOTIFICATION_TEXT_TYPE_INFO_2, mail->subject, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
	notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, EMAIL_NOTI_ICON_PATH);
	notification_set_application(noti, PKG);
	notification_set_pkgname(noti, PKG);
	notification_set_person(noti, person_info->person_id);
	notification_set_time_to_text(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1, mail->date_time);

	args = bundle_create();
	snprintf(buf, sizeof(buf), "%d", mail->mail_id);
	bundle_add(args, "mailid", buf);
	notification_set_args(noti, args, NULL);
	bundle_free(args);

	notification_insert(noti, NULL);
	notification_free(noti);
}

static void _notification_consume_by_id(int mailid)
{
	notification_list_h noti_list = NULL;
	notification_h noti = NULL;

	notification_get_list(NOTIFICATION_TYPE_NOTI, -1, &noti_list);
	while (noti_list)
	{
		int noti_mailid;
		bundle *args;
		const char *str_mailid;
		char *pkg_name;

		noti = notification_list_get_data(noti_list);

		notification_get_pkgname(noti, &pkg_name);
		if (!pkg_name)
			goto next;
		if (strcmp(pkg_name, PKG))
			goto next;

		notification_get_args(noti, &args, NULL);
		if (!args)
			goto next;

		str_mailid = bundle_get_val(args, "mailid");
		noti_mailid = atoi(str_mailid);

		if (noti_mailid != mailid)
			goto next;

		notification_delete(noti);
		break;
next:
		noti_list = notification_list_remove(noti_list, noti);
	}

	while (noti_list)
		noti_list = notification_list_remove(noti_list, noti);
}

static void _notification_consume(email_mail_data_t *mail, email_mail_person_info_t *person_info)
{
	notification_list_h noti_list = NULL;
	notification_h noti = NULL;

	notification_get_list(NOTIFICATION_TYPE_NOTI, -1, &noti_list);
	while (noti_list)
	{
		int noti_personid, noti_mailid;
		noti = notification_list_get_data(noti_list);
		bundle *args;
		const char *str_mailid;
		char *pkg_name;

		notification_get_pkgname(noti, &pkg_name);
		if (!pkg_name)
			goto next;
		if (strcmp(pkg_name, PKG))
			goto next;

		notification_get_person(noti, &noti_personid);
		if (noti_personid != person_info->person_id)
			goto next;

		notification_get_args(noti, &args, NULL);
		if (!args)
			goto next;

		str_mailid = bundle_get_val(args, "mailid");
		noti_mailid = atoi(str_mailid);

		if (noti_mailid != mail->mail_id)
			goto next;

		notification_delete(noti);
		break;
next:
		noti_list = notification_list_remove(noti_list, noti);
	}

	while (noti_list)
		noti_list = notification_list_remove(noti_list, noti);
}

static email_mail_person_info_t *_contact_unknown_create(const char *email)
{
	contacts_record_h contact = NULL, email_rec = NULL;
	int err, current_time, contact_id = 0, person_id = 0;
	email_mail_person_info_t *ret = NULL;

	err = contacts_record_create(_contacts_contact._uri, &contact);
	if (err != CONTACTS_ERROR_NONE) {
		EM_DEBUG_LOG("contacts_record_create() failed");
		return 0;
	}

	err = contacts_record_create(_contacts_email._uri, &email_rec);
	if (err != CONTACTS_ERROR_NONE) {
		EM_DEBUG_LOG("contacts_record_create() failed");
		goto error;
	}
	contacts_record_set_str(email_rec, _contacts_email.email, email);
	contacts_record_add_child_record(contact, _contacts_contact.email, email_rec);

	current_time = time(NULL);
	contacts_record_set_int(contact, _contacts_contact.is_unknown, current_time);

	err = contacts_db_insert_record(contact, &contact_id);
	if (err != CONTACTS_ERROR_NONE) {
		EM_DEBUG_LOG("contacts_db_insert_record() failed");
		goto error;
	}
	contacts_record_destroy(contact, true);

	contacts_db_get_record(_contacts_contact._uri, contact_id, &contact);
	contacts_record_get_int(contact, _contacts_contact.person_id, &person_id);

	ret = malloc(sizeof(email_mail_person_info_t));
	if (!ret)
	{
		EM_DEBUG_LOG("No memory to alloc email_mail_person_info_t");
		goto error;
	}
	ret->display_name = strdup(email);
	ret->person_id = person_id;

error:
	contacts_record_destroy(contact, true);
	return ret;
}

static void _person_info_free(email_mail_person_info_t **person_info)
{
	if (!person_info || !*person_info)
		return;
	free((*person_info)->display_name);
	free(*person_info);
	*person_info = NULL;
}

INTERNAL_FUNC void emcore_notification_handle(email_noti_on_storage_event transaction_type, int data1, int data2 , char *data3, int data4)
{
	email_mail_data_t *mail_data = NULL;
	email_mail_person_info_t *person_info = NULL;
	int mail_id;

	switch (transaction_type) {
		case NOTI_MAIL_ADD:
		{
			mail_id = data2;

			if (emcore_get_mail_data(mail_id, &mail_data) != EMAIL_ERROR_NONE) {
				EM_DEBUG_LOG("emcore_get_mail_data(%d, &mail_data) failed", mail_id);
				break;
			}

			if (mail_data->flags_seen_field)
				break;
			if (!_is_valid_box(mail_data->mailbox_type))
				break;

			person_info = _person_find(mail_data->email_address_sender);
			if (!person_info)
				person_info = _contact_unknown_create(mail_data->email_address_sender);

		    if (!person_info)
		    {
			EM_DEBUG_LOG("person_info == NULL");
			break;
		    }

		    _notification_create(mail_data, person_info);
		    _person_info_free(&person_info);
			break;
		}
		case NOTI_MAIL_UPDATE:
		{
			mail_id = data2;
			if (emcore_get_mail_data(mail_id, &mail_data) != EMAIL_ERROR_NONE) {
				EM_DEBUG_LOG("emcore_get_mail_data(%d, &mail_data) failed", mail_id);
				break;
			}

			if (!_is_valid_box(mail_data->mailbox_type))
				break;

			if (!mail_data->flags_seen_field)
				break;

			person_info = _person_find(mail_data->email_address_sender);
			if (!person_info)
				break;

			_notification_consume(mail_data, person_info);
			_person_info_free(&person_info);
			break;
		}
		case NOTI_MAIL_FIELD_UPDATE2:
		{
			int field = data2;
			int seen = data4;
			const char *mail_ids = data3;
			char *string, *pch;

			if (field != EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD || !seen)
				return;

			string = strdup(mail_ids);
			pch = strtok(string, ",");
			while (pch != NULL)
			{
				mail_id = atoi(pch);

				if (mail_data)
					emcore_free_mail_data_list(&mail_data, 1);
				mail_data = NULL;

				if (emcore_get_mail_data(mail_id, &mail_data) != EMAIL_ERROR_NONE) {
					EM_DEBUG_LOG("emcore_get_mail_data(%d, &mail_data) failed", mail_id);
					goto next_field_update;
				}

				person_info = _person_find(mail_data->email_address_sender);
				if (!person_info)
					goto next_field_update;

				_notification_consume(mail_data, person_info);
				_person_info_free(&person_info);
next_field_update:
				pch = strtok (NULL, ",");
			}
			free(string);
			break;
		}
		case NOTI_MAIL_DELETE:
		{
			const char *mail_ids = data3;
			char *string, *pch;

			string = strdup(mail_ids);
			pch = strtok(string,",");
			while (pch != NULL)
			{
				mail_id = atoi(pch);
				_notification_consume_by_id(mail_id);
				pch = strtok (NULL, ",");
			}
			free(string);
			break;
		}
		case NOTI_MAIL_MOVE:
		{
			const char *mail_ids = data3;
			char *string, *pch;

			string = strdup(mail_ids);
			pch = strtok(string,",");
			while (pch != NULL) {
				mail_id = atoi(pch);

				if (mail_data)
					emcore_free_mail_data_list(&mail_data, 1);
				mail_data = NULL;

				if (emcore_get_mail_data(mail_id, &mail_data) != EMAIL_ERROR_NONE) {
					EM_DEBUG_LOG("emcore_get_mail_data(%d, &mail_data) failed", mail_id);
					goto next_move;
				}

				//only when email is moved to trash or span box
				if (_is_valid_box(mail_data->mailbox_type))
					continue;

				person_info = _person_find(mail_data->email_address_sender);
				if (person_info)
					goto next_move;

				_notification_consume(mail_data, person_info);
				_person_info_free(&person_info);
next_move:
				pch = strtok (NULL, ",");
			}
			free(string);
			break;
		}
		default:
			break;
	}

	if (mail_data)
		emcore_free_mail_data_list(&mail_data, 1);
}

INTERNAL_FUNC int emcore_initialize_notification(void)
{
	contacts_connect2();
	return EMAIL_ERROR_NONE;
}

INTERNAL_FUNC int emcore_shutdown_notification(void)
{
	contacts_disconnect2();
	return EMAIL_ERROR_NONE;
}
