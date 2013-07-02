Name:       email-service
Summary:    E-mail Framework Middleware package
Version:    0.10.101
Release:    1
Group:      Messaging/Service
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    email.service
Source1001: 	email-service.manifest
Requires: connman
Requires: webkit2-efl
Requires(post):    /sbin/ldconfig
Requires(post):    systemd
Requires(post):    /usr/bin/sqlite3
Requires(post):    /usr/bin/vconftool
Requires(preun):   systemd
Requires(postun):  /sbin/ldconfig
Requires(postun):  systemd
BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gthread-2.0)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(vconf-internal-keys)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dbus-glib-1)
BuildRequires:  pkgconfig(contacts-service2)
BuildRequires:  pkgconfig(uw-imap-toolkit)
BuildRequires:  pkgconfig(drm-client)
BuildRequires:  pkgconfig(openssl)
BuildRequires:  pkgconfig(alarm-service)
BuildRequires:  pkgconfig(mm-player)
BuildRequires:  pkgconfig(mm-session)
BuildRequires:  pkgconfig(secure-storage)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(accounts-svc)
BuildRequires:  pkgconfig(libsystemd-daemon)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(gconf-2.0)
BuildRequires:  pkgconfig(cert-svc)
BuildRequires:  pkgconfig(badge)
BuildRequires:  pkgconfig(feedback)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(libwbxml2)
BuildRequires:  pkgconfig(msg-service)

%description
E-mail Framework Middleware Library/Binary package

%package tests
Summary:    E-mail Framework Middleware - Test Applications
Group:      Messaging/Testing
Requires:   %{name} = %{version}-%{release}

%description tests
E-mail Framework Middleware test application

%package devel
Summary:    E-mail Framework Middleware Development package
Group:      Development/Messaging
Requires:   %{name} = %{version}-%{release}

%description devel
E-mail Framework Middleware Development package


%prep
%setup -q
cp %{SOURCE1001} .

%build

export CFLAGS="${CFLAGS} -fPIC -Wall -g -fvisibility=hidden"
export CXXFLAGS="${CXXFLAGS} -fPIC -Wall -g -fvisibility=hidden"

%cmake .

make %{?_smp_mflags}

%install
mkdir -p %{buildroot}/usr/share/license
%make_install

mkdir -p %{buildroot}/usr/lib/systemd/user/tizen-middleware.target.wants
install -m 0644 %SOURCE1 %{buildroot}/usr/lib/systemd/user/
ln -sf ../email.service %{buildroot}/usr/lib/systemd/user/tizen-middleware.target.wants/


%post
/sbin/ldconfig

#################################################################
# Add preset account information
#################################################################
echo "[EMAIL-SERVICE] Start adding preset account information..." 

################################################################################################

# for default mail slot szie
vconftool set -t int    db/private/email-service/slot_size "100"        -g 6514

# for latest mail id
vconftool set -t int    db/private/email-service/latest_mail_id "0"     -g 6514

# for default account id
vconftool set -t int    db/private/email-service/default_account_id "0" -g 6514

# for default account id
vconftool set -t int    memory/sync/email "0" -i -g 6514

# for priority send 
vconftool set -t string db/private/email-service/noti_ringtone_path "Whistle.mp3" -g 6514
vconftool set -t int    db/private/email-service/noti_rep_type "0" -g 6514
vconftool set -t bool   db/private/email-service/noti_notification_ticker "0" -g 6514
vconftool set -t bool   db/private/email-service/noti_display_content_ticker "0" -g 6514
vconftool set -t bool   db/private/email-service/noti_badge_ticker "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/1 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/2 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/3 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/4 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/5 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/6 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/7 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/8 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/9 "0" -i -g 6514
vconftool set -t int    db/private/email-service/noti_private_id/10 "0" -i -g 6514

