%global test_email_app_enabled 0

Name:       email-service
Summary:    E-mail Framework Middleware
Version:    0.10.101
Release:    0
Group:      Messaging/Service
License:    Apache-2.0 and BSD-3-Clause
Source0:    %{name}-%{version}.tar.gz
Source1:    email.service
Source2:    email-service.manifest
Source3:    email-service_init_db.sh
Suggests:          webkit2-efl
Requires:          connman
Requires(post):    /sbin/ldconfig
Requires(post):    systemd
Requires(post):    /usr/bin/sqlite3
Requires(post):    /usr/bin/vconftool
Requires(post):    ss-server
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
BuildRequires:  pkgconfig(pmapi)
BuildRequires:  pkgconfig(libsmack)
BuildRequires:  pkgconfig(security-server)
BuildRequires:  pkgconfig(deviced)
BuildRequires:  pkgconfig(icu-i18n)
BuildRequires:  pkgconfig(libtzplatform-config)

%description
E-mail Framework Middleware Library/Binary package.


%package devel
Summary:    E-mail Framework Middleware (dev)
Group:      Development/Messaging
Requires:   %{name} = %{version}-%{release}

%description devel
E-mail Framework Middleware Development package.


%prep
%setup -q
cp %{SOURCE2} .


%build

export CFLAGS="${CFLAGS} -fPIC -Wall -g -fvisibility=hidden"
export CXXFLAGS="${CXXFLAGS} -fPIC -Wall -g -fvisibility=hidden"
export LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--rpath=%{_libdir} -Wl,--as-needed"

%cmake . \
         -DTZ_SYS_SMACK=%TZ_SYS_SMACK \
         -DTZ_SYS_DATA=%TZ_SYS_DATA \
         -DTZ_SYS_ETC=%TZ_SYS_ETC \
%if %{test_email_app_enabled}
         -DTEST_APP_SUPPORT=On
%endif

%__make %{?_smp_mflags}

find -name '*.pc' -exec sed -i -e 's/\$version/%{version}/g' {} \;


%install
%make_install

mkdir -p %{buildroot}%{_unitdir_user}/tizen-middleware.target.wants
install -m 0644 %{SOURCE1} %{buildroot}%{_unitdir_user}
ln -sf ../email.service %{buildroot}%{_unitdir_user}/tizen-middleware.target.wants/
install -m 0775 %{SOURCE3} %{buildroot}%{_bindir}/


%post
/sbin/ldconfig
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


%post devel -p /sbin/ldconfig

%postun devel -p /sbin/ldconfig


%files
%manifest email-service.manifest
%if %{test_email_app_enabled}
%{_bindir}/email-test-app
%endif
%{TZ_SYS_DATA}/email/res/*
%{_bindir}/email-service
%{_libdir}/lib*.so.*
%{_libdir}/libemail-core-sound.so
%{_libdir}/libemail-core-sound.so.*
%{_unitdir_user}/email.service
%{_unitdir_user}/tizen-middleware.target.wants/email.service
%{_datarootdir}/dbus-1/services/email-service.service
%config %{TZ_SYS_SMACK}/accesses.d/email-service.rule
%{_bindir}/email-service_init_db.sh
%license NOTICE LICENSE.APLv2 LICENSE.BSD

%files devel
%{_includedir}/email-service/*.h
%{_libdir}/lib*.so
%{_libdir}/libemail-core-sound.so
%{_libdir}/libemail-core-sound.so.*
%{_libdir}/pkgconfig/*.pc
