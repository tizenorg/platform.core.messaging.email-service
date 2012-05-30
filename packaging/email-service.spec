Name:       email-service
Summary:    E-mail Framework Middleware package
Version:    0.4.2
Release:    1
Group:      System/Libraries
License:    TBD
Source0:    %{name}-%{version}.tar.gz
Source1001: packaging/email-service.manifest 
Requires(post):    /sbin/ldconfig
Requires(post):    /usr/bin/sqlite3
Requires(post):    /usr/bin/vconftool
Requires(postun):  /sbin/ldconfig
BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gthread-2.0)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dbus-glib-1)
BuildRequires:  pkgconfig(contacts-service)
BuildRequires:  pkgconfig(uw-imap-toolkit)
BuildRequires:  pkgconfig(drm-service)
BuildRequires:  pkgconfig(openssl)
BuildRequires:  pkgconfig(alarm-service)
BuildRequires:  pkgconfig(mm-player)
BuildRequires:  pkgconfig(mm-session)
BuildRequires:  pkgconfig(devman_haptic)
BuildRequires:  pkgconfig(secure-storage)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(accounts-svc)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(gconf-2.0)


BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description
E-mail Framework Middleware Library/Binary package


%package devel
Summary:    E-mail Framework Middleware Development package
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
E-mail Framework Middleware Development package


%prep
%setup -q

%build
cp %{SOURCE1001} .

export CFLAGS="${CFLAGS} -fPIC -Wall -g -fvisibility=hidden"
export CXXFLAGS="${CXXFLAGS} -fPIC -Wall -g -fvisibility=hidden"
export LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--rpath=%{_prefix}/lib -Wl,--as-needed"

cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

# Call make instruction with smp support
#make %{?jobs:-j%jobs}
make

%install
%make_install

%clean
rm -rf %{buildroot}

%post
/sbin/ldconfig


#################################################################
# Add preset account information
#################################################################
echo "[EMAIL-SERVICE] Start adding preset account information..." 

#################################################################
# Email Settings
#################################################################

