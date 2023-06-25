#!/bin/bash

# This only works on AlmaLinux 8 with Qt 6.5.x. At least the following
# packages must be installed:

#   attr
#   autoconf
#   bash
#   boost-devel
#   createrepo
#   desktop-file-utils
#   docbook-style-xsl
#   fdupes
#   file-devel
#   flac
#   flac-devel
#   fmt-devel
#   fuse
#   fuse-libs
#   gcc-c++
#   gcc-toolset-11-annobin-plugin-gcc
#   gcc-toolset-11-gcc-c++
#   gettext-devel
#   git
#   glibc-devel
#   gmp-devel
#   gtest-devel
#   hicolor-icon-theme
#   libdvdread-devel
#   libogg-devel
#   libstdc++-devel
#   libvorbis-devel
#   libxslt
#   make
#   moreutils
#   pcre2-devel
#   pkgconfig
#   po4a
#   rpm-build
#   rpmlint
#   ruby
#   rubygem-rake
#   strace
#   sudo
#   vim
#   wget
#   zlib-devel
#   zsh

# This must be run from inside an unpacked MKVToolNix source
# directory. You can run it from inside a git checkout, but make sure
# to that submodules have been initialized and updated.

set -e
set -x

if [[ ! -e /dev/fuse ]]; then
  sudo mknod --mode=0666 /dev/fuse c 10 229
fi

TOP_DIR="$(readlink -f ${0})"
TOP_DIR="${TOP_DIR%/*}/../.."
cd "${TOP_DIR}"
TOP_DIR="${PWD}"
RELEASE_VERSION=0
QTVERSION="latest"
APP="mkvtoolnix-gui"
APP_DIR="${TOP_DIR}/appimage/${APP}.AppDir"

function display_help {
  cat <<EOF
MKVToolNix AppImage build script

Syntax:

  build.sh [-B|--no-build] [-q|--qt <Qt version>] [-r|--release-version]
           [-h|--help]

Parameters:

  --no-build         don't run 'configure' and './drake clean'; only
                     possible if 'build-run' exists
  --qt <Qt version>  build against this Qt version (default: $QTVERSION)
  --release-version  don't built the version number via 'git describe'
                     even if '.git' directory is present
  --help             display help
EOF

  exit 0
}

while [[ -n $1 ]]; do
  case $1 in
    -B|--no-build)        NO_BUILD=1           ;;
    -q|--qt)              QTVERSION=$2 ; shift ;;
    -r|--release-version) RELEASE_VERSION=1    ;;
    -h|--help)            display_help         ;;
  esac

  shift
done

QTDIR="${HOME}/opt/qt/${QTVERSION}/gcc_64"
NO_GLIBC_VERSION=1

if git rev-parse --show-toplevel &> /dev/null && [[ $RELEASE_VERSION == 0 ]]; then
  # If revision is a tag: release-28.2.0
  # If it isn't: release-28.1.0-7-g558fbc986
  VERSION="$(git describe --tags | sed -e 's/release-//')"
  if [[ $VERSION != *-*-* ]]; then
    VERSION=${VERSION}-0-g0
  fi
  NUM=${VERSION%-*}
  NUM=${NUM##*-}
  VERSION="${VERSION%%-*}-z$(printf '%03d' ${NUM})-${VERSION##*-}"
else
  VERSION="$(perl -ne 'next unless m/^AC_INIT/; s{.*?,\[}{}; s{\].*}{}; print; exit' ${TOP_DIR}/configure.ac)"
fi
JOBS=$(nproc)

functions_sh="${TOP_DIR}/packaging/appimage/functions.sh"
if [[ ! -f "${functions_sh}" ]]; then
   wget -O "${functions_sh}" -q https://raw.githubusercontent.com/AppImage/AppImages/master/functions.sh
fi
source "${functions_sh}"

if [[ ! -f configure ]]; then
  ./autogen.sh
fi

if [[ -f /etc/centos-release ]]; then
  devtoolset=$(ls -1d /opt/rh/*toolset-* | tail -n 1)
  export CC=${devtoolset}/root/bin/gcc
  export CXX=${devtoolset}/root/bin/g++
fi

export PKG_CONFIG_PATH="${QTDIR}/lib/pkgconfig:${PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="${QTDIR}/lib:${LD_LIBRARY_PATH}"
export LDFLAGS="-L${QTDIR}/lib ${LDFLAGS}"
export PATH="${QTDIR}/bin:${PATH}"

if [[ ( ! -f build-config ) && ( "$NO_BUILD" != 1 ) ]]; then
  ./configure \
    --prefix=/usr \
    --enable-optimization

  ./drake clean
fi

rm -rf "${APP_DIR}" out

./drake -j${JOBS} apps:mkvtoolnix-gui
./drake -j${JOBS}
./drake install DESTDIR="${APP_DIR}"

cd appimage/${APP}.AppDir

cp "${TOP_DIR}/packaging/appimage/select-binary.sh" AppRun
chmod 0755 AppRun

cd usr

# Qt plugins
mkdir -p bin/{iconengines,imageformats,multimedia,platforms,platforminputcontexts,tls}
cp ${QTDIR}/plugins/iconengines/*svg*.so bin/iconengines/
cp ${QTDIR}/plugins/imageformats/*svg*.so bin/imageformats/
cp ${QTDIR}/plugins/multimedia/libgst*.so bin/multimedia/
cp ${QTDIR}/plugins/platforms/libq{minimal,offscreen,wayland,xcb}*.so bin/platforms/
cp ${QTDIR}/plugins/platforminputcontexts/lib*.so bin/platforminputcontexts/
cp ${QTDIR}/plugins/tls/libqopensslbackend.so bin/tls/

find bin -type f -exec strip {} \+

mkdir -p lib lib64
chmod u+rwx lib lib64

copy_deps

find -type d -exec chmod u+w {} \+

mkdir all_libs
mv ./home all_libs
mv ./lib* all_libs
mv ./usr all_libs
mkdir lib
# inefficient loop due to the same lib potentially being present in
# several directories & mv throwing a fit about it ("will not
# overwrite just-created…")
for lib in `find all_libs -type f` ; do
  mv $lib lib/
done
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

cd ..

cp ./usr/share/icons/hicolor/256x256/apps/mkvtoolnix-gui.png .
cp ./usr/share/applications/org.bunkus.mkvtoolnix-gui.desktop mkvtoolnix-gui.desktop

fix_desktop mkvtoolnix-gui.desktop

cd ..
generate_type2_appimage
