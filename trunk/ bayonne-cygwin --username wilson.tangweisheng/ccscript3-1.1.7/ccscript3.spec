Summary: GNU ccScript3 - a C++ framework for threaded scripting engine.
Name: ccscript3
Version: 1.1.7
Release: 1
Epoch: 0
Copyright: LGPL
Group: Development/Libraries
URL: http://www.gnu.org/software/commoncpp/commoncpp.html
Source0: ftp://ftp.gnu.org/gnu/ccscript/ccscript3-%{PACKAGE_VERSION}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root 
Requires: commoncpp2 >= 1.3.1
Requires: unixODBC
BuildRequires: commoncpp2-devel >= 1.3.1
BuildRequires: pkgconfig
BuildRequires: libstdc++-devel
BuildRequires: unixODBC-devel

%package devel
Group: Development/Libraries
Summary: Headers and static link library for ccScript3.
Requires: %{name} = %{epoch}:%{version}-%{release} 
Requires: commoncpp2-devel >= 1.3.1

%description
GNU ccScript3 is a C++ class framework for creating a virtual machine
execution system for use with and as a scripting/assembler language for
state-transition driven realtime systems. 

%description devel
This package provides the header files, link libraries, and 
documentation for building applications that use GNU ccScript3. 

%prep
%setup
%build
%configure
make %{?_smp_mflags} LDFLAGS="-s" CXXFLAGS="$RPM_OPT_FLAGS"

%install

%makeinstall

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,0755)
%doc AUTHORS COPYING ChangeLog NEWS README TODO
%dir %{_libdir}/ccscript3*
%{_libdir}/*.so.*
%{_libdir}/ccscript3*/*

%files devel
%defattr(-,root,root,0755)
%{_libdir}/*.a
%{_libdir}/*.so
%{_libdir}/*.la   
%{_libdir}/pkgconfig/*.pc
%{_includedir}/cc++/*.h

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig  
