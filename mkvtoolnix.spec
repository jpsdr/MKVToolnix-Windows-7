#
# spec file for RPM-based distributions
#

Name: mkvtoolnix
URL: https://mkvtoolnix.download/
Version: 18.0.0
Release: 1
Summary: Tools to create, alter and inspect Matroska files
Source: %{name}-%{version}.tar.xz
Requires: hicolor-icon-theme

BuildRequires: desktop-file-utils, fdupes, file-devel, flac, flac-devel, glibc-devel, libogg-devel, libstdc++-devel, libvorbis-devel, make, pkgconfig, zlib-devel, boost-devel >= 1.46.0, po4a

%if 0%{?centos}
BuildRequires: devtoolset-6-gcc-c++, rubygem-drake
%endif

%if 0%{?suse_version}
BuildRequires: gettext-tools libqt5-qtbase-devel, libqt5-qtmultimedia-devel, ruby2.1-rubygem-rake, libxslt-tools, docbook-xsl-stylesheets, googletest-devel
%else
BuildRequires: gettext-devel, qt5-qtbase-devel, qt5-qtmultimedia-devel, libxslt, docbook-style-xsl, gtest-devel
%endif

%if 0%{?suse_version}
BuildRequires: gcc7-c++
%endif

%if 0%{?fedora}
BuildRequires: gcc-c++ >= 4.9.0, rubypick, pugixml-devel, rubygem-drake, json-devel >= 2
%endif

%if 0%{?suse_version}
Group: Productivity/Multimedia/Other
License: GPL-2.0
%else
Group: Applications/Multimedia
License: GPLv2
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

export CFLAGS="%{optflags}"
export CXXFLAGS="%{optflags}"

%if 0%{?centos}
export CC=/opt/rh/devtoolset-6/root/bin/gcc
export CXX=/opt/rh/devtoolset-6/root/bin/g++
%endif

%if 0%{?suse_version}
export CC=/usr/bin/gcc-7
export CXX=/usr/bin/g++-7
%endif

%configure \
  --enable-debug \
  --disable-optimization

%build
%if 0%{?suse_version}
rake
%else
drake
%endif

%check
%if 0%{?suse_version}
rake tests:run_unit
%else
drake tests:run_unit
%endif

%install
%if 0%{?suse_version}
rake DESTDIR=$RPM_BUILD_ROOT install
strip ${RPM_BUILD_ROOT}/usr/bin/*
%else
drake DESTDIR=$RPM_BUILD_ROOT install
%endif

for f in mkvtoolnix-gui mkvinfo; do
  desktop-file-validate %{buildroot}%{_datadir}/applications/org.bunkus.$f.desktop
done

%fdupes -s %buildroot/%_mandir
%fdupes -s %buildroot/%_prefix

%post
update-desktop-database                     &> /dev/null || true
touch --no-create %{_datadir}/icons/hicolor &> /dev/null || true
touch --no-create %{_datadir}/mime/packages &> /dev/null || true

%postun
update-desktop-database &>/dev/null || true
if [ $1 -eq 0 ]; then
  touch --no-create %{_datadir}/icons/hicolor         &> /dev/null || true
  gtk-update-icon-cache %{_datadir}/icons/hicolor     &> /dev/null || true
  touch --no-create %{_datadir}/mime/packages         &> /dev/null || true
  update-mime-database %{?fedora:-n} %{_datadir}/mime &> /dev/null || true
fi

%posttrans
gtk-update-icon-cache %{_datadir}/icons/hicolor     &> /dev/null || true
update-mime-database %{?fedora:-n} %{_datadir}/mime &> /dev/null || true

%files
%defattr (-,root,root)
%doc AUTHORS COPYING README.md NEWS.md
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
%lang(ro) %{_datadir}/locale/ro/*/*.mo
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
%{_datadir}/mkvtoolnix

%changelog -n mkvtoolnix
* Sat Nov 18 2017 Moritz Bunkus <moritz@bunkus.org> 18.0.0-1
- New version

* Sat Oct 14 2017 Moritz Bunkus <moritz@bunkus.org> 17.0.0-1
- New version

* Sat Sep 30 2017 Moritz Bunkus <moritz@bunkus.org> 16.0.0-1
- New version

* Sat Aug 19 2017 Moritz Bunkus <moritz@bunkus.org> 15.0.0-1
- New version

* Sun Jul 23 2017 Moritz Bunkus <moritz@bunkus.org> 14.0.0-1
- New version

* Sun Jun 25 2017 Moritz Bunkus <moritz@bunkus.org> 13.0.0-1
- New version

* Sat May 20 2017 Moritz Bunkus <moritz@bunkus.org> 12.0.0-1
- New version

* Sat Apr 22 2017 Moritz Bunkus <moritz@bunkus.org> 11.0.0-1
- New version

* Sat Mar 25 2017 Moritz Bunkus <moritz@bunkus.org> 10.0.0-1
- New version

* Sun Feb 19 2017 Moritz Bunkus <moritz@bunkus.org> 9.9.0-1
- New version

* Sun Jan 22 2017 Moritz Bunkus <moritz@bunkus.org> 9.8.0-1
- New version

* Tue Dec 27 2016 Moritz Bunkus <moritz@bunkus.org> 9.7.1-1
- New version

* Tue Dec 27 2016 Moritz Bunkus <moritz@bunkus.org> 9.7.0-1
- New version

* Tue Nov 29 2016 Moritz Bunkus <moritz@bunkus.org> 9.6.0-1
- New version

* Sun Oct 16 2016 Moritz Bunkus <moritz@bunkus.org> 9.5.0-1
- New version

* Sun Sep 11 2016 Moritz Bunkus <moritz@bunkus.org> 9.4.2-1
- New version

* Sun Sep 11 2016 Moritz Bunkus <moritz@bunkus.org> 9.4.1-1
- New version

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
