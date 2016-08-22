#
# spec file for RPM-based distributions
#

Name: mkvtoolnix
URL: https://mkvtoolnix.download/
Version: 9.4.0
Release: 1
Summary: Tools to create, alter and inspect Matroska files
Source: %{name}-%{version}.tar.xz

BuildRequires: fdupes, file-devel, flac, flac-devel, libcurl-devel, libogg-devel, libstdc++-devel, libvorbis-devel, make, pkgconfig

%if 0%{?centos} && 0%{?centos} < 7
BuildRequires: devtoolset-1.1-gcc-c++ >= 4.6.3
%else
BuildRequires: boost-devel >= 1.46.0, gcc-c++ >= 4.6.3, ruby >= 1.9
%endif

%if 0%{?suse_version}
Group: Productivity/Multimedia/Other
License: GPL-2.0
BuildRequires: gettext-tools libqt5-qtbase-devel
%endif

%if 0%{?fedora} || 0%{?rhel} || 0%{?centos}
Group: Applications/Multimedia
License: GPLv2
BuildRequires: qt5-qtbase-devel, gettext-devel

%if 0%{?fedora}
BuildRequires: rubypick, pugixml-devel
%endif

%endif

%description
Tools to create and manipulate Matroska files (extensions .mkv and
.mka), a new container format for audio and video files. Includes
command line tools mkvextract, mkvinfo, mkvmerge, mkvpropedit and a
graphical frontend for them, mkvmerge-gui.

Authors:
--------
    Moritz Bunkus <moritz@bunkus.org>

%prep
%setup

export CFLAGS="$RPM_OPT_FLAGS"
export CXXFLAGS="$RPM_OPT_FLAGS"

%if 0%{?centos} && 0%{?centos} < 7
export CC=/opt/centos/devtoolset-1.1/root/usr/bin/gcc
export CXX=/opt/centos/devtoolset-1.1/root/usr/bin/g++
export EXTRA_CONFIGURE_ARGS="--with-boost=/opt/boost"
%endif

%configure --enable-debug --enable-optimization $EXTRA_CONFIGURE_ARGS

%build
./drake

%install
./drake DESTDIR=$RPM_BUILD_ROOT install
%fdupes -s %buildroot/%_mandir
%fdupes -s %buildroot/%_prefix

%files
%defattr (-,root,root)
%doc AUTHORS COPYING README.md ChangeLog
%{_bindir}/*
%{_datadir}/applications/*.desktop
%{_datadir}/icons/hicolor/*/*/*.png
%{_datadir}/mime/packages/*.xml
%lang(ca) %{_datadir}/locale/ca/*/*.mo
%lang(cs) %{_datadir}/locale/cs/*/*.mo
%lang(de) %{_datadir}/locale/de/*/*.mo
%lang(es) %{_datadir}/locale/es/*/*.mo
%lang(eu) %{_datadir}/locale/eu/*/*.mo
%lang(fr) %{_datadir}/locale/fr/*/*.mo
%lang(it) %{_datadir}/locale/it/*/*.mo
%lang(ja) %{_datadir}/locale/ja/*/*.mo
%lang(ko) %{_datadir}/locale/ko/*/*.mo
%lang(lt) %{_datadir}/locale/lt/*/*.mo
%lang(nl) %{_datadir}/locale/nl/*/*.mo
%lang(pl) %{_datadir}/locale/pl/*/*.mo
%lang(pt) %{_datadir}/locale/pt/*/*.mo
%lang(pt_BR) %{_datadir}/locale/pt_BR/*/*.mo
%lang(ru) %{_datadir}/locale/ru/*/*.mo
%lang(sr_RS) %{_datadir}/locale/sr_RS/*/*.mo
%lang(sr_RS@latin) %{_datadir}/locale/sr_RS@latin/*/*.mo
%lang(sv) %{_datadir}/locale/sv/*/*.mo
%lang(tr) %{_datadir}/locale/tr/*/*.mo
%lang(uk) %{_datadir}/locale/uk/*/*.mo
%lang(zh_CN) %{_datadir}/locale/zh_CN/*/*.mo
%lang(zh_TW) %{_datadir}/locale/zh_TW/*/*.mo
%{_datadir}/man/man1/*
%{_datadir}/man/ca
%{_datadir}/man/de
%{_datadir}/man/es
%{_datadir}/man/ja
%{_datadir}/man/ko
%{_datadir}/man/nl
%{_datadir}/man/pl
%{_datadir}/man/uk
%{_datadir}/man/zh_CN

%changelog -n mkvtoolnix
* Mon Aug 22 2016 Moritz Bunkus <moritz@bunkus.org> 9.4.0-1
- New version

* Thu Jul 14 2016 Moritz Bunkus <moritz@bunkus.org> 9.3.1-1
- New version

* Wed Jul 13 2016 Moritz Bunkus <moritz@bunkus.org> 9.3.0-1
- New version

* Sat May 28 2016 Moritz Bunkus <moritz@bunkus.org> 9.2.0-1
- New version

* Sat Apr 23 2016 Moritz Bunkus <moritz@bunkus.org> 9.1.0-1
- New version

* Mon Mar 28 2016 Moritz Bunkus <moritz@bunkus.org> 9.0.1-1
- New version

* Sat Mar 26 2016 Moritz Bunkus <moritz@bunkus.org> 9.0.0-1
- New version

* Sun Feb 21 2016 Moritz Bunkus <moritz@bunkus.org> 8.9.0-1
- New version

* Sun Jan 10 2016 Moritz Bunkus <moritz@bunkus.org> 8.8.0-1
- New version

* Thu Dec 31 2015 Moritz Bunkus <moritz@bunkus.org> 8.7.0-1
- New version

* Sun Nov 29 2015 Moritz Bunkus <moritz@bunkus.org> 8.6.1-1
- New version

* Sat Nov 28 2015 Moritz Bunkus <moritz@bunkus.org> 8.6.0-1
- New version

* Wed Nov  4 2015 Moritz Bunkus <moritz@bunkus.org> 8.5.2-1
- New version

* Wed Oct 21 2015 Moritz Bunkus <moritz@bunkus.org> 8.5.1-1
- New version

* Sat Oct 17 2015 Moritz Bunkus <moritz@bunkus.org> 8.5.0-1
- New version

* Sat Sep 19 2015 Moritz Bunkus <moritz@bunkus.org> 8.4.0-1
- New version

* Sat Aug 15 2015 Moritz Bunkus <moritz@bunkus.org> 8.3.0-1
- Removed support for wxWidgets-based GUIs

* Sat May  9 2015 Moritz Bunkus <moritz@bunkus.org> 7.8.0-1
- Add support for the Qt-based GUIs

* Sat Nov 15 2014 Moritz Bunkus <moritz@bunkus.org> 7.3.0-1
- Serious reorganization & fixes for rpmlint complaints