#if [ ${USER} = "root" ]
#then

	## Setting
	# Sending
	vconftool set -g 5000 -t bool	db/Services/Email/Sending/KeepCopy			"1"
	vconftool set -g 5000 -t bool	db/Services/Email/Sending/SendMeCopy		"1"
	vconftool set -g 5000 -t bool	db/Services/Email/Sending/ReqDeliveryRep	"0"
	vconftool set -g 5000 -t bool	db/Services/Email/Sending/ReqReadRep		"0"
	vconftool set -g 5000 -t int	db/Services/Email/Sending/Priority			"1"
	vconftool set -g 5000 -t string	db/Services/Email/Sending/ActiveAccount		""
	vconftool set -g 5000 -t bool	db/Services/Email/Sending/IncBodyReply		"1"
	vconftool set -g 5000 -t bool	db/Services/Email/Sending/IncAttachFwd		"1"
	# Receiving
	vconftool set -g 5000 -t int	db/Services/Email/Receiving/AutoPoll		"0"
	vconftool set -g 5000 -t int	db/Services/Email/Receiving/PollTime		"0"
	vconftool set -g 5000 -t int	db/Services/Email/Receiving/SendReadRep		"2"
	vconftool set -g 5000 -t int	db/Services/Email/Receiving/Reclimit		"0"
	vconftool set -g 5000 -t int	db/Services/Email/Receiving/FetchOption		"0"
	vconftool set -g 5000 -t bool	db/Services/Email/Receiving/KeepServer		"1"
	vconftool set -g 5000 -t int	db/Services/Email/Receiving/ServDelOption	"1"

	vconftool set -g 5000 -t int    db/Services/Email/NbAccount                	"6"

	## Accounts

	# Gmail
	vconftool set -g 5000 -t string db/Services/Email/1/General/NetworkName		"default"
	vconftool set -g 5000 -t string	db/Services/Email/1/General/AccountName		"Gmail"
	vconftool set -g 5000 -t string db/Services/Email/1/General/EmailAddr		""
	vconftool set -g 5000 -t string db/Services/Email/1/General/UserId			""
	vconftool set -g 5000 -t string	db/Services/Email/1/General/Password		""
	vconftool set -g 5000 -t string	db/Services/Email/1/General/LoginType		"username_type"

	# MailboxType : pop3(0), imap4(1) -> pop3(1), imap4(2) in email-service
	vconftool set -g 5000 -t int    db/Services/Email/1/Incoming/MailboxType	"1"
	vconftool set -g 5000 -t string db/Services/Email/1/Incoming/ServAddr       "imap.gmail.com"
	vconftool set -g 5000 -t int    db/Services/Email/1/Incoming/Port           "993"
	vconftool set -g 5000 -t int    db/Services/Email/1/Incoming/Secure         "1"
	vconftool set -g 5000 -t bool	db/Services/Email/1/Incoming/Apop			"0"
	vconftool set -g 5000 -t bool	db/Services/Email/1/Incoming/AutoEmailSync	"0"
	vconftool set -g 5000 -t bool	db/Services/Email/1/Incoming/IncludeAttach	"0"
	vconftool set -g 5000 -t int	db/Services/Email/1/Incoming/ImapFetchOpt	"1"

	vconftool set -g 5000 -t string	db/Services/Email/1/Outgoing/ServAddr		"smtp.gmail.com"
	vconftool set -g 5000 -t int	db/Services/Email/1/Outgoing/Port			"465"
	vconftool set -g 5000 -t bool	db/Services/Email/1/Outgoing/SmtpAuth		"0"
	vconftool set -g 5000 -t int	db/Services/Email/1/Outgoing/Secure			"1"
	vconftool set -g 5000 -t bool	db/Services/Email/1/Outgoing/SameIdPwd		"1"
	vconftool set -g 5000 -t bool	db/Services/Email/1/Outgoing/PopBeforeSmtp	"0"

	# Hotmail
	vconftool set -g 5000 -t string db/Services/Email/2/General/NetworkName		"default"
	vconftool set -g 5000 -t string	db/Services/Email/2/General/AccountName		"Hotmail"
	vconftool set -g 5000 -t string db/Services/Email/2/General/EmailAddr		""
	vconftool set -g 5000 -t string db/Services/Email/2/General/UserId			""
	vconftool set -g 5000 -t string	db/Services/Email/2/General/Password		""
	vconftool set -g 5000 -t string	db/Services/Email/2/General/LoginType		"username_type"

	# MailboxType : pop3(0), imap4(1) -> pop3(1), imap4(2) in email-service
	vconftool set -g 5000 -t int    db/Services/Email/2/Incoming/MailboxType	"0"
	vconftool set -g 5000 -t string db/Services/Email/2/Incoming/ServAddr       "pop3.live.com"
	vconftool set -g 5000 -t int    db/Services/Email/2/Incoming/Port           "995"
	vconftool set -g 5000 -t int    db/Services/Email/2/Incoming/Secure         "1"
	vconftool set -g 5000 -t bool	db/Services/Email/2/Incoming/Apop			"0"
	vconftool set -g 5000 -t bool	db/Services/Email/2/Incoming/AutoEmailSync	"0"
	vconftool set -g 5000 -t bool	db/Services/Email/2/Incoming/IncludeAttach	"0"
	vconftool set -g 5000 -t int	db/Services/Email/2/Incoming/ImapFetchOpt	"1"

	vconftool set -g 5000 -t string	db/Services/Email/2/Outgoing/ServAddr		"smtp.live.com"
	vconftool set -g 5000 -t int	db/Services/Email/2/Outgoing/Port			"587"
	vconftool set -g 5000 -t bool	db/Services/Email/2/Outgoing/SmtpAuth		"0"
	vconftool set -g 5000 -t int	db/Services/Email/2/Outgoing/Secure			"2"
	vconftool set -g 5000 -t bool	db/Services/Email/2/Outgoing/SameIdPwd		"1"
	vconftool set -g 5000 -t bool	db/Services/Email/2/Outgoing/PopBeforeSmtp	"0"

	# AOL
	vconftool set -g 5000 -t string db/Services/Email/3/General/NetworkName		"default"
	vconftool set -g 5000 -t string	db/Services/Email/3/General/AccountName		"AOL"
	vconftool set -g 5000 -t string db/Services/Email/3/General/EmailAddr		""
	vconftool set -g 5000 -t string db/Services/Email/3/General/UserId			""
	vconftool set -g 5000 -t string	db/Services/Email/3/General/Password		""
	vconftool set -g 5000 -t string	db/Services/Email/3/General/LoginType		"username_type"

	# MailboxType : pop3(0), imap4(1) -> pop3(1), imap4(2) in email-service
	vconftool set -g 5000 -t int    db/Services/Email/3/Incoming/MailboxType	"1"
	vconftool set -g 5000 -t string db/Services/Email/3/Incoming/ServAddr       "imap.aol.com"
	vconftool set -g 5000 -t int    db/Services/Email/3/Incoming/Port           "143"
	vconftool set -g 5000 -t int    db/Services/Email/3/Incoming/Secure         "0"
	vconftool set -g 5000 -t bool	db/Services/Email/3/Incoming/Apop			"0"
	vconftool set -g 5000 -t bool	db/Services/Email/3/Incoming/AutoEmailSync	"0"
	vconftool set -g 5000 -t bool	db/Services/Email/3/Incoming/IncludeAttach	"0"
	vconftool set -g 5000 -t int	db/Services/Email/3/Incoming/ImapFetchOpt	"0"

	vconftool set -g 5000 -t string	db/Services/Email/3/Outgoing/ServAddr		"smtp.aol.com"
	vconftool set -g 5000 -t int	db/Services/Email/3/Outgoing/Port			"587"
	vconftool set -g 5000 -t bool	db/Services/Email/3/Outgoing/SmtpAuth		"0"
	vconftool set -g 5000 -t int	db/Services/Email/3/Outgoing/Secure			"0"
	vconftool set -g 5000 -t bool	db/Services/Email/3/Outgoing/SameIdPwd		"1"
	vconftool set -g 5000 -t bool	db/Services/Email/3/Outgoing/PopBeforeSmtp	"0"

	# Samsung 3G for test
	vconftool set -g 5000 -t string db/Services/Email/4/General/NetworkName		"default"
	vconftool set -g 5000 -t string	db/Services/Email/4/General/AccountName		"Samsung 3G"
	vconftool set -g 5000 -t string db/Services/Email/4/General/EmailAddr		""
	vconftool set -g 5000 -t string db/Services/Email/4/General/UserId			""
	vconftool set -g 5000 -t string	db/Services/Email/4/General/Password		""
	vconftool set -g 5000 -t string	db/Services/Email/4/General/LoginType		"username_type"

	# MailboxType : pop3(0), imap4(1) -> pop3(1), imap4(2) in email-service
	vconftool set -g 5000 -t int    db/Services/Email/4/Incoming/MailboxType	"0"
	vconftool set -g 5000 -t string db/Services/Email/4/Incoming/ServAddr       "165.213.73.235"
	vconftool set -g 5000 -t int    db/Services/Email/4/Incoming/Port           "995"
	vconftool set -g 5000 -t int    db/Services/Email/4/Incoming/Secure         "1"
	vconftool set -g 5000 -t bool	db/Services/Email/4/Incoming/Apop			"0"
	vconftool set -g 5000 -t bool	db/Services/Email/4/Incoming/AutoEmailSync	"0"
	vconftool set -g 5000 -t bool	db/Services/Email/4/Incoming/IncludeAttach	"0"
	vconftool set -g 5000 -t int	db/Services/Email/4/Incoming/ImapFetchOpt	"1"

	vconftool set -g 5000 -t string	db/Services/Email/4/Outgoing/ServAddr		"165.213.73.235"
	vconftool set -g 5000 -t int	db/Services/Email/4/Outgoing/Port			"465"
	vconftool set -g 5000 -t bool	db/Services/Email/4/Outgoing/SmtpAuth		"0"
	vconftool set -g 5000 -t int	db/Services/Email/4/Outgoing/Secure			"1"
	vconftool set -g 5000 -t bool	db/Services/Email/4/Outgoing/SameIdPwd		"1"
	vconftool set -g 5000 -t bool	db/Services/Email/4/Outgoing/PopBeforeSmtp	"0"

	# Samsung Imap for test
	vconftool set -g 5000 -t string db/Services/Email/5/General/NetworkName		"default"
	vconftool set -g 5000 -t string	db/Services/Email/5/General/AccountName		"Samsung Imap"
	vconftool set -g 5000 -t string db/Services/Email/5/General/EmailAddr		""
	vconftool set -g 5000 -t string db/Services/Email/5/General/UserId			""
	vconftool set -g 5000 -t string	db/Services/Email/5/General/Password		""
	vconftool set -g 5000 -t string	db/Services/Email/5/General/LoginType		"username_type"

	# MailboxType : pop3(0), imap4(1) -> pop3(1), imap4(2) in email-service
	vconftool set -g 5000 -t int    db/Services/Email/5/Incoming/MailboxType	"1"
	vconftool set -g 5000 -t string db/Services/Email/5/Incoming/ServAddr       "165.213.73.235"
	vconftool set -g 5000 -t int    db/Services/Email/5/Incoming/Port           "993"
	vconftool set -g 5000 -t int    db/Services/Email/5/Incoming/Secure         "1"
	vconftool set -g 5000 -t bool	db/Services/Email/5/Incoming/Apop			"0"
	vconftool set -g 5000 -t bool	db/Services/Email/5/Incoming/AutoEmailSync	"0"
	vconftool set -g 5000 -t bool	db/Services/Email/5/Incoming/IncludeAttach	"0"
	vconftool set -g 5000 -t int	db/Services/Email/5/Incoming/ImapFetchOpt	"1"

	vconftool set -g 5000 -t string	db/Services/Email/5/Outgoing/ServAddr		"165.213.73.235"
	vconftool set -g 5000 -t int	db/Services/Email/5/Outgoing/Port			"465"
	vconftool set -g 5000 -t bool	db/Services/Email/5/Outgoing/SmtpAuth		"0"
	vconftool set -g 5000 -t int	db/Services/Email/5/Outgoing/Secure			"1"
	vconftool set -g 5000 -t bool	db/Services/Email/5/Outgoing/SameIdPwd		"1"
	vconftool set -g 5000 -t bool	db/Services/Email/5/Outgoing/PopBeforeSmtp	"0"


	# Yahoo
	vconftool set -g 5000 -t string db/Services/Email/6/General/NetworkName		"default"
	vconftool set -g 5000 -t string	db/Services/Email/6/General/AccountName		"Yahoomail"
	vconftool set -g 5000 -t string db/Services/Email/6/General/EmailAddr		""
	vconftool set -g 5000 -t string db/Services/Email/6/General/UserId			""
	vconftool set -g 5000 -t string	db/Services/Email/6/General/Password		""
	vconftool set -g 5000 -t string	db/Services/Email/6/General/LoginType		"username_type"

	# MailboxType : pop3(0), imap4(1) -> pop3(1), imap4(2) in email-service
	vconftool set -g 5000 -t int    db/Services/Email/6/Incoming/MailboxType	"0"
	vconftool set -g 5000 -t string db/Services/Email/6/Incoming/ServAddr       "pop.mail.yahoo.co.kr"
	vconftool set -g 5000 -t int    db/Services/Email/6/Incoming/Port           "995"
	vconftool set -g 5000 -t int    db/Services/Email/6/Incoming/Secure         "1"
	vconftool set -g 5000 -t bool	db/Services/Email/6/Incoming/Apop			"0"
	vconftool set -g 5000 -t bool	db/Services/Email/6/Incoming/AutoEmailSync	"0"
	vconftool set -g 5000 -t bool	db/Services/Email/6/Incoming/IncludeAttach	"0"
	vconftool set -g 5000 -t int	db/Services/Email/6/Incoming/ImapFetchOpt	"1"

	vconftool set -g 5000 -t string	db/Services/Email/6/Outgoing/ServAddr		"smtp.mail.yahoo.co.kr"
	vconftool set -g 5000 -t int	db/Services/Email/6/Outgoing/Port			"465"
	vconftool set -g 5000 -t bool	db/Services/Email/6/Outgoing/SmtpAuth		"0"
	vconftool set -g 5000 -t int	db/Services/Email/6/Outgoing/Secure			"1"
	vconftool set -g 5000 -t bool	db/Services/Email/6/Outgoing/SameIdPwd		"1"
	vconftool set -g 5000 -t bool	db/Services/Email/6/Outgoing/PopBeforeSmtp	"0"

