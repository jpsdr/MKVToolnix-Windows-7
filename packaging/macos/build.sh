#!/bin/zsh

set -e
set -x

setopt nullglob

export SCRIPT_PATH=${0:a:h}
source ${SCRIPT_PATH}/config.sh
test -f ${SCRIPT_PATH}/config.local.sh && source ${SCRIPT_PATH}/config.local.sh
source ${SCRIPT_PATH}/specs.sh

RAKE=./drake

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
    xz|lz|lzma) COMPRESSION=J ;;
    bz2)        COMPRESSION=j ;;
    gz)         COMPRESSION=z ;;
    tar)        COMPRESSION=  ;;
    *)          echo Unknown compression for ${FILE} ; exit 1 ;;
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

  if [[ -n $NO_CONFIGURE ]]; then
    return
  fi

  saved_CFLAGS=${CFLAGS}
  saved_CXXFLAGS=${CXXFLAGS}
  saved_LDFLAGS=${LDFLAGS}

  export CFLAGS="${CFLAGS} -I${TARGET}/include"
  export LDFLAGS="${LDFLAGS} -L${TARGET}/lib"

  if [[ ( -n ${CONFIGURE} ) || ( -x ./configure ) ]]; then
    if [[ -n ${build_package_hook_pre_configure} ]]; then
      ${build_package_hook_pre_configure}
    fi

    $DEBUG ${CONFIGURE:-./configure} $@

    if [[ -z $NO_MAKE ]]; then
      $DEBUG make

      if [[ -n ${build_package_hook_pre_installation} ]]; then
        ${build_package_hook_pre_installation}
      fi

      build_tarball
    fi

  else
    mkdir mtx-build
    cd mtx-build

    $DEBUG cmake .. $@

    if [[ -z $NO_MAKE ]]; then
      $DEBUG make

      if [[ -n ${build_package_hook_pre_installation} ]]; then
        ${build_package_hook_pre_installation}
      fi

      build_tarball command "make DESTDIR=TMPDIR install"
    fi

    cd ..

  fi

  CFLAGS=${saved_CFLAGS}
  CXXFLAGS=${saved_CXXFLAGS}
  LDFLAGS=${saved_LDFLAGS}
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

function build_vorbis_pre_configure {
  echo fixing libVorbis build

  perl -pi -e 's{-+force_cpusubtype_ALL}{}g' configure.ac configure
}

function build_vorbis {
  build_package_hook_pre_configure=build_vorbis_pre_configure \
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
    --with-ogg-libraries=${TARGET}/lib \
    --with-ogg-includes=${TARGET}/include/ \
    --with-libiconv-prefix=${TARGET} \
    --enable-static \
    --disable-shared
}

function build_zlib {
  DIR=${${spec_zlib[1]%%.tar*}/zlib-v/zlib-} \
  build_package zlib \
    --prefix=${TARGET} \
    --static
}

function build_gettext_fix_compilation {
  perl -pi -e 's/#define setlocale.*//g' $( find . -name libintl.h )
}

