#!/bin/bash

# This only works on CentOS 7 so far. At least the following packages
# must be installed:

#   cmark-devel
#   desktop-file-utils
#   devtoolset-8-gcc-c++
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
#   pcre2
#   pcre2-devel
#   pkgconfig
#   po4a
#   qt5-qtbase-devel
#   qt5-qtmultimedia-devel
#   wget
#   zlib-devel

# Note that you also need Boost. While CentOS 7 includes Boost, the
# included version is too old.  Instead compile Boost yourself (as
# static libraries) and adjust the path to it down below where
# configure is executed, e.g. like this:

#   echo "using gcc : : /opt/rh/devtoolset-8/root/usr/bin/g++ ; " > tools/build/src/user-config.jam
#   ./bootstrap.sh --prefix=/opt/boost/${PWD:t} \
#     --without-libraries=python,mpi,wave,graph_parallel
#    ./b2 -j$(( $(nproc) + 2 )) variant=release link=static install


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
  devtoolset=$(ls -1d /opt/rh/devtoolset-* | tail -n 1)
  boost=$(ls -1d /opt/boost/boost* | tail -n 1)
  export CC=${devtoolset}/root/bin/gcc
  export CXX=${devtoolset}/root/bin/g++
  export CONFIGURE_ARGS="--with-boost=${boost}"
fi

export PKG_CONFIG_PATH="${QTDIR}/lib/pkgconfig:${PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="${QTDIR}/lib:${LD_LIBRARY_PATH}"
export LDFLAGS="-L${QTDIR}/lib ${LDFLAGS}"

if [[ ( ! -f build-config ) && ( "$NO_BUILD" != 1 ) ]]; then
  ./configure \
    --prefix=/usr \
    --enable-optimization \
    --with-moc="${QTDIR}/bin/moc" \
    --with-uic="${QTDIR}/bin/uic" \
    --with-rcc="${QTDIR}/bin/rcc" \
    --with-qmake="${QTDIR}/bin/qmake" \
    "$CONFIGURE_ARGS"

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
mkdir -p bin/{audio,mediaservice,platforms,iconengines,imageformats}
cp ${QTDIR}/plugins/audio/*.so bin/audio/
cp ${QTDIR}/plugins/iconengines/*svg*.so bin/iconengines/
cp ${QTDIR}/plugins/imageformats/*svg*.so bin/imageformats/
cp ${QTDIR}/plugins/mediaservice/libgst{audiodecoder,mediaplayer}*.so bin/mediaservice/
cp ${QTDIR}/plugins/platforms/libq{minimal,offscreen,wayland,xcb}*.so bin/platforms/

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

cd ..

cp ./usr/share/icons/hicolor/256x256/apps/mkvtoolnix-gui.png .
cp ./usr/share/applications/org.bunkus.mkvtoolnix-gui.desktop mkvtoolnix-gui.desktop

fix_desktop mkvtoolnix-gui.desktop

cd ..
generate_type2_appimage