#fi


################################################################################################

#if [ ${USER} = "root" ]
#then

vconftool set -t string db/email/preset_account/aol/sending_address   "smtp.aol.com"
vconftool set -t int    db/email/preset_account/aol/sending_port      "587"
vconftool set -t int    db/email/preset_account/aol/sending_ssl       "0"
vconftool set -t int    db/email/preset_account/aol/receiving_type    "2"
vconftool set -t string db/email/preset_account/aol/receiving_address "imap.aol.com"
vconftool set -t int    db/email/preset_account/aol/receiving_port    "143"
vconftool set -t int    db/email/preset_account/aol/receiving_ssl     "0"

vconftool set -t string db/email/preset_account/gmail/sending_address   "smtp.gmail.com"
vconftool set -t int    db/email/preset_account/gmail/sending_port      "465"
vconftool set -t int    db/email/preset_account/gmail/sending_ssl       "1"
vconftool set -t int    db/email/preset_account/gmail/receiving_type    "2"
# for POP3 server
#vconftool set -t string db/email/preset_account/gmail/receiving_address "pop.gmail.com"
#vconftool set -t int    db/email/preset_account/gmail/receiving_port    "995"
# for IMAP4 server
vconftool set -t string db/email/preset_account/gmail/receiving_address "imap.gmail.com"
vconftool set -t int    db/email/preset_account/gmail/receiving_port    "993"
vconftool set -t int    db/email/preset_account/gmail/receiving_ssl     "1"

