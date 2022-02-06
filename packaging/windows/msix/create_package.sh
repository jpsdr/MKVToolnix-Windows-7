#!/bin/bash

set -e -x

cd "$(dirname "$(readlink -f "$0")")"
if [[ -f certificate.sh ]]; then
  source ./certificate.sh
fi

if [[ -z "$1" ]] || [[ ! -f "$1" ]]; then
  echo "Error: need path to 7z package as first argument"
  exit 1
fi

if [[ ! -f assets/StoreLogo.png ]]; then
  echo "Error: the assets don't seem to have been built."
  exit 1
fi

makeappx="$(find '/c/Program Files (x86)/Windows Kits/10/bin/' -type f -iname makeappx.exe | grep -i /x64/ | sort | tail -n1)"
makepri="$(find '/c/Program Files (x86)/Windows Kits/10/bin/' -type f -iname makepri.exe | grep -i /x64/ | sort | tail -n1)"
signtool="$(find '/c/Program Files (x86)/Windows Kits/10/bin/' -type f -iname signtool.exe | grep -i /x64/ | sort | tail -n1)"

if [[ -z "$makeappx" ]]; then
  echo "Error: makeappx.exe not found"
  exit 1
fi

if [[ -z "$makepri" ]]; then
  echo "Error: makepri.exe not found"
  exit 1
fi

if [[ -z "$signtool" ]]; then
  echo "Error: signtool.exe not found"
  exit 1
fi

mtxversion="${1%.7z}"
mtxversion="${mtxversion##*-}"

while [[ "$(echo $mtxversion | sed -Ee 's/[0-9]*//g')" != ... ]]; do
  mtxversion=${mtxversion}.0
done

case "$1" in
  *64-bit*) mtxarch=x64 ;;
  *32-bit*) mtxarch=x86 ;;
  *)
    echo "Error: could not determine architecture from file name."
    exit 1
    ;;
esac

rm -rf package-${mtxarch}
mkdir -p package-${mtxarch}
cd package-${mtxarch}

winpwd="$(echo "$PWD" | sed -Ee 's!^/([a-z])!\1:!i' -e 's!/!\\!g')"

7z x "$1"
rm -f mkvtoolnix/data/portable-app mkvtoolnix/MKVToolNix.url

sed -E \
    -e "s/Version=\"\"/Version=\"${mtxversion}\"/" \
    -e "s/ProcessorArchitecture=\"\"/ProcessorArchitecture=\"${mtxarch}\"/" \
    < ../manifest.xml > manifest.xml

(
  echo '[Files]'
  echo "\"$PWD/manifest.xml\" \"AppxManifest.xml\""
  echo "\"$PWD/resources.pri\" \"Resources.pri\""

  find mkvtoolnix -type f | {
    while read name ; do
      echo "\"$PWD/${name}\" \"${name}\""
    done
  }

  find ../assets -type f | {
    while read name ; do
      echo "\"$PWD/${name}\" \"Assets${name#../assets}\""
    done
  }

) | sed -Ee 's!^"/([a-z])!"\1:!i'  -e 's!/!\\!g' > mapping.txt

msix_base="mkvtoolnix-${mtxversion}-${mtxarch}"

MSYS2_ARG_CONV_EXCL=\* "${makepri}" new /pr "${winpwd}" /mn "${winpwd}/manifest.xml" /cf "${winpwd}/../priconfig.xml" /o /of "${winpwd}/resources.pri"
MSYS2_ARG_CONV_EXCL=\* "${makeappx}" pack /f "${winpwd}/mapping.txt" /p "${winpwd}/${msix_base}.msix" /o

if [[ "$2" == "--sign" ]]; then
  cp "${msix_base}.msix" "${msix_base}-signed.msix"
  MSYS2_ARG_CONV_EXCL=\* "${signtool}" sign /f "${CERTIFICATE_PATH}" /p "${CERTIFICATE_PASSWORD}" /fd sha256 "${winpwd}/${msix_base}-signed.msix"
fi
