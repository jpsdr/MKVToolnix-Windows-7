#!/bin/zsh

set -e
set -x

export SCRIPT_PATH=${0:a:h}
source ${SCRIPT_PATH}/config.sh

if type -p drake &> /dev/null; then
  RAKE=drake
else
  RAKE=rake
fi

function fail {
  echo $@
  exit 1
}

function build_tarball {
  local package=${PWD:t}
  if [[ -n $SHARED ]] package="${package}-shared"
  $DEBUG ${SCRIPT_PATH}/myinstall.sh build package $package $@
}

function build_package {
  local FILE=$1
  shift
  local PACKAGE=${${FILE%.*}%.tar}
  local DIR=${DIR:-$PACKAGE}

  case ${FILE##*.} in
    xz|lzma) COMPRESSION=J ;;
    bz2)     COMPRESSION=j ;;
    gz)      COMPRESSION=z ;;
    tar)     COMPRESSION=  ;;
    *)       echo Unknown compression for ${FILE} ; exit 1 ;;
  esac

  cd $CMPL
  if [[ -z $NO_EXTRACTION ]]; then
    $DEBUG rm -rf ${DIR}
    $DEBUG tar x${COMPRESSION}f ${SRCDIR}/${FILE}
  fi

  $DEBUG cd ${DIR}

  if [[ -z $NO_CONFIGURE ]]; then
    $DEBUG ${CONFIGURE:-./configure} $@

    if [[ -z $NO_MAKE ]]; then
      $DEBUG make
      build_tarball
    fi
  fi
}

mkdir -p $CMPL

function build_autoconf {
  build_package autoconf-2.69.tar.xz --prefix=${TARGET}
}

function build_automake {
  build_package automake-1.14.1.tar.gz --prefix=${TARGET}
}

function build_pkgconfig {
  build_package pkg-config-0.28.tar.gz --prefix=${TARGET} \
     --with-pc-path=${TARGET}/lib/pkgconfig --with-internal-glib \
     --enable-static --enable-shared=no
}

function build_ogg {
  build_package libogg-1.3.2.tar.gz --prefix=${TARGET} \
    --disable-shared --enable-static
}

function build_vorbis {
  build_package libvorbis-1.3.5.tar.gz --prefix=${TARGET} \
    --with-ogg-libraries=${TARGET}/lib --with-ogg-includes=${TARGET}/include/ \
    --enable-static --disable-shared
}

function build_flac {
  build_package flac-1.3.1.tar.xz --prefix=${TARGET} \
    --disable-asm-optimizations --disable-xmms-plugin \
    --with-ogg-libraries=${TARGET}/lib --with-ogg-includes=${TARGET}/include/ \
    --enable-static --disable-shared
}

function build_zlib {
  build_package zlib-1.2.8.tar.xz --prefix=${TARGET} --static
}

function build_gettext {
  NO_MAKE=1 build_package gettext-0.19.8.1.tar.gz --prefix=${TARGET} \
    --without-libexpat-prefix \
    --without-libxml2-prefix \
    --without-emacs \
    --enable-static --disable-shared

  make -C gettext-runtime/intl install
  make -C gettext-tools install
}

function build_ruby {
  build_package ruby-2.1.6.tar.bz2 --prefix=${TARGET} \
    --enable-static --disable-shared
}

function build_boost {
  local -a args properties

  args=(--reconfigure -sICONV_PATH=/usr -j$DRAKETHREADS --prefix=TMPDIR/${TARGET} --libdir=TMPDIR/${TARGET}/lib)
  properties=(toolset=clang link=static variant=release)
  if [[ -n $CXXFLAGS ]] properties+=(cxxflags="${(q)CXXFLAGS}")
  if [[ -n $LDFLAGS  ]] properties+=(linkflags="${(q)LDFLAGS}")

  NO_MAKE=1 CONFIGURE=./bootstrap.sh build_package boost_1_60_0.tar.bz2 \
    --with-toolset=clang
  build_tarball command "./b2 ${args} ${properties} install"
}

