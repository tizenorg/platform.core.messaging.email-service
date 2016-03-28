%global test_email_app_enabled 1

Name:       email-service
Summary:    E-mail Framework Middleware package
Version:    0.10.103
Release:    1
Group:      Messaging/Service
License:    Apache-2.0 and BSD-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    email-service.socket
Source2:    email-service.manifest
Source3:    email-service_init_db.sh
Source4:    email-service.service

%if "%{?profile}" == "wearable"
ExcludeArch: %{arm} %ix86 x86_64
%endif

%if "%{?profile}" == "tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

Requires: connman
Requires: gmime
Requires(post):    /sbin/ldconfig
Requires(post):    systemd
Requires(post):    /usr/bin/sqlite3
Requires(post):    /usr/bin/vconftool
Requires(post):    contacts-service2
Requires(post):    msg-service
Requires(preun):   systemd
Requires(postun):  /sbin/ldconfig
Requires(postun):  systemd
BuildRequires:  cmake
BuildRequires:  pkgconfig(gmime-2.6)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gthread-2.0)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(vconf-internal-keys)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(uw-imap-toolkit)
BuildRequires:  pkgconfig(openssl)
BuildRequires:  pkgconfig(alarm-service)
BuildRequires:  pkgconfig(key-manager)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(contacts-service2)
BuildRequires:  pkgconfig(accounts-svc)
BuildRequires:  pkgconfig(libsystemd-daemon)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  pkgconfig(tpkp-curl)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(cert-svc-vcore)
BuildRequires:  pkgconfig(badge)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(libwbxml2)
BuildRequires:  pkgconfig(msg-service)
BuildRequires:  pkgconfig(cynara-client)
BuildRequires:  pkgconfig(cynara-creds-socket)
BuildRequires:  pkgconfig(cynara-session)
BuildRequires:  pkgconfig(cynara-creds-commons)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(icu-i18n)
BuildRequires:  pkgconfig(storage)
BuildRequires:  pkgconfig(capi-network-connection)
BuildRequires:  pkgconfig(capi-system-device)
#BuildRequires:  pkgconfig(vasum)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(libsmack)
BuildRequires:  pkgconfig(sqlite3)
Requires: libtzplatform-config

%description
E-mail Framework Middleware Library/Binary package


%package devel
Summary:    E-mail Framework Middleware Development package
Group:      Development/Messaging
Requires:   %{name} = %{version}-%{release}

%description devel
E-mail Framework Middleware Development package


%prep
%setup -q
cp %{SOURCE2} .

%build

export CFLAGS="${CFLAGS} -fPIC -Wall -g -fvisibility=hidden -fdata-sections -ffunction-sections"
export CXXFLAGS="${CXXFLAGS} -fPIC -Wall -g -fvisibility=hidden"
export LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--rpath=%{_libdir} -Wl,--as-needed"

%cmake .  \
-DTZ_SYS_DATA=%TZ_SYS_DATA \
-DTZ_SYS_ETC=%TZ_SYS_ETC \
%if %{test_email_app_enabled}
        -DTEST_APP_SUPPORT=On
%endif

make %{?_smp_mflags}

find -name '*.pc' -exec sed -i -e 's/\$version/%{version}/g' {} \;

%install
mkdir -p %{buildroot}/usr/share/license
if [ -d %{_datarootdir}/license/email-service]; then
	rm -rf %{_datarootdir}/license/email-service
fi
%make_install

mkdir -p %{buildroot}%{_unitdir_user}
install -m 0644 %{SOURCE4} %{buildroot}%{_unitdir_user}/email-service.service

mkdir -p %{buildroot}%{_unitdir_user}/sockets.target.wants
install -m 0644 %{SOURCE1} %{buildroot}%{_unitdir_user}/email-service.socket
ln -s ../email-service.socket %{buildroot}%{_unitdir_user}/sockets.target.wants/email-service.socket

install -m 0775 %{SOURCE3} %{buildroot}%{_bindir}/

%post
/sbin/ldconfig

#################################################################
# Set executin script
#################################################################
chgrp %TZ_SYS_USER_GROUP %{_bindir}/email-service_init_db.sh

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
%manifest email-service.manifest
%if %{test_email_app_enabled}
%{_bindir}/email-test-app
%endif
%{TZ_SYS_DATA}/email/res/*
%{_bindir}/email-service
%{_libdir}/lib*.so.*
%{_unitdir_user}/email-service.service
%{_unitdir_user}/email-service.socket
%{_unitdir_user}/sockets.target.wants/email-service.socket
%{_datarootdir}/dbus-1/services/email-service.service
%{_datarootdir}/license/email-service
%attr(0775,root,root) /etc/rc.d/init.d/email-service
%{_bindir}/email-service_init_db.sh

%files devel
%{_includedir}/email-service/*.h
%{_libdir}/lib*.so
%{_libdir}/pkgconfig/*.pc
