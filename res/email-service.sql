CREATE TABLE mail_account_tbl 
( 
	account_id                               INTEGER PRIMARY KEY,
	account_name                             VARCHAR(51),
	logo_icon_path                           VARCHAR(256),
	user_data                                BLOB,
	user_data_length                         INTEGER,
	account_svc_id                           INTEGER,
	sync_status                              INTEGER,
	sync_disabled                            INTEGER,
	default_mail_slot_size                   INTEGER,
	user_display_name                        VARCHAR(31),
	user_email_address                       VARCHAR(129),
	reply_to_address                         VARCHAR(129),
	return_address                           VARCHAR(129),
	incoming_server_type                     INTEGER,
	incoming_server_address                  VARCHAR(51),
	incoming_server_port_number              INTEGER,
	incoming_server_user_name                VARCHAR(51),
	incoming_server_password                 VARCHAR(51),
	incoming_server_secure_connection        INTEGER,
	retrieval_mode                           INTEGER,
	keep_mails_on_pop_server_after_download  INTEGER,
	check_interval                           INTEGER,
	auto_download_size                       INTEGER,
	outgoing_server_type                     INTEGER,
	outgoing_server_address                  VARCHAR(51),
	outgoing_server_port_number              INTEGER,
	outgoing_server_user_name                VARCHAR(51),
	outgoing_server_password                 VARCHAR(51),
	outgoing_server_secure_connection        INTEGER,
	outgoing_server_need_authentication      INTEGER,
	outgoing_server_use_same_authenticator   INTEGER,
	priority                                 INTEGER,
	keep_local_copy                          INTEGER,
	req_delivery_receipt                     INTEGER,
	req_read_receipt                         INTEGER,
	download_limit                           INTEGER,
	block_address                            INTEGER,
	block_subject                            INTEGER,
	display_name_from                        VARCHAR(256),
	reply_with_body                          INTEGER,
	forward_with_files                       INTEGER,
	add_myname_card                          INTEGER,
	add_signature                            INTEGER,
	signature                                VARCHAR(256),
	add_my_address_to_bcc                    INTEGER,
	pop_before_smtp                          INTEGER,
	incoming_server_requires_apop            INTEGER,
	smime_type                               INTEGER,
	certificate_path                         VARCHAR(256),
	cipher_type                              INTEGER,
	digest_type                              INTEGER,
	notification                             INTEGER
);
CREATE TABLE mail_box_tbl 
(    
	mailbox_id                       INTEGER PRIMARY KEY,
	account_id                       INTEGER,
	local_yn                         INTEGER,
	mailbox_name                     VARCHAR(256),    
	mailbox_type                     INTEGER,    
	alias                            VARCHAR(256),    
	deleted_flag                     INTEGER,    
	modifiable_yn                    INTEGER,    
	total_mail_count_on_server       INTEGER,
	has_archived_mails               INTEGER,    
	mail_slot_size                   INTEGER,
	no_select                        INTEGER,
	last_sync_time                   DATETIME
);
CREATE TABLE mail_read_mail_uid_tbl          
(    
	account_id                       INTEGER ,
	mailbox_id                       INTEGER ,
	local_uid                        INTEGER ,
	mailbox_name                     VARCHAR(256) ,
	s_uid                            VARCHAR(129) ,
	data1                            INTEGER ,
	data2                            VARCHAR(257) ,
	flag                             INTEGER ,
	idx_num                          INTEGER PRIMARY KEY
);
CREATE TABLE mail_rule_tbl          
(    
	account_id                       INTEGER ,
	rule_id                          INTEGER PRIMARY KEY,
	type                             INTEGER ,
	value                            VARCHAR(257)  ,
	action_type                      INTEGER ,
	target_mailbox_id                INTEGER ,
	flag1                            INTEGER ,
	flag2                            INTEGER    
);
CREATE TABLE mail_tbl
(
	mail_id                          INTEGER PRIMARY KEY,
	account_id                       INTEGER,
	mailbox_id                       INTEGER,
	mailbox_type                     INTEGER,
	subject                          TEXT,
	date_time                        DATETIME,
	server_mail_status               INTEGER,
	server_mailbox_name              VARCHAR(129),
	server_mail_id                   VARCHAR(129),
	message_id                       VARCHAR(257),
	reference_mail_id                INTEGER,
	full_address_from                TEXT,
	full_address_reply               TEXT,
	full_address_to                  TEXT,
	full_address_cc                  TEXT,
	full_address_bcc                 TEXT,
	full_address_return              TEXT,
	email_address_sender             TEXT collation user1,
	email_address_recipient          TEXT collation user1,
	alias_sender                     TEXT,
	alias_recipient                  TEXT,
	body_download_status             INTEGER,
	file_path_plain                  VARCHAR(257),
	file_path_html                   VARCHAR(257),
	file_path_mime_entity            VARCHAR(257),
	mail_size                        INTEGER,
	flags_seen_field                 BOOLEAN,
	flags_deleted_field              BOOLEAN,
	flags_flagged_field              BOOLEAN,
	flags_answered_field             BOOLEAN,
	flags_recent_field               BOOLEAN,
	flags_draft_field                BOOLEAN,
	flags_forwarded_field            BOOLEAN,
	DRM_status                       INTEGER,
	priority                         INTEGER,
	save_status                      INTEGER,
	lock_status                      INTEGER,
	report_status                    INTEGER,
	attachment_count                 INTEGER,
	inline_content_count             INTEGER,
	thread_id                        INTEGER,
	thread_item_count                INTEGER,
	preview_text                     TEXT, 
	meeting_request_status           INTEGER,
	message_class                    INTEGER,
	digest_type                      INTEGER,
	smime_type                       INTEGER,
	scheduled_sending_time           DATETIME,
	eas_data_length                  INTEGER,
	eas_data                         BLOB,
	FOREIGN KEY(account_id)          REFERENCES mail_account_tbl(account_id)
);
CREATE TABLE mail_attachment_tbl 
( 
	attachment_id                            INTEGER PRIMARY KEY,
	attachment_name                          VARCHAR(257),
	attachment_path                          VARCHAR(257),
	attachment_size                          INTEGER,
	mail_id                                  INTEGER,
	account_id                               INTEGER,
	mailbox_id                               INTEGER,
	attachment_save_status                   INTEGER,
	attachment_drm_type                      INTEGER,
	attachment_drm_method                    INTEGER,
	attachment_inline_content_status         INTEGER,
	attachment_mime_type                     VARCHAR(257)
);
CREATE TABLE mail_partial_body_activity_tbl
(
	account_id			INTEGER,
	mail_id                      	INTEGER,
	server_mail_id               	INTEGER,
	activity_id                  	INTEGER PRIMARY KEY,
	activity_type                	INTEGER,
	mailbox_id                   	INTEGER,
	mailbox_name                 	VARCHAR(4000)
);
CREATE TABLE mail_meeting_tbl
(
	mail_id                          INTEGER PRIMARY KEY,
	account_id                       INTEGER,
	mailbox_id                       INTEGER,
	meeting_response                 INTEGER,
	start_time                       INTEGER,
	end_time                         INTEGER,
	location                         TEXT ,
	global_object_id                 TEXT ,
	offset                           INTEGER,
	standard_name                    TEXT ,
	standard_time_start_date         INTEGER,
	standard_bias                    INTEGER,
	daylight_name                    TEXT ,
	daylight_time_start_date         INTEGER,
	daylight_bias                    INTEGER
);
CREATE TABLE mail_local_activity_tbl  
(  
	activity_id                      INTEGER,
	account_id                       INTEGER,
	mail_id                          INTEGER,
	activity_type                    INTEGER, 
	server_mailid                    VARCHAR(129),
	src_mbox                         VARCHAR(129),
	dest_mbox                        VARCHAR(129) 
);
CREATE TABLE mail_certificate_tbl 
( 
	certificate_id                   INTEGER,
	issue_year                       INTEGER,
	issue_month                      INTEGER,
	issue_day                        INTEGER,
	expiration_year                  INTEGER,
	expiration_month                 INTEGER,
	expiration_day                   INTEGER,
	issue_organization_name          VARCHAR(256),
	email_address                    VARCHAR(129),
	subject_str                      VARCHAR(256),
	filepath                         VARCHAR(256),
	password                         VARCHAR(51)
);
CREATE TABLE mail_task_tbl  
(  
	task_id                          INTEGER PRIMARY KEY,
	task_type                        INTEGER,
	task_status                      INTEGER,
	task_priority                    INTEGER,
	task_parameter_length            INTEGER, 
	task_parameter                   BLOB,
	date_time                        DATETIME
);
CREATE VIRTUAL TABLE mail_text_tbl USING fts4
(
	mail_id				 INTEGER,
	account_id			 INTEGER,
	mailbox_id			 INTEGER,
	body_text			 TEXT
);
CREATE UNIQUE INDEX mail_account_idx1 ON mail_account_tbl (account_id);
CREATE UNIQUE INDEX mail_box_idx1 ON mail_box_tbl (mailbox_id);
CREATE UNIQUE INDEX mail_read_mail_uid_idx1 ON mail_read_mail_uid_tbl (account_id, mailbox_id, local_uid, mailbox_name, s_uid);
CREATE UNIQUE INDEX mail_idx1 ON mail_tbl (mail_id, account_id);
CREATE UNIQUE INDEX mail_attachment_idx1 ON mail_attachment_tbl (mail_id, attachment_id);
CREATE UNIQUE INDEX mail_meeting_idx1 ON mail_meeting_tbl (mail_id);
CREATE UNIQUE INDEX task_idx1 ON mail_task_tbl (task_id);
CREATE INDEX mail_idx_date_time ON mail_tbl (date_time);
CREATE INDEX mail_idx_thread_item_count ON mail_tbl (thread_item_count);
