# Copyright (c) 2008, 2009 David Sugar, Tycho Softworks.
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

Name: ucommon
Summary: Portable C++ runtime for threads and sockets
Version: 2.1.2
Release: 0%{?dist}
License: LGPLv3+
URL: http://www.gnu.org/software/commoncpp
Source0: http://www.gnutelephony.org/dist/tarballs/ucommon-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: doxygen graphviz-gd 
Group: System Environment/Libraries
Summary: Runtime library for portable C++ threading and sockets

%description
uCommon is a lightweight C++ library to facilitate using C++ design patterns
even for very deeply embedded applications, such as for systems using uClibc
along with POSIX threading support. For this reason, UCommon disables language
features that consume memory or introduce runtime overhead. UCommon introduces
some design patterns from Objective-C, such as reference counted objects,
memory pools, and smart pointers. UCommon introduces some new concepts for
handling of thread locking and synchronization.

%package devel
Requires: %{name} = %{version}-%{release}
Requires: pkgconfig
Group: Development/Libraries
Summary: Headers for building ucommon applications

%package doc
Group: Documentation
Summary: Generated class documentation for ucommon

%description devel
This package provides header and support files needed for building 
applications that use the uCommon library.

%description doc
Generated class documentation for GNU uCommon library from header files, 
html browsable.

%prep
%setup -q
%build

%configure --disable-static
%{__make} %{?_smp_mflags}
%{__rm} -rf doc/html
%{__make} doxy

%install
%{__rm} -rf %{buildroot}
%{__make} DESTDIR=%{buildroot} INSTALL="install -p" install
%{__chmod} 0755 %{buildroot}%{_bindir}/ucommon-config
%{__rm} %{buildroot}%{_libdir}/*.la

%clean
%{__rm} -rf %{buildroot}

%files 
%defattr(-,root,root,-)
%doc AUTHORS README COPYING COPYING.LESSER NEWS SUPPORT ChangeLog
%{_libdir}/*.so.*

%files devel
%defattr(-,root,root,-)
%{_libdir}/*.so
%{_includedir}/ucommon/
%{_libdir}/pkgconfig/*.pc
%{_bindir}/ucommon-config
%{_mandir}/man1/ucommon-config.*

%files doc
%defattr(-,root,root,-)
%doc doc/html/*

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%changelog
* Sun Jul 19 2009 - David Sugar <dyfet@gnutelephony.org> - 2.0.6-1
- fixed ucommon-config upstream.

* Tue May 05 2009 - David Sugar <dyfet@gnutelephony.org> - 2.0.5-4
- removed static libraries, fixed other build issues (#498736)

* Sun May 03 2009 - David Sugar <dyfet@gnutelephony.org> - 2.0.5-3
- spec file prepared for redhat/fedora (#498736)

