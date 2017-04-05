#!/bin/zsh

set -e
# set -x

setopt nullglob

src_dir=${${0:a}:h}/../..
src_dir=${src_dir:a}
no_strip=0

if [[ -f ${src_dir}/tools/windows/conf.sh ]] source ${src_dir}/tools/windows/conf.sh

function fail {
  print -- $@
  exit 1
}

function setup_variables {
  host=$(awk '/^host *=/ { print $3 }' ${src_dir}/build-config)
  mxe_usr_dir=${mxe_dir}/usr/${host}
}

function strip_files {
  if [[ $no_strip == 1 ]] return

  print -n -- "Stripping files…"

  cd ${tgt_dir}
  ${host}-strip *.exe

  print -- " done"
}

function sign_exes {
  if [[ -z $exe_signer ]] return

  print -n -- "Signing executables…"

  cd ${tgt_dir}
  for exe (*.exe) {
    ${exe_signer} ${exe} ${exe}.signed
    mv ${exe}.signed ${exe}
  }

  print -- " done"
}

function create_directories {
  print -- "Creating directories…"

  cd ${tgt_dir}
  rm -rf *
  mkdir -p examples data/sounds doc/licenses locale/libqt

  print -- " done"
}

function copy_files {
  local qt5trdir lang baseqm mo qm
  print -n -- "Copying files…"

  cd ${src_dir}

  cp -R installer examples ${tgt_dir}/
  rm -rf ${tgt_dir}/examples/stylesheets
  cp src/*.exe src/mkvtoolnix-gui/*.exe installer/*.url ${tgt_dir}/
  cp share/icons/windows/mkvtoolnix-gui.ico ${tgt_dir}/installer/

  cp ${mxe_usr_dir}/share/misc/magic.mgc ${tgt_dir}/data/
  cp share/sounds/* ${tgt_dir}/data/sounds/
  touch ${tgt_dir}/data/portable-app

  cp README.md ${tgt_dir}/doc/README.txt
  cp COPYING ${tgt_dir}/doc/COPYING.txt
  cp NEWS.md ${tgt_dir}/doc/NEWS.txt
  cp doc/command_line_references.html ${tgt_dir}/doc/
  cp doc/licenses/*.txt ${tgt_dir}/doc/licenses/

  for mo in po/*.mo ; do
    language=${${mo:t}:r}
    mkdir -p ${tgt_dir}/locale/${language}/LC_MESSAGES
    cp ${mo} ${tgt_dir}/locale/${language}/LC_MESSAGES/mkvtoolnix.mo
  done

  local qt5trdir=${mxe_usr_dir}/qt5/translations
  local qm
  for qm (${qt5trdir}/qt_*.qm) {
    if [[ ${qm} == *qt_help* ]] continue

    lang=${${${qm:t}:r}#qt_}

    baseqm=${qt5trdir}/qtbase_${lang}.qm
    if [[ -f $baseqm ]] qm=$baseqm
    cp ${qm} ${tgt_dir}/locale/libqt/qt_${lang}.qm
  }

  local ts
  for ts (po/qt/*.ts) {
    lang=${${${ts:t}:r}#qt_}

    lrelease -qm ${tgt_dir}/locale/libqt/qt_${lang}.qm ${ts}
  }

  typeset -a translations
  translations=($(awk '/^MANPAGES_TRANSLATIONS/ { gsub(".*= *", "", $0); gsub(" *$", "", $0); print $0 }' build-config))

  typeset -a xml_files expected_files
  typeset src_file dst_file commands saxon_process

  cd ${src_dir}/doc/man
  xml_files=(*.xml)

  commands=$(mktemp)

  for lang (. $translations) {
    typeset lang_dir=${src_dir}/doc/man/${lang}
    cd ${lang_dir}

    if [[ $lang == . ]] lang=en

    man_dest=${tgt_dir}/doc/${lang}
    mkdir -p ${man_dest}
    cp ${src_dir}/doc/stylesheets/mkvtoolnix-doc.css ${man_dest}/

    for src_file (${xml_files}) {
      dst_file=${man_dest}/$(basename ${src_file} .xml).html
      expected_files+=(${dst_file})

      echo ${src_dir}/tools/windows/saxon_process.sh ${lang_dir}/${src_file} ${dst_file} ${src_dir}/doc/stylesheets/docbook-to-html.xsl >> ${commands}
    }
  }

  xargs -n 1 -P $(nproc) -d '\n' -i zsh -c '{}' < ${commands}

  rm -f ${commands}

  for dst_file (${expected_files}) {
    if [[ ! -f ${dst_file} ]] exit 1
  }

  echo " done"
}

while [[ ! -z $1 ]]; do
  case $1 in
    -t|--target-dir) tgt_dir=$2;    shift; ;;
    -m|--mxe-dir)    mxe_dir=$2;    shift; ;;
    -s|--saxon-dir)  saxon_dir=$2;  shift; ;;
    --exe-signer)    exe_signer=$2; shift; ;;
    *)               fail "Unknown option $1" ;;
  esac

  shift
done

if [[   -z ${tgt_dir}   ]] fail "The target directory has not been set"
if [[ ! -d ${tgt_dir}   ]] fail "The target directory does not exist"
if [[   -z ${mxe_dir}   ]] fail "The MXE base directory has not been set"
if [[ ! -d ${mxe_dir}   ]] fail "The MXE base directory does not exist"
if [[   -z ${saxon_dir} ]] fail "The Saxon-HE base directory has not been set"
if [[ ! -d ${saxon_dir} ]] fail "The Saxon-HE base directory does not exist"
if [[ ( -n ${exe_signer} ) && ( ! -x ${exe_signer} ) ]] fail "The EXE signer cannot be run"

setup_variables
create_directories
copy_files
strip_files
sign_exes

exit 0