function build_gettext {
  build_package_hook_pre_installation=build_gettext_fix_compilation \
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
    --with-pic \
    --without-readline
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

function build_qt {
  local -a args
  args=(
    --prefix=${TARGET}
    -opensource
    -confirm-license
    -release
    -force-pkg-config
    -pkg-config

    -nomake benchmarks
    -nomake examples
    -nomake tests
    -nomake manual-tests
    -nomake minimal-static-tests

    -skip qt3d,qt5compat,qtactiveqt,qtcharts,qtcoap,qtconnectivity,qtdatavis3d,qtdeclarative,qtdoc,qthttpserver,qtlanguageserver,qtlottie,qtmqtt,qtnetworkauth,qtopcua,qtpositioning,qtquick3d,qtquick3dphysics,qtquicktimeline,qtremoteobjects,qtscxml,qtsensors,qtserialbus,qtserialport,qtspeech,qtvirtualkeyboard,qtwayland,qtwebchannel,qtwebengine,qtwebsockets,qtwebview,qtgraphs,qtlocation,qtquickeffectmaker

    -no-framework
    -no-avx512

    -no-feature-cups
    -no-feature-dbus
    -no-feature-glib
    -no-feature-openssl
    -no-feature-sql
    -no-feature-sql-db2
    -no-feature-sql-ibase
    -no-feature-sql-mysql
    -no-feature-sql-oci
    -no-feature-sql-odbc
    -no-feature-sql-sqlite

    -no-sbom

    --
    -DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET
  )

  local package=qt-everywhere-src-${QTVER}
  local saved_CXXFLAGS=$CXXFLAGS
  export CXXFLAGS="${QT_CXXFLAGS}"
  export QMAKE_CXXFLAGS="${CXXFLAGS}"

  NO_CONFIGURE=1 build_package qt

  cd ${CMPL}/${package}
  $DEBUG ./configure ${args}

  time $DEBUG cmake \
       --build . \
       --parallel $DRAKETHREADS

  # cd ${CMPL}/${package}
  build_tarball command "make DESTDIR=TMPDIR install"

  CXXFLAGS=$saved_CXXFLAGS
}

function build_docbook_xsl {
  if [[ -d ${DOCBOOK_XSL_ROOT_DIR} ]]; then
    return
  fi

  retrieve_file docbook_xsl

  tar xfC ${SRCDIR}/${spec_docbook_xsl[1]} ${DOCBOOK_XSL_ROOT_DIR:h}
  ln -s ${${spec_docbook_xsl[1]}%%.tar*} ${DOCBOOK_XSL_ROOT_DIR}
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

  ./configure ${args}

  grep -q 'BUILD_GUI.*yes' build-config
}

function build_mkvtoolnix {
  if [[ -z ${MTX_VER} ]] fail Variable MTX_VER not set

  dmgbase=${CMPL}/dmg-${MTX_VER}
  dmgcnt=$dmgbase/MKVToolNix-${MTX_VER}.app/Contents
  dmgmac=$dmgcnt/MacOS

  if [[ ! -f ${SRCDIR}/mkvtoolnix-${MTX_VER}.tar.xz ]]; then
    curl -o ${SRCDIR}/mkvtoolnix-${MTX_VER}.tar.xz https://mkvtoolnix.download/sources/mkvtoolnix-${MTX_VER}.tar.xz
  fi

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

  mv ${dmgmac}/mkvtoolnix ${dmgmac}/data

  cp README.md $dmgbase/README.txt
  cp COPYING $dmgbase/COPYING.txt
  cp NEWS.md $dmgbase/NEWS.txt

  cat > $dmgbase/README.macOS.txt <<EOF
MKVToolNix – macOS specific notes
========================================

Configuration files are stored in ~/.config/bunkus.org and temporary
files are stored in the folder automatically set via TMPDIR.

This build works only with macOS ${MACOSX_DEPLOYMENT_TARGET} or newer. Older
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

  sed -e "s/::MTX_VER::/${MTX_VER}/" -e "s/::MACOSX_DEPLOYMENT_TARGET::/${MACOSX_DEPLOYMENT_TARGET}/" \
      < packaging/macos/Info.plist \
      > $dmgcnt/Info.plist

  cat $dmgcnt/Info.plist

  mkdir -p ${dmgmac}/libs
  cp -v -a \
     ${TARGET}/lib/libQt6{Concurrent*.dylib,Core*.dylib,Gui*.dylib,Multimedia*.dylib,Network*.dylib,PrintSupport*.dylib,Svg*.dylib,Widgets*.dylib} \
     ${TARGET}/lib/libboost_system*.dylib \
     ${dmgmac}/libs/

  cd ${TARGET}/plugins

  for dylib in audio/* iconengines/libqsvg*dylib imageformats/libqico*dylib imageformats/libqsvg*dylib imageformats/libqweb*dylib multimedia/lib*dylib platforms/libqcocoa*dylib styles/*dylib tls/*dylib; do
    dstdir=${dmgmac}/${dylib:h:t}
    mkdir -p ${dstdir}
    cp -v ${dylib} ${dstdir}/
  done

  cd -

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
    local build_number_file=$HOME/opt/build-number
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

mkdir -p ${TARGET} ${TARGET}/include ${TARGET}/lib ${SRCDIR} ${DOCBOOK_XSL_ROOT_DIR:h}

if [[ -z $MTX_VER ]]; then
  MTX_VER=$(awk -F, '/AC_INIT/ { gsub("[][]", "", $2); print $2 }' < ${SCRIPT_PATH}/../../configure.ac)
fi

if [[ -z $@ ]]; then
  build_docbook_xsl
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
  build_gmp
  build_boost
  build_qt
  build_mkvtoolnix

else
  while [[ -n $1 ]]; do
    build_$1
    shift;
  done
fi