vconftool set -t string db/email/preset_account/yahoo/sending_address   "smtp.mail.yahoo.co.kr"
vconftool set -t int    db/email/preset_account/yahoo/sending_port      "465"
vconftool set -t int    db/email/preset_account/yahoo/sending_ssl       "1"
vconftool set -t int    db/email/preset_account/yahoo/receiving_type    "1"
vconftool set -t string db/email/preset_account/yahoo/receiving_address "pop.mail.yahoo.co.kr"
vconftool set -t int    db/email/preset_account/yahoo/receiving_port    "995"
vconftool set -t int    db/email/preset_account/yahoo/receiving_ssl     "1"

vconftool set -t string db/email/preset_account/hotmail/sending_address   "smtp.live.com"
vconftool set -t int    db/email/preset_account/hotmail/sending_port      "587"
vconftool set -t int    db/email/preset_account/hotmail/sending_ssl       "2"
vconftool set -t int    db/email/preset_account/hotmail/receiving_type    "1"
vconftool set -t string db/email/preset_account/hotmail/receiving_address "pop3.live.com"
vconftool set -t int    db/email/preset_account/hotmail/receiving_port    "995"
vconftool set -t int    db/email/preset_account/hotmail/receiving_ssl     "1"

vconftool set -t string db/email/preset_account/samsung_pop3/sending_address   "165.213.73.235"
vconftool set -t int    db/email/preset_account/samsung_pop3/sending_port      "465"
vconftool set -t int    db/email/preset_account/samsung_pop3/sending_ssl       "1"
vconftool set -t int    db/email/preset_account/samsung_pop3/receiving_type    "1"
vconftool set -t string db/email/preset_account/samsung_pop3/receiving_address "165.213.73.235"
vconftool set -t int    db/email/preset_account/samsung_pop3/receiving_port    "995"
vconftool set -t int    db/email/preset_account/samsung_pop3/receiving_ssl     "1"

