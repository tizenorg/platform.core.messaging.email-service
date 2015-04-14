%global test_email_app_enabled 0

Name:       email-service
Summary:    E-mail Framework Middleware package
Version:    0.10.101
Release:    1
Group:      Messaging/Service
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    email.service
Source2:    email-service.manifest
Source3:    email-service_init_db.sh
Requires: connman
Suggests: webkit2-efl
Requires(post):    /sbin/ldconfig
Requires(post):    systemd
Requires(post):    /usr/bin/sqlite3
Requires(post):    /usr/bin/vconftool
Requires(post):    libss-client
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
BuildRequires:  pkgconfig(deviced)
BuildRequires:  pkgconfig(icu-i18n)
BuildRequires:  pkgconfig(cynara-client)
BuildRequires:  pkgconfig(cynara-creds-socket)
BuildRequires:  pkgconfig(cynara-session)
BuildRequires:  pkgconfig(cynara-creds-commons)
BuildRequires:  pkgconfig(libtzplatform-config)
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

export CFLAGS="${CFLAGS} -fPIC -Wall -g -fvisibility=hidden"
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

mkdir -p %{buildroot}/usr/lib/systemd/user/tizen-middleware.target.wants
install -m 0644 %SOURCE1 %{buildroot}/usr/lib/systemd/user/
ln -sf ../email.service %{buildroot}/usr/lib/systemd/user/tizen-middleware.target.wants/
install -m 0775 %{SOURCE3} %{buildroot}%{_bindir}/

%post
/sbin/ldconfig

#################################################################
# Set executin script
#################################################################
echo "[EMAIL-SERVICE] Set executing script ..."
mkdir -p %{buildroot}/etc/rc.d/rc3.d/
mkdir -p %{buildroot}/etc/rc.d/rc5.d/
EMAIL_SERVICE_EXEC_SCRIPT=/etc/rc.d/init.d/email-service
EMAIL_SERVICE_BOOT_SCRIPT=/etc/rc.d/rc3.d/S70email-service
EMAIL_SERVICE_FASTBOOT_SCRIPT=/etc/rc.d/rc5.d/S70email-service

chmod 755 ${EMAIL_SERVICE_EXEC_SCRIPT}
rm -rf ${EMAIL_SERVICE_BOOT_SCRIPT}
rm -rf ${EMAIL_SERVICE_FASTBOOT_SCRIPT}
ln -s ${EMAIL_SERVICE_EXEC_SCRIPT} ${EMAIL_SERVICE_BOOT_SCRIPT}
ln -s ${EMAIL_SERVICE_EXEC_SCRIPT} ${EMAIL_SERVICE_FASTBOOT_SCRIPT}
echo "[EMAIL-SERVICE] Finish executing script ..."

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
%{_libdir}/libemail-core-sound.so
%{_libdir}/libemail-core-sound.so.*
%{_unitdir_user}/email.service
%{_unitdir_user}/tizen-middleware.target.wants/email.service
%{_datarootdir}/dbus-1/services/email-service.service
%{_datarootdir}/license/email-service
%attr(0755,root,root) /etc/rc.d/init.d/email-service
%{_bindir}/email-service_init_db.sh

%files devel
%{_includedir}/email-service/*.h
%{_libdir}/lib*.so
%{_libdir}/libemail-core-sound.so
%{_libdir}/libemail-core-sound.so.*
%{_libdir}/pkgconfig/*.pc
