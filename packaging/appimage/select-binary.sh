#!/bin/bash

# variables provided to type 2 AppImages:
#
# APPIMAGE — (Absolute) path to AppImage file (with symlinks resolved)
#
# APPDIR — Path of mountpoint of the SquashFS image contained in the
# AppImage
#
# OWD — Path to working directory at the time the AppImage is called
#
# ARGV0 — Name/path used to execute the script. This corresponds to
# the value you’d normally receive via the argv argument passed to
# your main method. Usually contains the filename or path to the
# AppImage, relative to the current working directory.


# Adopted from AppRun.c
export GSETTINGS_SCHEMA_DIR="$APPDIR/usr/share/glib-2.0/schemas/:$GSETTINGS_SCHEMA_DIR"
export GST_PLUGIN_SYSTEM_PATH="$APPDIR/usr/lib/gstreamer:$GST_PLUGIN_SYSTEM_PATH"
export GST_PLUGIN_SYSTEM_PATH_1_0="$APPDIR/usr/lib/gstreamer-1.0:$GST_PLUGIN_SYSTEM_PATH_1_0"
export LD_LIBRARY_PATH="$APPDIR/usr/lib/:$APPDIR/usr/lib/i386-linux-gnu/:$APPDIR/usr/lib/x86_64-linux-gnu/:$APPDIR/usr/lib32/:$APPDIR/usr/lib64/:$APPDIR/lib/:$APPDIR/lib/i386-linux-gnu/:$APPDIR/lib/x86_64-linux-gnu/:$APPDIR/lib32/:$APPDIR/lib64/:$LD_LIBRARY_PATH"
export PATH="$APPDIR/usr/bin/:$APPDIR/usr/sbin/:$APPDIR/usr/games/:$APPDIR/bin/:$APPDIR/sbin/:$PATH"
export PERLLIB="$APPDIR/usr/share/perl5/:$APPDIR/usr/lib/perl5/:$PERLLIB"
export PYTHONDONTWRITEBYTECODE=1
export PYTHONHOME="$APPDIR/usr/"
export PYTHONPATH="$APPDIR/usr/share/pyshared/:$PYTHONPATH"
export QT_PLUGIN_PATH="$APPDIR/usr/lib/qt4/plugins/:$APPDIR/usr/lib/i386-linux-gnu/qt4/plugins/:$APPDIR/usr/lib/x86_64-linux-gnu/qt4/plugins/:$APPDIR/usr/lib32/qt4/plugins/:$APPDIR/usr/lib64/qt4/plugins/:$APPDIR/usr/lib/qt5/plugins/:$APPDIR/usr/lib/i386-linux-gnu/qt5/plugins/:$APPDIR/usr/lib/x86_64-linux-gnu/qt5/plugins/:$APPDIR/usr/lib32/qt5/plugins/:$APPDIR/usr/lib64/qt5/plugins/:$QT_PLUGIN_PATH"
export XDG_DATA_DIRS="$APPDIR/usr/share/:/usr/local/share/:/usr/share"

if [ ! -z $ARGV0 ] ; then
  BINARY_NAME="$(basename "$ARGV0")"
  if [ -e "$APPDIR/usr/bin/$BINARY_NAME" ] ; then
    exec "$APPDIR/usr/bin/$BINARY_NAME" "$@"
  fi
fi

exec "$APPDIR/usr/bin/mkvtoolnix-gui" "$@"