vconftool set -t string db/email/preset_account/samsung_imap4/sending_address   "165.213.73.235"
vconftool set -t int    db/email/preset_account/samsung_imap4/sending_port      "465"
vconftool set -t int    db/email/preset_account/samsung_imap4/sending_ssl       "1"
vconftool set -t int    db/email/preset_account/samsung_imap4/receiving_type    "2"
vconftool set -t string db/email/preset_account/samsung_imap4/receiving_address "165.213.73.235"
vconftool set -t int    db/email/preset_account/samsung_imap4/receiving_port    "993"
vconftool set -t int    db/email/preset_account/samsung_imap4/receiving_ssl     "1"

# for Active Sync	- Let email app create this key
#vconftool set -t int    db/email_handle/active_sync_handle     "0"

# for contact sync - sync from the first contact change
vconftool set -t int    db/email/last_sync_time "0"

# for contact sync - sync from the first contact change
vconftool set -t int    db/email/slot_size "100"

# for badge
vconftool set -t int	db/badge/com.samsung.email "0"

#fi

echo "[EMAIL-SERVICE] Finish adding preset account information"


#################################################################
# Set executin script
#################################################################
echo "[EMAIL-SERVICE] Set executing script ..."
EMAIL_SERVICE_EXEC_SCRIPT=/etc/rc.d/init.d/email-service
EMAIL_SERVICE_BOOT_SCRIPT=/etc/rc.d/rc3.d/S70email-service
EMAIL_SERVICE_FASTBOOT_SCRIPT=/etc/rc.d/rc5.d/S70email-service
echo '#!/bin/sh' > ${EMAIL_SERVICE_EXEC_SCRIPT}
echo '/usr/bin/email-service &' >> ${EMAIL_SERVICE_EXEC_SCRIPT} 
chmod 755 ${EMAIL_SERVICE_EXEC_SCRIPT}
rm -rf ${EMAIL_SERVICE_BOOT_SCRIPT}
rm -rf ${EMAIL_SERVICE_FASTBOOT_SCRIPT}
ln -s ${EMAIL_SERVICE_EXEC_SCRIPT} ${EMAIL_SERVICE_BOOT_SCRIPT} 
ln -s ${EMAIL_SERVICE_EXEC_SCRIPT} ${EMAIL_SERVICE_FASTBOOT_SCRIPT}
echo "[EMAIL-SERVICE] Finish executing script ..."

