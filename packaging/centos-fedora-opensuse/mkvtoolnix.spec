#
# spec file for RPM-based distributions
#

Name: mkvtoolnix
URL: https://mkvtoolnix.download/
Version: 92.0
Release: 1
Summary: Tools to create, alter and inspect Matroska files
Source0: %{name}-%{version}.tar.xz
Group: Applications/Multimedia
License: GPLv2

BuildRequires: cmark-devel
BuildRequires: desktop-file-utils
BuildRequires: docbook-style-xsl
BuildRequires: fdupes
BuildRequires: flac
BuildRequires: flac-devel
BuildRequires: fmt-devel
BuildRequires: gettext-devel
BuildRequires: glibc-devel
BuildRequires: gmp-devel
BuildRequires: gtest-devel
BuildRequires: libdvdread-devel
BuildRequires: libogg-devel
BuildRequires: libstdc++-devel
BuildRequires: libvorbis-devel
BuildRequires: libxslt
BuildRequires: make
BuildRequires: pkgconfig
BuildRequires: po4a
BuildRequires: zlib-devel

%if 0%{?rhel}
# RHEL, CentOS, AlmaLinux, RockyLinux

%if 0%{?rhel} == 9
BuildRequires: gcc-c++
BuildRequires: qt6-qtbase-devel
BuildRequires: qt6-qtmultimedia-devel
BuildRequires: qt6-qtsvg
%endif
%endif

%if 0%{?fedora}
BuildRequires: boost-devel
BuildRequires: gcc-c++
BuildRequires: json-devel >= 2
BuildRequires: libappstream-glib
BuildRequires: qt6-qtbase-devel
BuildRequires: qt6-qtmultimedia-devel
BuildRequires: qt6-qtsvg
BuildRequires: qt6-qtsvg-devel
BuildRequires: rubypick
BuildRequires: utf8cpp-devel
%endif

%description
Mkvtoolnix is a set of utilities to mux and demux audio, video and subtitle
streams into and from Matroska containers.


%package gui
Summary: Qt-based graphical interface for Matroska container manipulation
Requires: %{name} = %{version}-%{release}
Requires: hicolor-icon-theme

%description gui
Mkvtoolnix is a set of utilities to mux and demux audio, video and subtitle
streams into and from Matroska containers.

This package contains the Qt graphical interface for these utilities.

Requires: hicolor-icon-theme

%prep
# %{gpgverify} --keyring='%{S:2}' --signature='%{S:1}' --data='%{S:0}'
%setup

export CFLAGS="%{optflags}"
export CXXFLAGS="%{optflags}"
unset CONFIGURE_ARGS

%if 0%{?rhel} && 0%{?rhel} <= 7
export CPPFLAGS="${CPPFLAGS} -I/usr/include/boost169"
export CONFIGURE_ARGS="--with-boost-libdir=/usr/lib64/boost169"
%endif
for SUB_DIR in gcc-toolset-11 gcc-toolset 10 devtoolset-10; do
  if test -x /opt/rh/$SUB_DIR/root/bin/gcc; then
    source /opt/rh/$SUB_DIR/enable
    break
  fi
done

%build
%configure \
  --disable-optimization \
  --enable-debug \
  --with-tools \
  "$CONFIGURE_ARGS"
# if [[ -f ~/mtx-rpm-compiled.tar.gz ]]; then
#   tar xzf ~/mtx-rpm-compiled.tar.gz
# fi

./drake apps
./drake tools
./drake

# tar czf ~/mtx-rpm-compiled-$(date '+%%Y%%m%%d%%H%%M%%S').tar .

%check
./drake tests:run_unit

%install
./drake DESTDIR=$RPM_BUILD_ROOT TOOLS=1 install
desktop-file-validate %{buildroot}%{_datadir}/applications/org.bunkus.mkvtoolnix-gui.desktop
if test `lsb_release -is` = Fedora; then
  appstream-util validate-relax --nonet %{buildroot}%{_metainfodir}/org.bunkus.mkvtoolnix-gui.appdata.xml
