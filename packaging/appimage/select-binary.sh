#!/bin/bash

APPIMAGE_DIR="$(dirname "$(readlink -f "${0}")")"

# Adopted from AppRun.c
export GSETTINGS_SCHEMA_DIR="$APPIMAGE_DIR/usr/share/glib-2.0/schemas/:$GSETTINGS_SCHEMA_DIR"
export GST_PLUGIN_SYSTEM_PATH="$APPIMAGE_DIR/usr/lib/gstreamer:$GST_PLUGIN_SYSTEM_PATH"
export GST_PLUGIN_SYSTEM_PATH_1_0="$APPIMAGE_DIR/usr/lib/gstreamer-1.0:$GST_PLUGIN_SYSTEM_PATH_1_0"
export LD_LIBRARY_PATH="$APPIMAGE_DIR/usr/lib/:$APPIMAGE_DIR/usr/lib/i386-linux-gnu/:$APPIMAGE_DIR/usr/lib/x86_64-linux-gnu/:$APPIMAGE_DIR/usr/lib32/:$APPIMAGE_DIR/usr/lib64/:$APPIMAGE_DIR/lib/:$APPIMAGE_DIR/lib/i386-linux-gnu/:$APPIMAGE_DIR/lib/x86_64-linux-gnu/:$APPIMAGE_DIR/lib32/:$APPIMAGE_DIR/lib64/:$LD_LIBRARY_PATH"
export PATH="$APPIMAGE_DIR/usr/bin/:$APPIMAGE_DIR/usr/sbin/:$APPIMAGE_DIR/usr/games/:$APPIMAGE_DIR/bin/:$APPIMAGE_DIR/sbin/:$PATH"
export PERLLIB="$APPIMAGE_DIR/usr/share/perl5/:$APPIMAGE_DIR/usr/lib/perl5/:$PERLLIB"
export PYTHONDONTWRITEBYTECODE=1
export PYTHONHOME="$APPIMAGE_DIR/usr/"
export PYTHONPATH="$APPIMAGE_DIR/usr/share/pyshared/:$PYTHONPATH"
export QT_PLUGIN_PATH="$APPIMAGE_DIR/usr/lib/qt4/plugins/:$APPIMAGE_DIR/usr/lib/i386-linux-gnu/qt4/plugins/:$APPIMAGE_DIR/usr/lib/x86_64-linux-gnu/qt4/plugins/:$APPIMAGE_DIR/usr/lib32/qt4/plugins/:$APPIMAGE_DIR/usr/lib64/qt4/plugins/:$APPIMAGE_DIR/usr/lib/qt5/plugins/:$APPIMAGE_DIR/usr/lib/i386-linux-gnu/qt5/plugins/:$APPIMAGE_DIR/usr/lib/x86_64-linux-gnu/qt5/plugins/:$APPIMAGE_DIR/usr/lib32/qt5/plugins/:$APPIMAGE_DIR/usr/lib64/qt5/plugins/:$QT_PLUGIN_PATH"
export XDG_DATA_DIRS="$APPIMAGE_DIR/usr/share/:/usr/local/share/:/usr/share"

# Get binary to run from executable's name: symlink the AppImage to
# e.g. "mkvmerge", execute that symlink & mkvmerge will be run from
# inside the AppImage. If no such binary exists, fall back to the GUI.
if [ ! -z $APPIMAGE ] ; then
  BINARY_NAME="$(basename "$ARGV0")"
  if [ -e "$APPIMAGE_DIR/usr/bin/$BINARY_NAME" ] ; then
    exec "$APPIMAGE_DIR/usr/bin/$BINARY_NAME" "$@"
  fi
fi

exec "$APPIMAGE_DIR/usr/bin/mkvtoolnix-gui" "$@"
