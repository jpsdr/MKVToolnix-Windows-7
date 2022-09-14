#!/bin/bash

set -e -x

cd "$(dirname "$(readlink -f "$0")")"

base_dir=${base_dir:-/z/home/mosu/files/html/bunkus.org/videotools/mkvtoolnix/windows/releases}

function determine_latest_version {
  ls "${base_dir}" | \
    grep '[0-9]*\.[0-9]*' | \
    sed -Ee 's!.*/!!' | \
    sort -V | \
    tail -n 1
}

mtxversion=${mtxversion:-$(determine_latest_version)}
archive_dir="${archive_dir:-${base_dir}/${mtxversion}}"

sign=

if [[ $1 =~ '^(-s|--sign|--signed)$' ]]; then
  sign=--sign
fi

for file in "${archive_dir}"/${mtxversion}/*.7z; do
  ./create_package.sh "$file" $sign
done

mv -v package-*/*.msix /z/home/mosu/dl/