#################################################################
# Create DB file and tables.
#################################################################
echo "[EMAIL-SERVICE] Creating Email Tables ..."
sqlite3 /opt/dbspace/.email-service.db 'PRAGMA journal_mode = PERSIST;
CREATE TABLE mail_account_tbl 
( 
	account_bind_type                INTEGER,
	account_name                     VARCHAR(51),
	receiving_server_type            INTEGER,
	receiving_server_addr            VARCHAR(51),
	email_addr                       VARCHAR(129),
	user_name                        VARCHAR(51),
	password                         VARCHAR(51),
	retrieval_mode                   INTEGER,
	port_num                         INTEGER,
	use_security                     INTEGER,
	sending_server_type              INTEGER,
	sending_server_addr              VARCHAR(51),
	sending_port_num                 INTEGER,
	sending_auth                     INTEGER,
	sending_security                 INTEGER,
	sending_user                     VARCHAR(51),
	sending_password                 VARCHAR(51),
	display_name                     VARCHAR(31),
	reply_to_addr                    VARCHAR(129),
	return_addr                      VARCHAR(129),
	account_id                       INTEGER PRIMARY KEY,
	keep_on_server                   INTEGER,
	flag1                            INTEGER,
	flag2                            INTEGER,
	pop_before_smtp                  INTEGER,
	apop                             INTEGER,
	logo_icon_path                   VARCHAR(256),
	preset_account                   INTEGER,
	target_storage                   INTEGER,
	check_interval                   INTEGER,
	priority                         INTEGER,
	keep_local_copy                  INTEGER,
	req_delivery_receipt             INTEGER,
	req_read_receipt                 INTEGER,
	download_limit                   INTEGER,
	block_address                    INTEGER,
	block_subject                    INTEGER,
	display_name_from                VARCHAR(256),
	reply_with_body                  INTEGER,
	forward_with_files               INTEGER,
	add_myname_card                  INTEGER,
	add_signature                    INTEGER,
	signature                        VARCHAR(256),
	add_my_address_to_bcc            INTEGER,
	my_account_id                    INTEGER,
	index_color                      INTEGER,
	sync_status                      INTEGER
);

