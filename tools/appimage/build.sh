#!/bin/bash
# This is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
#
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# For more information, please refer to <http://unlicense.org/>

set -e
set -x

APP=MKVToolNix
LOWERAPP=${APP,,}
JOBS=4
CXX="g++"

#VERSION="git"
VERSION="21.0.0"

mtxdir="mkvtoolnix-$VERSION"
mtxurl="https://mkvtoolnix.download/sources/${mtxdir}.tar.xz"
giturl="https://gitlab.com/mbunkus/mkvtoolnix"
cmurl="https://mkvtoolnix.download/ubuntu/xenial/binary/amd64/libcmark-dev_0.28.3-1~bunkus01_amd64.deb"

export LDFLAGS="-Wl,-z,relro -Wl,--as-needed -Wl,-rpath,XORIGIN/../lib"

TOP="$PWD"

mkdir -p $APP
cd $APP

# build-dependencies
if [ -x /usr/bin/apt ]; then
  sudo apt update
  sudo apt -y upgrade
  sudo apt -y dist-upgrade
  sudo apt -y --no-install-recommends install \
    git autoconf automake build-essential pkg-config wget ca-certificates git cmake \
    p7zip-full fuse python python3 gettext rake docbook-xsl xsltproc chrpath \
    libogg-dev libvorbis-dev libflac-dev libboost-all-dev libmagic-dev \
    qt5-default qtbase5-dev qtbase5-dev-tools qtmultimedia5-dev
  sudo apt clean
fi

wget $cmurl -O libcmark-dev.deb
sudo dpkg -i libcmark-dev.deb

if [ ! -e "$mtxdir/usr/bin/mkvtoolnix-gui" ]; then
  rm -rf $mtxdir
  if [ "$VERSION" = "git" ]; then
    git clone $giturl $mtxdir
    cd $mtxdir
    git describe --always | tail -c+9 > version
    git submodule init
    git submodule update
    ./autogen.sh
  else
    wget -c $mtxurl
    tar xf ${mtxdir}.tar.xz
    cd $mtxdir
  fi
  ./configure --prefix=/usr --disable-debug --enable-appimage
  rake JOBS=$JOBS
  rake install DESTDIR="$PWD"
  cd -
fi

wget -q https://github.com/probonopd/AppImages/raw/master/functions.sh -O ./functions.sh
. ./functions.sh

rm -rf ${APP}.AppDir
mkdir ${APP}.AppDir
cd ${APP}.AppDir

cp -r ../$mtxdir/usr .
strip --strip-all ./usr/bin/*
chrpath -k -r '$ORIGIN/../lib' ./usr/bin/*

mkdir -p ./usr/share/file
if [ -e /usr/share/file/magic.mgc ]; then
  cp /usr/share/file/magic.mgc ./usr/share/file
elif [ -e /usr/share/misc/magic.mgc ]; then
  cp /usr/share/misc/magic.mgc ./usr/share/file
elif [ -e /usr/lib/file/magic.mgc ]; then
  cp /usr/lib/file/magic.mgc ./usr/share/file
fi

mkdir -p ./usr/lib
cp -r /usr/lib/x86_64-linux-gnu/qt5/plugins ./usr/lib/qtplugins

copy_deps
move_lib
mv ./usr/lib/x86_64-linux-gnu/* ./usr/lib
delete_blacklisted

cp ./usr/share/icons/hicolor/256x256/apps/mkvtoolnix-gui.png ${LOWERAPP}.png
cp "$TOP/mtx.desktop" ${LOWERAPP}.desktop

cp "$TOP/mtxlaunch.sh" ./usr/bin
ln -s usr/bin/mtxlaunch.sh AppRun
chmod a+x ./usr/bin/*

cat <<EOF> ./usr/bin/qt.conf
[Paths]
Plugins = ../lib/qtplugins
lib = ../lib
EOF

url="$mtxurl"
if [ "$VERSION" = "git" ]; then
  url="$giturl  $(cat ../$mtxdir/version)"
fi

cat <<EOF> SOURCES
MKVToolNix: $url
libcmark: $cmurl
          https://github.com/commonmark/cmark

Build system:
$(lsb_release -irc)
$(uname -mo)

Package repositories:
$(cat /etc/apt/sources.list /etc/apt/sources.list.d/* | grep '^deb ')

EOF

GLIBC_NEEDED=$(glibc_needed)
if [ "$VERSION" = "git" ]; then
  VERSION=$(cat ../$mtxdir/version)
fi

cd ..
generate_type2_appimage
