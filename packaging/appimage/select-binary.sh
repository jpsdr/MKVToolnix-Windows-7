#!/bin/bash

BIN_DIR="$(dirname "$(readlink -f "${0}")")"

if [ ! -z $APPIMAGE ] ; then
  BINARY_NAME="$(basename "$ARGV0")"
  if [ -e "$BIN_DIR/$BINARY_NAME" ] ; then
    exec "$BIN_DIR/$BINARY_NAME" "$@"
  fi
fi

exec "$BIN_DIR/mkvtoolnix-gui" "$@"