function build_qtbase {
  local -a args
  args=(--prefix=${TARGET} -opensource -confirm-license -release
        -c++std c++14
        -force-pkg-config -pkg-config -nomake examples -nomake tests
        -no-glib -no-dbus  -no-sql-mysql -no-sql-sqlite -no-sql-odbc -no-sql-psql -no-sql-tds
        -no-openssl -no-cups -no-feature-cups
        # -no-feature-printer
        -no-feature-printpreviewwidget -no-feature-printdialog -no-feature-printpreviewdialog)
  args+=(-no-framework)
  if [[ -z $SHARED_QT ]] args+=(-static)

  local package=qtbase-opensource-src-${QTVER}
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  NO_CONFIGURE=1 build_package ${package}.tar.xz

  if [[ $QTVER == 5.7.0 ]] patch -p2 < ${SCRIPT_PATH}/qt-patches/002-xcrun-xcode-8.patch
  $DEBUG ./configure ${args}

  # find . -name Makefile| xargs perl -pi -e 's{-fvisibility=hidden|-fvisibility-inlines-hidden}{}g'

  $DEBUG make

  # cd ${CMPL}/${package}
  build_tarball command "make INSTALL_ROOT=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qttools {
  local -a tools to_install
  tools=(linguist/lrelease linguist/lconvert linguist/lupdate macdeployqt)
  to_install=()

  local package=qttools-opensource-src-${QTVER}
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package ${package}.tar.xz

  for tool ($tools) {
    to_install+=($PWD/bin/${tool##*/})
    pushd src/$tool
    qmake
    make
    popd
  }

  # cd ${CMPL}/${package}
  build_tarball command "mkdir -p TMPDIR/${TARGET}/bin && cp -v $to_install TMPDIR/${TARGET}/bin/"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qttranslations {
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package qttranslations-opensource-src-${QTVER}.tar.xz
  $DEBUG make
  build_tarball command "make INSTALL_ROOT=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qtmacextras {
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package qtmacextras-opensource-src-${QTVER}.tar.xz
  $DEBUG make
  build_tarball command "make INSTALL_ROOT=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qtmultimedia {
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package qtmultimedia-opensource-src-${QTVER}.tar.xz
  $DEBUG make
  build_tarball command "make INSTALL_ROOT=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qtsvg {
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package qtsvg-opensource-src-${QTVER}.tar.xz
  $DEBUG make
  build_tarball command "make INSTALL_ROOT=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qtimageformats {
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package qtimageformats-opensource-src-${QTVER}.tar.xz
  $DEBUG make
  build_tarball command "make INSTALL_ROOT=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qt {
  build_qtbase
  build_qtmultimedia
  build_qtsvg
  build_qtimageformats
  build_qttools
  build_qttranslations
}

function build_configured_mkvtoolnix {
  if [[ -z ${MTX_VER} ]] fail Variable MTX_VER not set

  dmgbase=${CMPL}/dmg-${MTX_VER}
  dmgcnt=$dmgbase/MKVToolNix-${MTX_VER}.app/Contents
  dmgmac=$dmgcnt/MacOS

  local -a args
  args=(
    --prefix=$dmgmac --bindir=$dmgmac --datarootdir=$dmgmac
    --with-extra-libs=${TARGET}/lib --with-extra-includes=${TARGET}/include
    --with-boost-libdir=${TARGET}/lib
    --with-docbook-xsl-root=${DOCBOOK_XSL_ROOT_DIR}
    --disable-debug
  )

  if [[ -z $SHARED_QT ]] args+=(--enable-static-qt)

  ./configure \
    LDFLAGS="$LDFLAGS -framework CoreFoundation -headerpad_max_install_names" \
    CXXFLAGS="-fvisibility=hidden -fvisibility-inlines-hidden" \
    ${args}

  grep -q 'USE_QT.*yes' build-config
}

function build_mkvtoolnix {
  if [[ -z ${MTX_VER} ]] fail Variable MTX_VER not set

  dmgbase=${CMPL}/dmg-${MTX_VER}
  dmgcnt=$dmgbase/MKVToolNix-${MTX_VER}.app/Contents
  dmgmac=$dmgcnt/MacOS

  NO_MAKE=1 NO_CONFIGURE=1 build_package mkvtoolnix-${MTX_VER}.tar.xz
  build_configured_mkvtoolnix

  ${RAKE} clean
  ${RAKE} -j ${DRAKETHREADS}
}

function build_dmg {
  if [[ -z ${MTX_VER} ]] fail Variable MTX_VER not set

  if [[ -f tools/macos/unlock_keychain.sh ]] tools/macos/unlock_keychain.sh

  dmgbase=${CMPL}/dmg-${MTX_VER}
  dmgapp=$dmgbase/MKVToolNix-${MTX_VER}.app
  dmgcnt=$dmgapp/Contents
  dmgmac=$dmgcnt/MacOS
  latest_link=${CMPL}/latest

  rm -f ${latest_link}

  if [[ -z $DMG_NO_CD ]] cd ${CMPL}/mkvtoolnix-${MTX_VER}

  rm -rf $dmgbase
  ${RAKE} install prefix=${dmgcnt}
  test -f ${dmgmac}/mkvtoolnix-gui

  strip ${dmgcnt}/MacOS/mkv{merge,info,info-gui,extract,propedit,toolnix-gui}

  mv ${dmgmac}/mkvtoolnix/sounds ${dmgmac}/sounds
  rmdir ${dmgmac}/mkvtoolnix

  cp README.md $dmgbase/README.txt
  cp COPYING $dmgbase/COPYING.txt
  cp NEWS.md $dmgbase/NEWS.txt

  cat > $dmgbase/README.MacOS.txt <<EOF
MKVToolNix – Mac OS specific notes
========================================

Configuration files are stored in ~/.config/bunkus.org and temporary
files are stored in the folder automatically set via TMPDIR.

This build works only with Mac OS X 10.9 and higher.

If you need the command line tools then copy mkvextract, mkvinfo,
mkvmerge and mkvproedit from ./MKVToolNix-${MTX_VER}/Contents/MacOS/
to /usr/local/bin

EOF

  mkdir -p $dmgcnt/Resources
  cp share/icons/macos/MKVToolNix.icns $dmgcnt/Resources/MKVToolNix.icns

  for file in ${TARGET}/translations/qtbase_*.qm; do
    lang=${${file%.qm}##*_}
    lcdir=${dmgmac}/locale/${lang}/LC_MESSAGES
    if [[ -d ${lcdir} ]] cp -v ${file} ${lcdir}/
  done

  ln -s /Applications ${dmgbase}/

  cat <<EOF > $dmgcnt/PkgInfo
APPL????
EOF

  cat <<EOF > $dmgcnt/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
  <dict>
    <key>LSEnvironment</key>
    <dict>
      <key>LANG</key>
      <string>en_US.UTF-8</string>
    </dict>
    <key>CFBundleDevelopmentRegion</key> <string>English</string>
    <key>CFBundleExecutable</key>        <string>mkvtoolnix-gui</string>
    <key>Properties</key>
      <dict>
        <key>file.encoding</key> <string>UTF-8</string>
      </dict>
    <key>CFBundleInfoDictionaryVersion</key> <string>${MTX_VER}</string>
    <key>CFBundlePackageType</key>           <string>APPL</string>
    <key>CSResourcesFileMapped</key>         <true/>
    <key>CFBundleVersion</key>               <string>MKVToolNix-${MTX_VER}</string>
    <key>CFBundleShortVersionString</key>    <string>${MTX_VER}</string>
    <key>NSPrincipalClass</key>              <string>NSApplication</string>
    <key>LSMinimumSystemVersion</key>        <string>${MACOSX_DEPLOYMENT_TARGET}</string>
    <key>CFBundleName</key>                  <string>MKVToolNix</string>
    <key>CFBundleIconFile</key>              <string>MKVToolNix.icns</string>
    <key>CFBundleDocumentTypes</key>
    <array>
      <dict>
        <key>CFBundleTypeExtensions</key>
        <array>
          <!-- Here goes the file formats your app can read -->
        </array>
        <key>CFBundleTypeIconFile</key>      <string>MKVToolNix.icns</string>
        <key>CFBundleTypeName</key>          <string>MKVToolNix-${MTX_VER}</string>
        <key>CFBundleTypeRole</key>          <string>Editor</string>
        <key>LSIsAppleDefaultForType</key>   <true/>
        <key>LSTypeIsPackage</key>           <false/>
      </dict>
   </array>
  </dict>
</plist>
EOF

  mkdir -p ${dmgmac}/libs
  cp -v -a ${TARGET}/lib/libQt5{Concurrent*.dylib,Core*.dylib,Gui*.dylib,Multimedia*.dylib,Network*.dylib,PrintSupport*.dylib,Widgets*.dylib} ${dmgmac}/libs/

  for plugin (audio mediaservice platforms playlistformats) cp -v -R ${TARGET}/plugins/${plugin} ${dmgmac}/

  for FILE (${dmgmac}/**/*.dylib(.) ${dmgmac}/{mkvinfo,mkvinfo-gui,mkvtoolnix-gui}) {
    otool -L ${FILE} | \
      grep -v : | \
      grep -v @executable_path | \
      awk '/libQt/ { print $1 }' | { \
      while read LIB ; do
        install_name_tool -change ${LIB} @executable_path/libs/${LIB:t} ${FILE}
      done
    }
  }

  if [[ -n ${SIGNATURE_IDENTITY} ]]; then
    typeset -a non_executables
    for FILE (${dmgcnt}/**/*(.)) {
      if [[ ${FILE} != */MacOS/mkv* ]] non_executables+=(${FILE})
    }

    codesign --force --sign ${SIGNATURE_IDENTITY} ${non_executables}
    codesign --force --sign ${SIGNATURE_IDENTITY} ${dmgmac}/mkv*(.)
  fi

  if [[ -n $DMG_NO_DMG ]] return

  volumename=MKVToolNix-${MTX_VER}
  if [[ $DMG_PRE == 1 ]]; then
    local build_number_file=$HOME/net/home/prog/video/mingw/src/uc/build-number
    local build=$(< $build_number_file)
    let build=$build+1
    echo $build > $build_number_file

    volumename=MKVToolNix-${MTX_VER}-build$(date '+%Y%m%d')-${build}-$(git rev-parse --short HEAD)
  fi

  dmgname=${CMPL}/MKVToolNix-${MTX_VER}.dmg
  dmgbuildname=${CMPL}/${volumename}.dmg

  rm -f ${dmgname} ${dmgbuildname}
  hdiutil create -srcfolder ${dmgbase} -volname ${volumename} \
    -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDZO -imagekey zlib-level=9 \
    ${CMPL}/MKVToolNix-${MTX_VER}

  if [[ -n ${SIGNATURE_IDENTITY} ]] codesign --force -s ${SIGNATURE_IDENTITY} ${dmgname}

  if [[ ${dmgname} != ${dmgbuildname} ]] mv ${dmgname} ${dmgbuildname}

  ln -s ${dmgbuildname} ${latest_link}
}

if [[ -z $MTX_VER ]]; then
  MTX_VER=$(awk -F, '/AC_INIT/ { gsub("[][]", "", $2); print $2 }' < ${SCRIPT_PATH}/../../configure.ac)
fi

if [[ -z $@ ]]; then
  build_autoconf
  build_automake
  build_pkgconfig
  build_ogg
  build_vorbis
  build_flac
  build_zlib
  build_gettext
  build_boost
  build_qt
  build_ruby
  build_configured_mkvtoolnix

else
  while [[ -n $1 ]]; do
    build_$1
    shift;
  done
fi
