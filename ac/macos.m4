dnl
dnl Check for being compiled on macOS
dnl

case $host in
  *darwin*)
    export LDFLAGS="$LDFLAGS -framework CoreFoundation"
    ;;
esac