#################################################################
# Set executin script
#################################################################
echo "[EMAIL-SERVICE] Set executing script ..."
EMAIL_SERVICE_EXEC_SCRIPT=/etc/rc.d/init.d/email-service
EMAIL_SERVICE_BOOT_SCRIPT=/etc/rc.d/rc3.d/S70email-service
EMAIL_SERVICE_FASTBOOT_SCRIPT=/etc/rc.d/rc5.d/S70email-service
echo '#!/bin/sh' > ${EMAIL_SERVICE_EXEC_SCRIPT}
echo 'account_count=$(sqlite3 /opt/usr/dbspace/.email-service.db "select COUNT(*) from mail_account_tbl")' >> ${EMAIL_SERVICE_EXEC_SCRIPT}
echo 'if [ "$(echo "$account_count" | cut -c0-1)" == "0" ]' >> ${EMAIL_SERVICE_EXEC_SCRIPT}
echo 'then' >> ${EMAIL_SERVICE_EXEC_SCRIPT}
echo '	echo 'There is no account'' >> ${EMAIL_SERVICE_EXEC_SCRIPT}
echo 'elif [ "$(echo "$account_count" | cut -c0-1)" == "" ]' >> ${EMAIL_SERVICE_EXEC_SCRIPT}
echo 'then' >> ${EMAIL_SERVICE_EXEC_SCRIPT}
echo '	echo 'DB failure'' >> ${EMAIL_SERVICE_EXEC_SCRIPT}
echo 'else' >> ${EMAIL_SERVICE_EXEC_SCRIPT}
echo '	/usr/bin/email-service & ' >> ${EMAIL_SERVICE_EXEC_SCRIPT}
echo 'fi' >> ${EMAIL_SERVICE_EXEC_SCRIPT}
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
mkdir -p /opt/usr
mkdir -p /opt/usr/dbspace

sqlite3 /opt/usr/dbspace/.email-service.db 'PRAGMA journal_mode = PERSIST;'
sqlite3 /opt/usr/dbspace/.email-service.db < /opt/usr/data/email/res/email-service.sql

echo "[EMAIL-SERVICE] Finish Creating Email Tables."

chgrp 6006 /opt/usr/dbspace/.email-service.db*
chmod 664 /opt/usr/dbspace/.email-service.db
chmod 664 /opt/usr/dbspace/.email-service.db-journal

mkdir -m775 -p /opt/usr/data/email/.email_data
chgrp 6006 /opt/usr/data/email/.email_data
#chsmack -a 'email-service' /opt/usr/data/email/.email_data

mkdir -m775 -p /opt/usr/data/email/.email_data/tmp
chgrp 6006 /opt/usr/data/email/.email_data/tmp
#chsmack -a 'email-service' /opt/usr/data/email/.email_data/tmp

mkdir -p /opt/share/cert-svc/certs/trusteduser/email
chgrp 6006 /opt/share/cert-svc/certs/trusteduser/email

#if [ -f /opt/usr/dbspace/.email-service.db ]
#then
#	chsmack -a 'email-service::db' /opt/usr/dbspace/.email-service.db*
#fi

systemctl daemon-reload
if [ $1 == 1 ]; then
    systemctl restart email.service
fi

%preun
if [ $1 == 0]; then
    systemctl stop email.service
fi

%postun
/sbin/ldconfig
systemctl daemon-reload


%files
%manifest %{name}.manifest
#%manifest email-service.manifest
%{_bindir}/email-service
/opt/usr/data/email/res/*
%{_libdir}/lib*.so.*
/usr/lib/systemd/user/email.service
/usr/lib/systemd/user/tizen-middleware.target.wants/email.service
/usr/share/dbus-1/services/email-service.service
/usr/share/license/email-service/LICENSE

/opt/etc/smack/accesses.d/email-service.rule

%files tests
%manifest %{name}.manifest
/usr/bin/email-test-app

%files devel
%manifest %{name}.manifest
%{_includedir}/email-service/*.h
%{_libdir}/lib*.so
%{_libdir}/pkgconfig/*.pc
