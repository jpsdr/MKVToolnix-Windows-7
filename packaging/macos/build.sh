#!/bin/zsh

set -e
set -x

export SCRIPT_PATH=${0:a:h}
source ${SCRIPT_PATH}/config.sh
source ${SCRIPT_PATH}/specs.sh

RAKE=./drake

if [[ ${spec_qtbase[1]} == *everywhere* ]]; then
  QTTYPE=everywhere
else
  QTTYPE=opensource
fi

function fail {
  echo $@
  exit 1
}

function verify_checksum {
  local file=$1
  local expected_checksum=$2
  local allow_failure=$3
  local actual_checksum

  actual_checksum=$(openssl dgst -sha256 < ${file} | sed -e 's/.* //')

  if [[ ${actual_checksum} != ${expected_checksum} ]]; then
    if [[ ${allow_failure:-0} == 1 ]]; then
      return 1
    fi

    fail "File checksum failed: ${file} SHA256 expected ${expected_checksum} actual ${actual_checksum}"
  fi
}

function retrieve_file {
  local spec_name="spec_$1"
  local -a spec=(${(P)spec_name})
  local file=${spec[1]}
  local url=${spec[2]}
  local expected_checksum=${spec[3]}

  file=${SRCDIR}/${file}

  if [[ -f ${file} ]]; then
    if verify_checksum ${file} ${expected_checksum} 1; then
      return
    fi

    echo "Warning: file ${file} exists but has the wrong checksum; retrieving anew"
    rm ${file}
  fi

  if [[ ! -f ${file} ]]; then
    curl -L ${url} > ${file}
  fi

  verify_checksum ${file} ${expected_checksum}
}

function build_tarball {
  local package=${PWD:t}
  if [[ -n $SHARED ]] package="${package}-shared"
  $DEBUG ${SCRIPT_PATH}/myinstall.sh build package $package $@
}

