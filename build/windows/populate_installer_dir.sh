#!/bin/zsh

set -e
set -x

setopt nullglob

src_dir=${${0:a}:h}/../..
src_dir=${src_dir:a}
no_strip=0

if [[ -f ${src_dir}/build/windows/conf.sh ]] source ${src_dir}/build/windows/conf.sh

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

function saxon_process {
  java -classpath ${saxon_dir}/saxon9he.jar net.sf.saxon.Transform $@ |& \
    perl -pe '
      s{^Error\s+(at.*?)?\s*on\s+line\s+(\d+)\s+column\s+(\d+)\s+of\s+(.+?):}{\4:\2:\3: error: \1};
      s{(.+?)\s+on\s+line\s+(\d+)\s+of\s+(.+)}{\3:\2: warning: \1};'
}

function create_directories {
  print -- "Creating directories…"

  cd ${tgt_dir}
  rm -rf *
  mkdir -p examples data doc locale/libqt

  print -- " done"
}

function copy_files {
  local qt5trdir lang baseqm mo qm
  print -n -- "Copying files…"

  cd ${src_dir}

  cp -R installer examples ${tgt_dir}/
  rm -rf ${tgt_dir}/examples/stylesheets
  cp src/*.exe src/mkvtoolnix-gui/*.exe share/icons/windows/*.ico installer/*.url ${tgt_dir}/

  cp ${mxe_usr_dir}/share/misc/magic.mgc ${tgt_dir}/data/
  touch ${tgt_dir}/data/portable-app

  cp README.md ${tgt_dir}/doc/README.txt
  cp COPYING ${tgt_dir}/doc/COPYING.txt
  cp ChangeLog ${tgt_dir}/doc/ChangeLog.txt
  cp doc/command_line_references.html ${tgt_dir}/doc/

  for mo in po/*.mo ; do
    language=${${mo:t}:r}
    mkdir -p ${tgt_dir}/locale/${language}/LC_MESSAGES
    cp ${mo} ${tgt_dir}/locale/${language}/LC_MESSAGES/mkvtoolnix.mo
  done

  local -A customized_qt5_translations
  local ts
  for ts (po/qt/*.ts) {
    lang=${${${ts:t}:r}#qt_}
    customized_qt5_translations[$lang]=1

    lrelease -qm ${tgt_dir}/locale/libqt/qt_${lang}.qm ${ts}
  }

  qt5trdir=${mxe_usr_dir}/qt5/translations
  for qm (${qt5trdir}/qt_*.qm) {
    if [[ ${qm} == *qt_help* ]] continue

    lang=${${${qm:t}:r}#qt_}

    if [[ -n ${customized_qt5_translations[${lang}]} ]] continue

    baseqm=${qt5trdir}/qtbase_${lang}.qm
    if [[ -f $baseqm ]] qm=$baseqm
    cp ${qm} ${tgt_dir}/locale/libqt/qt_${lang}.qm
  }

  for lang (. de es ja nl uk zh_CN) {
    cd ${src_dir}/doc/man/${lang}

    if [[ $lang == . ]] lang=en

    man_dest=${tgt_dir}/doc/${lang}
    mkdir -p ${man_dest}
    cp ${src_dir}/doc/stylesheets/mkvtoolnix-doc.css ${man_dest}/

    for i (*.xml) {
      tmpi=$(mktemp)
      perl -pe 's/PUBLIC.*OASIS.*dtd"//' < $i > ${tmpi}
      saxon_process -o:${man_dest}/$(basename $i .xml).html -xsl:${src_dir}/doc/stylesheets/docbook-to-html.xsl $tmpi
      rm -f $tmpi
      if [[ ! -f ${man_dest}/$(basename $i .xml).html ]] exit 1
    }
  }

  echo " done"
}

while [[ ! -z $1 ]]; do
  case $1 in
    -t|--target-dir) tgt_dir=$2;   shift; ;;
    -m|--mxe-dir)    mxd_dir=$2;   shift; ;;
    -s|--saxon-dir)  saxon_dir=$2; shift; ;;
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

setup_variables
create_directories
copy_files
strip_files

exit 0
