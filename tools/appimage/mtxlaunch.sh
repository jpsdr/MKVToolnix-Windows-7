#!/bin/sh

check_cmd () {
  case "$1" in
    "mkvtoolnix-gui"|"mkvtoolnix")
        cmd="mkvtoolnix-gui"
        ;;
    "mkvinfo-text")
        cmd="mkvinfo"
        ;;
    "mkvextract"|"mkvinfo"|"mkvmerge"|"mkvpropedit")
        cmd="$1"
        ;;
  esac
}

if [ "x$ARGV0" != "x" ]; then
  argv0="$(basename "$ARGV0")"
  check_cmd "$argv0"
fi
if [ "x$cmd" = "x" ] && [ "x$1" != "x" ]; then
  check_cmd "$1"
fi
if [ "x$cmd" = "x" ]; then
  cmd="mkvtoolnix-gui"
else
  shift
fi

bindir="$(dirname "$(readlink -f "$0")")"
export PATH="$bindir:$PATH"
"$bindir/$cmd" $gui "$@"
