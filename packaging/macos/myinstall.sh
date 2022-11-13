#!/bin/zsh

set -e
set -x

source ${0:h}/config.sh

MODE=
FILE=
INSTALL_COMMAND="make DESTDIR=TMPDIR install"
PACKAGE=

function fail {
  print -- "$@"
  exit 1
}

function run_build {
  local package tmpdir install_command

  package=${PACKAGE:-${PWD:t}}
  tmpdir=$HOME/tmp/install-${$}-${package}
  install_command=${INSTALL_COMMAND//TMPDIR/${tmpdir}}
  echo PACKAGE $PACKAGE package $package tmpdir $tmpdir install_command $install_command
  $DEBUG mkdir -p $tmpdir $PACKAGE_DIR

  $DEBUG eval ${install_command}

  $DEBUG cd ${tmpdir}${TARGET}
  $DEBUG tar czf ${PACKAGE_DIR}/${package}.tar.gz .
  $DEBUG cd ${tmpdir}/..
  $DEBUG rm -rf $tmpdir
  $DEBUG cd ${TARGET}
  $DEBUG tar xzf ${PACKAGE_DIR}/${package}.tar.gz
}

function run_uninstall {
  if [[ -z $FILE ]] fail "No file given"
  FILE=${FILE:a}
  if [[ ! -f $FILE ]] FILE=${PACKAGE_DIR}/${FILE}.tar.gz
  if [[ ! -f $FILE ]] fail "No such package: $FILE"

  local tmpfile=$(mktemp ${TMPDIR}/uninstallXXXXXX)
  cd ${TARGET}
  tar tzf ${FILE} > ${tmpfile}
  grep -v '/$' ${tmpfile} | tr '\n' '\0' | xargs -0 $DEBUG rm -f
  grep    '/$' ${tmpfile} | perl -le '$/ = ""; print join("\n", reverse split(/\n/, <>))' | tr '\n' '\0' | xargs -0 $DEBUG rmdir || true
  rm ${tmpfile}
}

function run_install {
  if [[ -z $FILE ]] fail "No file given"
  FILE=${FILE:a}
  if [[ ! -f $FILE ]] FILE=${PACKAGE_DIR}/${FILE}.tar.gz
  if [[ ! -f $FILE ]] fail "No such package: $FILE"

  cd ${TARGET}
  tar xzf ${FILE}
}

while [[ -n $1 ]]; do
  case $1 in
    build|-b)     MODE=build ;;
    uninstall|-u) MODE=uninstall; FILE=$2; shift ;;
    install|-i)   MODE=install;   FILE=$2; shift ;;
    package|-p)   PACKAGE=$2;              shift ;;
    command|-c)   INSTALL_COMMAND=$2;      shift ;;
    --)           shift ;                  break; ;;
    *)            fail "Unknown option $1"        ;;
  esac

  shift
done

if [[ -z $MODE ]] fail "No mode given"

run_${MODE} $@