function build_package {
  local FUNC_NAME=$1 FILE
  case $1 in
    */*)
      FILE=${1//*\//}
      ;;
    *)
      retrieve_file $1

      local SPEC_NAME="spec_$1"
      local -a SPEC=(${(P)SPEC_NAME})
      FILE=${SPEC[1]}
      ;;
  esac
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

  local patch_dir=${SCRIPT_PATH}/${FUNC_NAME}-patches
  if [[ -d ${patch_dir} ]]; then
    for PART in ${patch_dir}/*.patch ; do
      patch -p1 < ${PART}
    done
  fi

  if [[ -z $NO_CONFIGURE ]]; then
    saved_CFLAGS=${CFLAGS}
    saved_CXXFLAGS=${CXXFLAGS}
    saved_LDFLAGS=${LDFLAGS}

    export CFLAGS="${CFLAGS} -I${TARGET}/include"
    export LDFLAGS="${LDFLAGS} -L${TARGET}/lib"

    if [[ ( -n ${CONFIGURE} ) || ( -x ./configure ) ]]; then
      $DEBUG ${CONFIGURE:-./configure} $@

      if [[ -z $NO_MAKE ]]; then
        $DEBUG make
        build_tarball
      fi

    else
      mkdir mtx-build
      cd mtx-build

      $DEBUG cmake .. $@

      if [[ -z $NO_MAKE ]]; then
        $DEBUG make
        build_tarball command "make DESTDIR=TMPDIR install"
      fi

      cd ..

    fi

    CFLAGS=${saved_CFLAGS}
    CXXFLAGS=${saved_CXXFLAGS}
    LDFLAGS=${saved_LDFLAGS}
  fi
}

mkdir -p $CMPL

function build_autoconf {
  build_package autoconf --prefix=${TARGET}
}

function build_automake {
  build_package automake --prefix=${TARGET}
}

function build_pkgconfig {
  build_package pkgconfig \
    --prefix=${TARGET} \
    --with-pc-path=${TARGET}/lib/pkgconfig \
    --with-internal-glib \
    --enable-static \
    --enable-shared=no
}

function build_libiconv {
  build_package libiconv \
    ac_cv_prog_AWK=/usr/bin/awk \
    ac_cv_path_GREP=/usr/bin/grep \
    ac_cv_path_SED=/usr/bin/sed \
    --prefix=${TARGET} \
    --enable-static \
    --docdir=${prefix}/share/doc/${name} \
    --without-libiconv-prefix \
    --without-libintl-prefix \
    --disable-nls \
    --enable-extra-encodings \
    --enable-shared=no
}

function build_cmake {
  build_package cmake --prefix=${TARGET}
}

function build_ogg {
  build_package ogg \
    --prefix=${TARGET} \
    --disable-shared \
    --enable-static
}

function build_vorbis {
  build_package vorbis \
    --prefix=${TARGET} \
    --with-ogg-libraries=${TARGET}/lib \
    --with-ogg-includes=${TARGET}/include/ \
    --enable-static \
    --disable-shared
}

function build_flac {
  build_package flac \
    --prefix=${TARGET} \
    --disable-asm-optimizations \
    --disable-xmms-plugin \
    --with-ogg-libraries=${TARGET}/lib \
    --with-ogg-includes=${TARGET}/include/ \
    --with-libiconv-prefix=${TARGET} \
    --enable-static \
    --disable-shared
}

function build_zlib {
  build_package zlib \
    --prefix=${TARGET} \
    --static
}

function build_gettext {
  build_package gettext \
    --prefix=${TARGET} \
    --disable-csharp \
    --disable-native-java \
    --disable-openmp \
    --without-emacs \
    --without-libexpat-prefix \
    --without-libxml2-prefix \
    --with-included-gettext \
    --with-included-glib \
    --with-included-libcroco \
    --with-included-libunistring \
    --with-included-libxml \
    --enable-static \
    --disable-shared
}

function build_gmp {
  build_package gmp \
    --prefix=${TARGET} \
    --enable-shared=no \
    --enable-static \
    --enable-cxx \
    --without-readline
}

function build_pcre2 {
  build_package pcre2 \
    --prefix=${TARGET} \
    --enable-pcre2-16 \
    --enable-utf \
    --enable-unicode-properties \
    --enable-cpp \
    --enable-shared=no \
    --disable-pcre2grep-libz \
    --disable-pcre2grep-libbz2 \
    --disable-pcre2test-libreadline
}

function build_boost {
  local -a args properties

  args=(--reconfigure -sICONV_PATH=${TARGET} -j$DRAKETHREADS --prefix=TMPDIR/${TARGET} --libdir=TMPDIR/${TARGET}/lib)
  properties=(toolset=clang link=static variant=release)
  if [[ -n $CXXFLAGS ]] properties+=(cxxflags="${(q)CXXFLAGS}")
  if [[ -n $LDFLAGS  ]] properties+=(linkflags="${(q)LDFLAGS}")

  NO_MAKE=1 CONFIGURE=./bootstrap.sh build_package boost \
    --with-toolset=clang
  build_tarball command "./b2 ${args} ${properties} install"
}

function build_cmark {
  build_package cmark \
    -DCMAKE_INSTALL_PREFIX=${TARGET} \
    -DCMARK_TESTS=OFF \
    -DCMARK_STATIC=ON \
    -DCMARK_SHARED=OFF
}

function build_curl {
  build_package curl \
    --prefix=${TARGET} \
    --disable-silent-rules \
    --enable-ipv6 \
    --without-brotli \
    --without-cyassl \
    --without-gnutls \
    --without-gssapi \
    --without-libmetalink \
    --without-librtmp \
    --without-libssh2 \
    --without-nghttp2 \
    --without-nss \
    --without-polarssl \
    --without-spnego \
    --without-darwinssl \
    --disable-ares \
    --disable-ldap \
    --disable-ldaps \
    --with-zlib=${TARGET} \
    --with-ssl=${TARGET} \
    --with-ca-path=${TARGET}/etc/openssl/certs
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

  local package=qtbase-${QTTYPE}-src-${QTVER}
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  NO_CONFIGURE=1 build_package qtbase

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

  local package=qttools-${QTTYPE}-src-${QTVER}
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package qttools

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

  CONFIGURE=qmake NO_MAKE=1 build_package qttranslations
  $DEBUG make
  build_tarball command "make INSTALL_ROOT=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qtmacextras {
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package qtmacextras
  $DEBUG make
  build_tarball command "make INSTALL_ROOT=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qtmultimedia {
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package qtmultimedia
  $DEBUG make
  build_tarball command "make INSTALL_ROOT=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qtsvg {
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package qtsvg
  $DEBUG make
  build_tarball command "make INSTALL_ROOT=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_qtimageformats {
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  CONFIGURE=qmake NO_MAKE=1 build_package qtimageformats
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
    --prefix=$dmgmac
    --bindir=$dmgmac
    --datarootdir=$dmgmac
    --with-extra-libs=${TARGET}/lib
    --with-extra-includes=${TARGET}/include
    --with-boost-libdir=${TARGET}/lib
    --with-docbook-xsl-root=${DOCBOOK_XSL_ROOT_DIR}
    --disable-debug
  )

  if [[ -z $SHARED_QT ]] args+=(--enable-static-qt)

  ./configure ${args}

  grep -q 'BUILD_GUI.*yes' build-config
}

function build_mkvtoolnix {
  if [[ -z ${MTX_VER} ]] fail Variable MTX_VER not set

  dmgbase=${CMPL}/dmg-${MTX_VER}
  dmgcnt=$dmgbase/MKVToolNix-${MTX_VER}.app/Contents
  dmgmac=$dmgcnt/MacOS

  NO_MAKE=1 NO_CONFIGURE=1 build_package /mkvtoolnix-${MTX_VER}.tar.xz
  build_configured_mkvtoolnix

  ${RAKE} clean
  ${RAKE} -j ${DRAKETHREADS}
}

function build_dmg {
  if [[ -z ${MTX_VER} ]] fail Variable MTX_VER not set

  if [[ -f packaging/macos/unlock_keychain.sh ]] packaging/macos/unlock_keychain.sh

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

  strip ${dmgcnt}/MacOS/mkv{merge,info,extract,propedit,toolnix-gui}

  mv ${dmgmac}/mkvtoolnix/sounds ${dmgmac}/sounds
  rmdir ${dmgmac}/mkvtoolnix

  cp README.md $dmgbase/README.txt
  cp COPYING $dmgbase/COPYING.txt
  cp NEWS.md $dmgbase/NEWS.txt

  cat > $dmgbase/README.macOS.txt <<EOF
MKVToolNix – macOS specific notes
========================================

Configuration files are stored in ~/.config/bunkus.org and temporary
files are stored in the folder automatically set via TMPDIR.

This build works only with macOS 10.14 "Mojave" or newer. Older
releases that work on older macOS versions can be found at
https://mkvtoolnix.download/downloads.html#macosx-old

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
    <key>Properties</key>
    <dict>
      <key>file.encoding</key> <string>UTF-8</string>
    </dict>
    <key>CFBundleDevelopmentRegion</key>     <string>en-US</string>
    <key>CFBundleIdentifier</key>            <string>download.mkvtoolnix.MKVToolNix</string>
    <key>CFBundleExecutable</key>            <string>mkvtoolnix-gui</string>
    <key>CFBundleInfoDictionaryVersion</key> <string>6.0</string>
    <key>CFBundlePackageType</key>           <string>APPL</string>
    <key>CSResourcesFileMapped</key>         <true/>
    <key>CFBundleVersion</key>               <string>${MTX_VER}</string>
    <key>CFBundleShortVersionString</key>    <string>${MTX_VER}</string>
    <key>NSPrincipalClass</key>              <string>NSApplication</string>
    <key>LSMinimumSystemVersion</key>        <string>${MACOSX_DEPLOYMENT_TARGET}</string>
    <key>CFBundleDisplayName</key>           <string>MKVToolNix</string>
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
  cp -v -a ${TARGET}/lib/libQt5{Concurrent*.dylib,Core*.dylib,Gui*.dylib,Multimedia*.dylib,Network*.dylib,PrintSupport*.dylib,Svg*.dylib,Widgets*.dylib} ${dmgmac}/libs/

  for plugin (audio iconengines imageformats mediaservice platforms playlistformats styles) cp -v -R ${TARGET}/plugins/${plugin} ${dmgmac}/

  ${SCRIPT_PATH}/fix_library_paths.sh ${dmgmac}/**/*.dylib(.) ${dmgmac}/{mkvmerge,mkvinfo,mkvextract,mkvpropedit,mkvtoolnix-gui}

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
    # Ziel: 29.0.0-revision-008-gb71b2b27c-01808
    # describe: release-29.0.0-8-gb71b2b27c
    local build_number_file=$HOME/net/home/prog/mac/build-number
    local build_number=$(< $build_number_file)
    let build_number=$build_number+1
    echo $build_number > $build_number_file
    build_number=$(printf '%05d' $build_number)
    local build=$(git describe --tags)
    while [[ $build != *-*-*-* ]]; do
      build=${build}-0
    done
    num=${${${build#release-}#*-}%-*}
    hash=${build##*-}
    revision="revision-$(printf '%03d' ${num})-${hash}-${build_number}"

    volumename=MKVToolNix-${MTX_VER}-${revision}
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
  build_libiconv
  build_cmake
  build_ogg
  build_vorbis
  build_flac
  build_zlib
  build_gettext
  build_cmark
  build_pcre2
  build_gmp
  build_boost
  build_qt
  build_configured_mkvtoolnix

else
  while [[ -n $1 ]]; do
    build_$1
    shift;
  done
fi