fi

install -pm 755 src/tools/{bluray_dump,ebml_validator,hevcc_dump,xyzvc_dump} $RPM_BUILD_ROOT%{_bindir}

%find_lang %{name}
%find_lang mkvextract --with-man
%find_lang mkvmerge --with-man
%find_lang mkvpropedit --with-man
%find_lang mkvinfo --with-man
cat mkv{extract,info,merge,propedit}.lang >> mkvtoolnix.lang

%find_lang mkvtoolnix-gui --with-man

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


%files -f %{name}.lang
%license COPYING
%doc AUTHORS README.md NEWS.md
%{_bindir}/bluray_dump
%{_bindir}/ebml_validator
%{_bindir}/hevcc_dump
%{_bindir}/mkvextract
%{_bindir}/mkvinfo
%{_bindir}/mkvmerge
%{_bindir}/mkvpropedit
%{_bindir}/xyzvc_dump
%{_mandir}/man1/mkvextract.1*
%{_mandir}/man1/mkvinfo.1*
%{_mandir}/man1/mkvmerge.1*
%{_mandir}/man1/mkvpropedit.1*

%files gui -f mkvtoolnix-gui.lang
%{_bindir}/mkvtoolnix-gui
%{_mandir}/man1/mkvtoolnix-gui.1*
%{_datadir}/applications/org.bunkus.mkvtoolnix-gui.desktop
%{_metainfodir}/org.bunkus.mkvtoolnix-gui.appdata.xml
%{_datadir}/icons/hicolor/*/apps/*.png
%{_datadir}/mime/packages/org.bunkus.mkvtoolnix-gui.xml
%{_datadir}/mkvtoolnix

%changelog -n mkvtoolnix
* Sat Apr 26 2025 Moritz Bunkus <moritz@bunkus.org> 92.0-1
- New version

* Sun Mar 16 2025 Moritz Bunkus <moritz@bunkus.org> 91.0-1
- New version

* Sat Feb  8 2025 Moritz Bunkus <moritz@bunkus.org> 90.0-1
- New version

* Fri Dec 27 2024 Moritz Bunkus <moritz@bunkus.org> 89.0-1
- New version

* Sat Oct 19 2024 Moritz Bunkus <moritz@bunkus.org> 88.0-1
- New version

* Sat Sep  7 2024 Moritz Bunkus <moritz@bunkus.org> 87.0-1
- New version

* Sat Jul 13 2024 Moritz Bunkus <moritz@bunkus.org> 86.0-1
- New version

* Sun Jun  2 2024 Moritz Bunkus <moritz@bunkus.org> 85.0-1
- New version

* Sun Apr 28 2024 Moritz Bunkus <moritz@bunkus.org> 84.0-1
- New version

* Sun Mar 10 2024 Moritz Bunkus <moritz@bunkus.org> 83.0-1
- New version

* Tue Jan  2 2024 Moritz Bunkus <moritz@bunkus.org> 82.0-1
- New version

* Sun Dec 10 2023 Moritz Bunkus <moritz@bunkus.org> 81.0-2
- Removed support for AlmaLinux/RockyLinux/CentOS <= 8 due to support
  for Qt 5 being dropped.

* Sat Dec  2 2023 Moritz Bunkus <moritz@bunkus.org> 81.0-1
- New version

* Sun Oct 29 2023 Moritz Bunkus <moritz@bunkus.org> 80.0-1
- New version

* Sun Aug 20 2023 Moritz Bunkus <moritz@bunkus.org> 79.0-1
- New version

* Sun Jul  2 2023 Moritz Bunkus <moritz@bunkus.org> 78.0-1
- New version

* Sun Jun  4 2023 Moritz Bunkus <moritz@bunkus.org> 77.0-1
- New version

* Sun Apr 30 2023 Moritz Bunkus <moritz@bunkus.org> 76.0-1
- New version

* Sun Mar 26 2023 Moritz Bunkus <moritz@bunkus.org> 75.0.0-1
- New version

* Sun Feb 12 2023 Moritz Bunkus <moritz@bunkus.org> 74.0.0-1
- New version

* Mon Jan  2 2023 Moritz Bunkus <moritz@bunkus.org> 73.0.0-1
- New version

* Sun Nov 13 2022 Moritz Bunkus <moritz@bunkus.org> 72.0.0-1
- New version

* Sun Oct  9 2022 Moritz Bunkus <moritz@bunkus.org> 71.1.0-1
- New version

* Sat Oct  8 2022 Moritz Bunkus <moritz@bunkus.org> 71.0.0-1
- New version

* Sun Aug 14 2022 Moritz Bunkus <moritz@bunkus.org> 70.0.0-1
- New version

* Sat Jul  9 2022 Moritz Bunkus <moritz@bunkus.org> 69.0.0-1
- New version

* Sun May 22 2022 Moritz Bunkus <moritz@bunkus.org> 68.0.0-1
- New version

* Sun Apr 10 2022 Moritz Bunkus <moritz@bunkus.org> 67.0.0-1
- New version

* Sun Mar 13 2022 Moritz Bunkus <moritz@bunkus.org> 66.0.0-1
- New version

* Sun Feb  6 2022 Moritz Bunkus <moritz@bunkus.org> 65.0.0-1
- New version

* Mon Dec 27 2021 Moritz Bunkus <moritz@bunkus.org> 64.0.0-1
- New version

* Sun Nov 14 2021 Moritz Bunkus <moritz@bunkus.org> 63.0.0-1
- New version

* Sun Oct 10 2021 Moritz Bunkus <moritz@bunkus.org> 62.0.0-1
- New version

* Mon Aug 30 2021 Moritz Bunkus <moritz@bunkus.org> 61.0.0-1
- New version

* Sat Jul 31 2021 Moritz Bunkus <moritz@bunkus.org> 60.0.0-1
- New version

* Sat Jul 10 2021 Moritz Bunkus <moritz@bunkus.org> 59.0.0-1
- New version

* Sun Jun 13 2021 Moritz Bunkus <moritz@bunkus.org> 58.0.0-1
- New version

* Wed Jun 02 2021 Moritz Bunkus <moritz@bunkus.org> 57.0.0-1
- Split package into non-GUI (mkvtoolnix) & GUI (mkvtoolnix-gui)

* Sat May 22 2021 Moritz Bunkus <moritz@bunkus.org> 57.0.0-1
- New version

* Fri Apr  9 2021 Moritz Bunkus <moritz@bunkus.org> 56.1.0-1
- New version

* Mon Apr  5 2021 Moritz Bunkus <moritz@bunkus.org> 56.0.0-1
- New version

* Sat Mar  6 2021 Moritz Bunkus <moritz@bunkus.org> 55.0.0-1
- New version

* Fri Feb 26 2021 Moritz Bunkus <moritz@bunkus.org> 54.0.0-1
- New version

* Sat Jan 30 2021 Moritz Bunkus <moritz@bunkus.org> 53.0.0-1
- New version

* Mon Jan  4 2021 Moritz Bunkus <moritz@bunkus.org> 52.0.0-1
- New version

* Sun Oct  4 2020 Moritz Bunkus <moritz@bunkus.org> 51.0.0-1
- New version

* Sun Sep  6 2020 Moritz Bunkus <moritz@bunkus.org> 50.0.0-1
- New version

* Sun Aug  2 2020 Moritz Bunkus <moritz@bunkus.org> 49.0.0-1
- New version

* Sat Jun 27 2020 Moritz Bunkus <moritz@bunkus.org> 48.0.0-1
- New version

* Sat May 30 2020 Moritz Bunkus <moritz@bunkus.org> 47.0.0-1
- New version

* Fri May  1 2020 Moritz Bunkus <moritz@bunkus.org> 46.0.0-1
- New version

* Sat Apr  4 2020 Moritz Bunkus <moritz@bunkus.org> 45.0.0-1
- New version

* Sun Mar  8 2020 Moritz Bunkus <moritz@bunkus.org> 44.0.0-1
- New version

* Sun Jan 26 2020 Moritz Bunkus <moritz@bunkus.org> 43.0.0-1
- New version

* Thu Jan  2 2020 Moritz Bunkus <moritz@bunkus.org> 42.0.0-1
- New version

* Fri Dec  6 2019 Moritz Bunkus <moritz@bunkus.org> 41.0.0-1
- New version

* Sat Nov  9 2019 Moritz Bunkus <moritz@bunkus.org> 40.0.0-1
- New version

* Mon Nov  4 2019 Moritz Bunkus <moritz@bunkus.org> 39.0.0-1
- New version

* Sun Oct  6 2019 Moritz Bunkus <moritz@bunkus.org> 38.0.0-1
- New version

* Sat Aug 24 2019 Moritz Bunkus <moritz@bunkus.org> 37.0.0-1
- New version

* Sat Aug 10 2019 Moritz Bunkus <moritz@bunkus.org> 36.0.0-1
- New version

* Sat Jun 22 2019 Moritz Bunkus <moritz@bunkus.org> 35.0.0-1
- New version

* Sat May 18 2019 Moritz Bunkus <moritz@bunkus.org> 34.0.0-1
- New version

* Sun Apr 14 2019 Moritz Bunkus <moritz@bunkus.org> 33.1.0-1
- New version

* Fri Apr 12 2019 Moritz Bunkus <moritz@bunkus.org> 33.0.0-1
- New version

* Tue Mar 12 2019 Moritz Bunkus <moritz@bunkus.org> 32.0.0-1
- New version

* Sat Feb  9 2019 Moritz Bunkus <moritz@bunkus.org> 31.0.0-1
- New version

* Sat Jan  5 2019 Moritz Bunkus <moritz@bunkus.org> 30.1.0-1
- New version

* Fri Jan  4 2019 Moritz Bunkus <moritz@bunkus.org> 30.0.0-1
- New version

* Sat Dec  1 2018 Moritz Bunkus <moritz@bunkus.org> 29.0.0-1
- New version

* Thu Oct 25 2018 Moritz Bunkus <moritz@bunkus.org> 28.2.0-1
- New version

* Tue Oct 23 2018 Moritz Bunkus <moritz@bunkus.org> 28.1.0-1
- New version

* Sat Oct 20 2018 Moritz Bunkus <moritz@bunkus.org> 28.0.0-1
- New version

* Wed Sep 26 2018 Moritz Bunkus <moritz@bunkus.org> 27.0.0-1
- New version

* Sun Aug 26 2018 Moritz Bunkus <moritz@bunkus.org> 26.0.0-1
- New version

* Thu Jul 12 2018 Moritz Bunkus <moritz@bunkus.org> 25.0.0-1
- New version

* Sun Jun 10 2018 Moritz Bunkus <moritz@bunkus.org> 24.0.0-1
- New version

* Wed May  2 2018 Moritz Bunkus <moritz@bunkus.org> 23.0.0-1
- New version

* Wed May  2 2018 Moritz Bunkus <moritz@bunkus.org> 23.0.0-1
- New version

* Sun Apr  1 2018 Moritz Bunkus <moritz@bunkus.org> 22.0.0-1
- New version

* Sat Feb 24 2018 Moritz Bunkus <moritz@bunkus.org> 21.0.0-1
- New version

* Mon Jan 15 2018 Moritz Bunkus <moritz@bunkus.org> 20.0.0-1
- New version

* Sun Dec 17 2017 Moritz Bunkus <moritz@bunkus.org> 19.0.0-1
- New version

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
