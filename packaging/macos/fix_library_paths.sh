#!/bin/zsh

while [[ -n $1 ]]; do
  FILE=$1
  shift

  otool -L ${FILE} | \
    grep -v : | \
    grep -v @executable_path | \
    awk '/libQt|libboost/ { print $1 }' | { \
    while read LIB ; do
      install_name_tool -change ${LIB} @executable_path/libs/${LIB:t} ${FILE}
    done
  }
done
