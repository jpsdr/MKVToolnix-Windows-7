#!/bin/zsh

set -e

if (( ${#@} != 2 )); then
  echo Usage: mkvver newver codename
  exit 1
fi
if [[ ! -f src/common/os.h ]]; then
  echo Wrong directory.
  exit 1
fi

VERSION=$(awk '-F[' '/AC_INIT/ { gsub("].*", "", $3); print $3 }' configure.ac)
if [[ -z $VERSION ]]; then
  echo Could not find the old version.
  exit 1
fi

export FROM=$VERSION
export TO=$1
export CODENAME=$2
export MANDATE="`date '+%Y-%m-%d'`"

function update_debian_changelog {
  export LC_TIME=en_US.UTF-8
  tmp=$(mktemp)
  date=$(date '+%a, %d %b %Y %H:%M:%S %z')
  ubuntu=$(awk "-F'" '/define_debian_ubuntu.*ubuntu/ { ubuntu=$4 } END { print ubuntu }' /home/mosu/prog/video/support/rake.d/debian_ubuntu_linuxmint.rb)
  cat > ${tmp} <<EOF
mkvtoolnix (${TO}-0~bunkus01) ${ubuntu}; urgency=low

  * New version.

 -- Moritz Bunkus <mo@bunkus.online>  ${date}

EOF
  cat packaging/debian/changelog >> ${tmp}
  cat ${tmp} > packaging/debian/changelog
  rm -f ${tmp}
}

function update_news {
  local date=$(LC_TIME=en_US.UTF-8 date '+%Y-%m-%d')
  local to_replace="# Version ?"
  local message="# Version ${TO} \"${CODENAME}\" ${date}"

  grep -q -F ${message} NEWS.md && return 0

  if ! grep -qF '# Version ?' NEWS.md; then
    echo "NEWS.md is missing the 'unknown version' line."
    exit 1
  fi

  perl -pi -e "s{^# Version \\?\$}{${message}}" NEWS.md
}

function update_spec {
  local date="$(date '+%a %b %e %Y')"
  perl -pi -e "
    s/^Version:.*/Version: ${TO}/;
    s/^Release:.*/Release: 1/;
    s/^(\\%changelog.*)/\${1}
* ${date} Moritz Bunkus <mo\\@bunkus.online> ${TO}-1
- New version
/" packaging/centos-fedora-opensuse/mkvtoolnix.spec
}

function update_files {
  TO_NSI=$TO
  while ! echo "$TO_NSI" | grep -q '\..*\.' ; do
    TO_NSI="${TO_NSI}.0"
  done

  perl -pi -e 's/^(AC_INIT.*\[)'$FROM'(\].*)/${1}'$TO'${2}/' configure.ac
  perl -pi -e 's/^MKVToolNix '$FROM'$/MKVToolNix '$TO'/' README.md
  perl -pi -e 's/^Building MKVToolNix [0-9.]+/Building MKVToolNix '$TO'/i' Building.for.Windows.md
  perl -pi -e 's/define PRODUCT_VERSION .*/define PRODUCT_VERSION \"'$TO_NSI'\"/' packaging/windows/installer/mkvtoolnix.nsi
  perl -pi -e "s{^constexpr.*VERSIONNAME.*}{constexpr auto VERSIONNAME = \"${CODENAME}\";}" src/common/version.cpp
}

function update_docs {
  TO_VERINF="$( echo $TO | sed -e 's/\./, /g')"
  while ! echo "$TO_VERINF" | grep -q ',.*,.*,' ; do
    TO_VERINF="${TO_VERINF}, 0"
  done

  setopt nullglob
  perl -pi -e "s/FILEVERSION.*/FILEVERSION ${TO_VERINF}/;
    s/PRODUCTVERSION.*/PRODUCTVERSION ${TO_VERINF}/;
    s/VALUE.*FileVersion.*/VALUE \"FileVersion\", \"${TO}\"/;
    s/VALUE.*ProductVersion.*/VALUE \"ProductVersion\", \"${TO}\"/;" \
      src/*/*.rc
  perl -pi -e "s/<!ENTITY version \".*\">/<!ENTITY version \"${TO}\">/; s/<!ENTITY date \".*\">/<!ENTITY date \"${MANDATE}\">/;" doc/man/**/*.xml
  perl -pi -e 's/^msg(id|str) "'${FROM}'"/msg$1 "'${TO}'"/' doc/man/po4a/po/*.{po,pot}

  ./drake
}

function update_appstream_metainfo {
  perl -pi -e "s{<releases>\\n}{<releases>\\n    <release version=\"${TO}\" date=\"${MANDATE}\"></release>\\n}" \
    share/metainfo/org.bunkus.mkvtoolnix-gui.appdata.xml
}

update_news
update_files
update_debian_changelog
update_spec
update_appstream_metainfo
update_docs

echo "Done setting the new version."
