%define _libname libccaudio2-1_0-0
%define _devname libccaudio2-devel

Name: ccaudio2
Summary: "ccaudio2" - portable C++ class framework for processing audio files.
Version: 1.0.0
Release: 1
Epoch: 0
License: LGPL v3 or later 
Group: Development/Libraries
URL: http://www.gnu.org/software/ccaudio2
Source0: ftp://ftp.gnu.org/gnu/ccaudio2/ccaudio2-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root 
BuildRequires: gcc-c++ gsm-devel 
BuildRequires: speex-devel >= 1.0.5
BuildRequires: doxygen, info

%description
The GNU ccAudio package offers a highly portable C++ class framework for
developing applications which manipulate audio streams and various
disk based audio file formats.  At the moment ccaudio is primarly a class
framework for handling .au, .wav (RIFF), and various .raw audio encoding
formats under Posix and win32 systems, though it may expand to become a
general purpose audio and soundcard support library.  Support for
controlling CD audio devices has recently been added as well as support
for codecs and other generic audio processing services.  

%package -n %{_libname}
Group: System/Libraries
Summary: Runtime libraries for GNU ccAudio
Provides: %{name} = %{version}-%{release}

%package -n %{_devname}
Group: Development/Libraries
Summary: Headers and static link library for ccaudio2.
Requires: %{_libname} = %{version}
Provides: %{name}-devel = %{version}-%{release}

%description -n %{_libname}
This package contains the runtime library needed by applications that use 
GNU ccAudio.

%description -n %{_devname}
This package provides the header files and documentation for building
applications that use GNU ccAudio.  

%prep
%setup
%build
%configure CXXFLAGS="$RPM_OPT_FLAGS"
%{__make} %{?_smp_mflags}

%install

%makeinstall
%{__strip} %{buildroot}/%{_libdir}/lib*.so.*.*
%{__strip} %{buildroot}/%{_libdir}/ccaudio2*/*.so
%{__strip} %{buildroot}/%{_bindir}/*

%clean
%{__rm} -rf %{buildroot}

%files -n %{_libname}
%defattr(-,root,root,-)
%doc AUTHORS COPYING COPYING.LESSER ChangeLog NEWS README TODO
%dir %{_libdir}/ccaudio2*
%{_libdir}/*.so.*
%{_libdir}/ccaudio2*/*
%{_bindir}/*
%{_mandir}/*/*

%files -n %{_devname}
%defattr(-,root,root,-)
%{_libdir}/*.a
%{_libdir}/*.so
%{_libdir}/*.la   
%{_libdir}/pkgconfig/*.pc
%{_includedir}/cc++/*.h

%post -n %{_libname} -p /sbin/ldconfig

%postun -n %{_libname} -p /sbin/ldconfig

%changelog
* Tue Jul 22 2008 - dyfet@gnutelephony.org
- initial spec file distribution.  
