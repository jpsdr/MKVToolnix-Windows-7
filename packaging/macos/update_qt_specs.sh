#!/bin/zsh

set -e

QT_VERSION=$1
QT_BASE_URL=https://download.qt.io/official_releases/qt/

TMPF=$(mktemp)

trap "rm -f ${TMPF}" EXIT

if [[ -z ${QT_VERSION} ]]; then
  print -- "Determining latest Qt version"

  curl --silent ${QT_BASE_URL} | \
    grep -E '<a +href *= *"6\.' | \
    sed -Ee 's/.*<a +href *= *"6\./6./' -e 's/[/"].*//' | \
    sort --version-sort | \
    tail -n 1 > ${TMPF}

  QT_VERSION=$(<${TMPF})

  curl --silent ${QT_BASE_URL}/${QT_VERSION}/ | \
    grep -E '<a +href *= *"6\.' | \
    sed -Ee 's/.*<a +href *= *"6\./6./' -e 's/[/"].*//' | \
    sort --version-sort | \
    tail -n 1 > ${TMPF}

  QT_VERSION=$(<${TMPF})
fi

print -- "Qt version to update to: ${QT_VERSION}"

QT_URL=${QT_BASE_URL}${QT_VERSION%.*}/${QT_VERSION}/submodules/

print -- "Determining submodule hashes"

# spec_qtbase=(qtbase-everywhere-src-5.13.1.tar.xz https://download.qt.io/official_releases/qt/5.13/5.13.1/submodules/qtbase-everywhere-src-5.13.1.tar.xz 20fbc7efa54ff7db9552a7a2cdf9047b80253c1933c834f35b0bc5c1ae021195)

for SUBMODULE in qtbase qtimageformats qtmultimedia qtsvg qttools qttranslations; do
  FILE_NAME=${SUBMODULE}-everywhere-src-${QT_VERSION}.tar.xz
  FILE_URL=${QT_URL}${FILE_NAME}

  curl --silent ${FILE_URL}.mirrorlist | \
    grep SHA-256 | \
    sed -e 's/.*<tt>//' -e 's/<\/tt>.*//' > ${TMPF}

  FILE_SHA256=$(<${TMPF})

  echo "spec_${SUBMODULE}=(${FILE_NAME} ${FILE_URL} ${FILE_SHA256})"
done
