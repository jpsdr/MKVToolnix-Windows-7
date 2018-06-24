#!/bin/bash

# This only works on CentOS 7 so far. At least the following packages
# must be installed:

#   boost-devel
#   cmark-devel
#   desktop-file-utils
#   devtoolset-6-gcc-c++
#   docbook-style-xsl
#   fdupes
#   file-devel
#   flac
#   flac-devel
#   fuse
#   fuse-libs
#   gettext-devel
#   glibc-devel
#   gtest-devel
#   libogg-devel
#   libstdc++-devel
#   libvorbis-devel
#   libxslt
#   make
#   pkgconfig
#   po4a
#   qt5-qtbase-devel
#   qt5-qtmultimedia-devel
#   rubygem-drake
#   wget
#   zlib-devel

# This must be run from inside an unpacked MKVToolNix source
# directory. You can run it from inside a git checkout, but make sure
# to that submodules have been initialized and updated.

set -e set -x

if [[ ! -e /dev/fuse ]]; then
  sudo mknod --mode=0666 /dev/fuse c 10 229
fi

TOP_DIR="$(readlink -f ${0})"
TOP_DIR="${TOP_DIR%/*}/../.."
cd "${TOP_DIR}"
TOP_DIR="${PWD}"

QTVERSION="5.11.1"
QTDIR="${HOME}/opt/qt/${QTVERSION}/gcc_64"

APP="mkvtoolnix-gui"
VERSION="$(perl -ne 'next unless m/^AC_INIT/; s{.*?,\[}{}; s{\].*}{}; print; exit' ${TOP_DIR}/configure.ac)"
JOBS=$(nproc)

wget -O "${TOP_DIR}/tools/appimage/functions.sh" -q https://raw.githubusercontent.com/AppImage/AppImages/master/functions.sh
. "${TOP_DIR}/tools/appimage/functions.sh"

if [[ ! -f configure ]]; then
  ./autogen.sh
fi

if [[ -f /etc/centos-release ]]; then
  export CC=/opt/rh/devtoolset-6/root/bin/gcc
  export CXX=/opt/rh/devtoolset-6/root/bin/g++
fi

export PKG_CONFIG_PATH="${QTDIR}/lib/pkgconfig:${PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="${QTDIR}/lib:${LD_LIBRARY_PATH}"
export LDFLAGS="-L${QTDIR}/lib ${LDFLAGS}"

./configure \
  --prefix=/usr \
  --enable-appimage \
  --enable-optimization \
  --with-moc="${QTDIR}/bin/moc" \
  --with-uic="${QTDIR}/bin/uic" \
  --with-rcc="${QTDIR}/bin/rcc" \
  --with-qmake="${QTDIR}/bin/qmake"

drake clean
rm -rf appimage out

drake -j${JOBS} apps:mkvtoolnix-gui
# exit $?
drake -j${JOBS}
drake install DESTDIR="${TOP_DIR}/appimage/${APP}.AppDir"

cd appimage/${APP}.AppDir/usr
strip ./bin/*

mkdir -p lib lib64
chmod u+rwx lib lib64

copy_deps

find

find -type d -exec chmod u+w {} \+

mkdir all_libs
mv ./home all_libs
mv ./lib* all_libs
mv ./usr all_libs
mkdir lib
mv `find all_libs -type f` lib
rm -rf all_libs

# dlopen()ed by libQt5Network
if [[ -f /etc/centos-release ]]; then
  cp -f /lib64/libssl.so.* lib/
  cp -f /lib64/libcrypto.so.* lib/
else
  cp -f /lib/x86_64-linux-gnu/libssl.so.1.0.0 lib
  cp -f /lib/x86_64-linux-gnu/libcrypto.so.1.0.0 lib
fi

delete_blacklisted

mkdir ./share/file
if [[ -f /etc/centos-release ]]; then
  cp /usr/share/misc/magic.mgc ./share/file
else
  cp /usr/share/file/magic.mgc ./share/file
fi

cd ..
cp ./usr/share/applications/org.bunkus.mkvtoolnix-gui.desktop mkvtoolnix-gui.desktop
fix_desktop mkvtoolnix-gui.desktop
cp ./usr/share/icons/hicolor/256x256/apps/mkvtoolnix-gui.png .

get_apprun
cd ..
generate_type2_appimage