CREATE TABLE mail_attachment_tbl 
( 
	attachment_id                    INTEGER PRIMARY KEY,
	attachment_name                  VARCHAR(257),
	attachment_path                  VARCHAR(257),
	attachment_size                  INTEGER,
	mail_id                          INTEGER,
	account_id                       INTEGER,
	mailbox_name                     VARCHAR(129),
	attachment_save_status           INTEGER,
	attachment_drm_type              INTEGER,
	attachment_drm_method            INTEGER,
	attachment_inline_content_status INTEGER,
	attachment_mime_type             VARCHAR(257)
);

CREATE TABLE mail_box_tbl 
(    
	mailbox_id                       INTEGER,
	account_id                       INTEGER,
	local_yn                         INTEGER,
	mailbox_name                     VARCHAR(256),    
	mailbox_type                     INTEGER,    
	alias                            VARCHAR(256),    
	sync_with_server_yn              INTEGER,    
	modifiable_yn                    INTEGER,    
	total_mail_count_on_server       INTEGER,
	has_archived_mails               INTEGER,    
	mail_slot_size                   INTEGER
);
CREATE TABLE mail_read_mail_uid_tbl          
(    
	account_id                       INTEGER ,
	local_mbox                       VARCHAR(129)  ,
	local_uid                        INTEGER ,
	mailbox_name                     VARCHAR(129)  ,
	s_uid                            VARCHAR(129)  ,
	data1                            INTEGER ,
	data2                            VARCHAR(257)  ,
	flag                             INTEGER ,
	idx_num                          INTEGER  PRIMARY KEY
);
CREATE TABLE mail_rule_tbl          
(    
	account_id                       INTEGER ,
	rule_id                          INTEGER  PRIMARY KEY,
	type                             INTEGER ,
	value                            VARCHAR(257)  ,
	action_type                      INTEGER ,
	dest_mailbox                     VARCHAR(129),
	flag1                            INTEGER  ,
	flag2                            INTEGER    
);
CREATE TABLE mail_tbl
(
	mail_id                          INTEGER PRIMARY_KEY,
	account_id                       INTEGER,
	mailbox_id                       INTEGER,
	mailbox_name                     VARCHAR(129),
	mailbox_type                     INTEGER,
	subject                          TEXT,
	date_time                        DATETIME,
	server_mail_status               INTEGER,
	server_mailbox_name              VARCHAR(129),
	server_mail_id                   VARCHAR(129),
	message_id                       VARCHAR(257),
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
	FOREIGN KEY(account_id)          REFERENCES mail_account_tbl(account_id)
);
CREATE TABLE mail_meeting_tbl
(
	mail_id                          INTEGER PRIMARY KEY,
	account_id                       INTEGER,
	mailbox_name                     TEXT ,
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


CREATE UNIQUE INDEX mail_account_idx1 ON mail_account_tbl (account_bind_type, account_id);
CREATE UNIQUE INDEX mail_attachment_idx1 ON mail_attachment_tbl (mail_id, attachment_id);
CREATE UNIQUE INDEX mail_box_idx1 ON mail_box_tbl (account_id, local_yn, mailbox_name);
CREATE UNIQUE INDEX mail_idx1 ON mail_tbl (mail_id, account_id);
CREATE UNIQUE INDEX mail_read_mail_uid_idx1 ON mail_read_mail_uid_tbl (account_id, local_mbox, local_uid, mailbox_name, s_uid);
CREATE UNIQUE INDEX mail_meeting_idx1 ON mail_meeting_tbl (mail_id);
CREATE INDEX mail_idx_date_time ON mail_tbl (date_time);
CREATE INDEX mail_idx_thread_item_count ON mail_tbl (thread_item_count);
'

echo "[EMAIL-SERVICE] Finish Creating Email Tables."


#################################################################
# Change file permission
#################################################################
#echo "[EMAIL-SERVICE] Start setting permission ..."
# 1. libraries
#chmod 644 /usr/lib/libemail-ipc.so.0.0.0
#chmod 644 /usr/lib/libemail-core.so.0.0.0
#chmod 644 /usr/lib/libemail-emn-storage.so.0.0.0
#chmod 644 /usr/lib/libemail-base.so.0.0.0
#chmod 644 /usr/lib/libem-storage.so.0.0.0
#chmod 644 /usr/lib/libem-network.so.0.0.0
#chmod 644 /usr/lib/libemail-mapi.so.0.0.0
#chmod 644 /usr/lib/libem-storage.so
#chmod 644 /usr/lib/libemail-base.so.0
#chmod 644 /usr/lib/libem-network.so.0
#chmod 644 /usr/lib/libemail-core.so.0
#chmod 644 /usr/lib/libemail-emn-storage.so
#chmod 644 /usr/lib/libemail-ipc.so
#chmod 644 /usr/lib/libemail-mapi.so.0
#chmod 644 /usr/lib/libem-storage.so.0
#chmod 644 /usr/lib/libem-network.so
#chmod 644 /usr/lib/libemail-ipc.so.0
#chmod 644 /usr/lib/libemail-core.so
#chmod 644 /usr/lib/libemail-base.so
#chmod 644 /usr/lib/libemail-mapi.so
#chmod 644 /usr/lib/libemail-emn-storage.so.0

# 2. executables
#chmod 700 /usr/bin/email-service_initDB
#chmod 700 /usr/bin/email-service

# 3. DB files
chmod 644 /opt/dbspace/.email-service.db                                   
chmod 644 /opt/dbspace/.email-service.db-journal       


#################################################################
# Change file owner
#################################################################
#echo "[EMAIL-SERVICE] Start setting owner ..."

	# 1. libraries
#	chown root:root /usr/lib/libemail-ipc.so.0.0.0
#	chown root:root /usr/lib/libemail-core.so.0.0.0
#	chown root:root /usr/lib/libemail-emn-storage.so.0.0.0
#	chown root:root /usr/lib/libemail-base.so.0.0.0
#	chown root:root /usr/lib/libem-storage.so.0.0.0
#	chown root:root /usr/lib/libem-network.so.0.0.0
#	chown root:root /usr/lib/libemail-mapi.so.0.0.0
#	chown root:root /usr/lib/libem-storage.so
#	chown root:root /usr/lib/libemail-base.so.0
#	chown root:root /usr/lib/libem-network.so.0
#	chown root:root /usr/lib/libemail-core.so.0
#	chown root:root /usr/lib/libemail-emn-storage.so
#	chown root:root /usr/lib/libemail-ipc.so
#	chown root:root /usr/lib/libemail-mapi.so.0
#	chown root:root /usr/lib/libem-storage.so.0
#	chown root:root /usr/lib/libem-network.so
#	chown root:root /usr/lib/libemail-ipc.so.0
#	chown root:root /usr/lib/libemail-core.so
#	chown root:root /usr/lib/libemail-base.so
#	chown root:root /usr/lib/libemail-mapi.so
#	chown root:root /usr/lib/libemail-emn-storage.so.0

	# 2. executables
#	chown root:root /usr/bin/email-service_initDB
#	chown root:root /usr/bin/email-service

	# 3. DB files
#if [ ${USER} = "root" ]
#then
    chown root:root /opt/dbspace/.email-service.db
    chown root:root /opt/dbspace/.email-service.db-journal
#fi

%postun -p /sbin/ldconfig



%files
%manifest email-service.manifest
%exclude /opt/dbspace/.email-service.db
%exclude /opt/dbspace/.email-service.db-journal
%exclude /usr/bin/email-test-app
%{_libdir}/lib*.so.*
%{_bindir}/email-service


%files devel
%manifest email-service.manifest
%{_includedir}/email-service/*.h
%{_libdir}/lib*.so
%{_libdir}/pkgconfig/*.pc


