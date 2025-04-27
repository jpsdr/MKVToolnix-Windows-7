#!/bin/zsh

set -e
# set -x

setopt nullglob
zmodload zsh/pcre

script_dir=${${0:a}:h}
src_dir=${script_dir}/../..
src_dir=${src_dir:a}
no_strip=${no_strip:-0}
is_shared=${is_shared:-0}

if [[ -f ${script_dir}/conf.sh ]] source ${script_dir}/conf.sh

function fail {
  print -- $@
  exit 1
}

function setup_variables {
  host=$(awk '/^host *=/ { print $3 }' ${src_dir}/build-config)
  mxe_usr_dir=${mxe_dir}/usr/${host}
  if [[ $host =~ shared ]] is_shared=1
}

function strip_files {
  if [[ $no_strip == 1 ]] return

  print -n -- "Stripping files…"

  cd ${tgt_dir}
  ${host}-strip **/*.exe **/*.dll

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
  print -n -- "Creating directories…"

  cd ${tgt_dir}
  rm -rf *
  mkdir -p examples data/sounds doc/licenses locale/libqt

  print -- " done"
}

function copy_dlls {
  if [[ $is_shared == 0 ]]; then
    return
  fi

  print -n -- "Copying DLLs…"

  local dll_src_dir=$(which ${host}-g++)
  dll_src_dir=${dll_src_dir:a:h}/../${host}/bin

  cd ${tgt_dir}

  # copy MKVToolNix' own DLLs
  cp ${src_dir}/src/common/libmtxcommon.dll .

  # copy Qt plugins
  mkdir plugins
  cp -R ${mxe_dir}/usr/${host}/qt5/plugins/{audio,iconengines,imageformats,mediaservice,platforms,styles} plugins/
  rm -f plugins/platforms/{qminimal,qoffscreen}.dll

  # copy basic DLLs
  cp ${dll_src_dir}/lib{crypto-,gnurx-,harfbuzz-0,pcre-1,pcre2-16,png16-,ssl-}*.dll .

  # copy dependencies
  ${script_dir}/copy_dll_dependencies.rb *.exe **/*.dll

  # fix permissions
  chmod a+x **/*.dll

  # create qt.conf
  cat > qt.conf <<EOF
[Paths]
Plugins=./plugins
EOF

  print -- " done"
}

function copy_drmingw_dlls {
  local drmingw_dir

  cd ${src_dir}

  if [[ -f build-config.local ]]; then
    drmingw_dir="$(awk -F= '/^DRMINGW_PATH/ { gsub("^ +| +$", "", $2); print $2 }' < build-config.local)"
  fi

  if [[ -z ${drmingw_dir} ]]; then
    drmingw_dir="$(awk -F= '/^DRMINGW_PATH/ { gsub("^ +| +$", "", $2); print $2 }' < build-config)"
  fi

  if [[ -z ${drmingw_dir} ]] return

  print -n -- "Copying Dr. MinGW DLLs…"

  cp ${drmingw_dir}/bin/*.dll ${tgt_dir}/

  echo " done"
}

function copy_files {
  local qttrdir customtrdir lang baseqm mo qm
  print -n -- "Copying files…"

  cd ${src_dir}

  cp -R packaging/windows/installer examples ${tgt_dir}/
  rm -rf ${tgt_dir}/examples/stylesheets
  # src/mkvtoolnix/*.exe
  cp src/*.exe src/mkvtoolnix-gui/*.exe packaging/windows/installer/*.url ${tgt_dir}/
  cp share/icons/windows/mkvtoolnix-gui.ico ${tgt_dir}/installer/

  mkdir ${tgt_dir}/tools
  cp src/tools/{bluray_dump,ebml_validator,hevcc_dump,xyzvc_dump}.exe ${tgt_dir}/tools/

  cp src/mkvtoolnix-gui/qt_resources.rcc ${tgt_dir}/data/
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

  if grep -q 'QT_LIBS.*qt5' build-config; then
    qttrdir=${mxe_usr_dir}/qt5/translations
    customtrdir=po/qt
  else
    qttrdir=${mxe_usr_dir}/qt6/translations
    customtrdir=po/qt6
  fi

  if [[ -d ${qttrdir} ]]; then
    local qm=''
    for qm (${qttrdir}/qt_*.qm) {
      if [[ ${qm} == *qt_help* ]] continue

      lang=${${${qm:t}:r}#qt_}

      baseqm=${qttrdir}/qtbase_${lang}.qm
      if [[ -f $baseqm ]] qm=$baseqm
      cp ${qm} ${tgt_dir}/locale/libqt/qt_${lang}.qm
    }
  fi

  if [[ -d ${customtrdir} ]]; then
    local ts
    for ts (${customtrdir}/*.ts) {
      lang=${${${ts:t}:r}#qt_}

      lrelease -qm ${tgt_dir}/locale/libqt/qt_${lang}.qm ${ts} > /dev/null
    }
  fi

  typeset -a translations
  translations=($(awk '/^MANPAGES_TRANSLATIONS/ { gsub(".*= *", "", $0); gsub(" *$", "", $0); print $0 }' build-config))

  typeset -a xml_files expected_files
  typeset src_file dst_file commands saxon_process

  cd ${src_dir}/doc/man
  xml_files=(*.xml)
  to_ignore=(mkvtoolnix.xml)
  xml_files=(${xml_files:|to_ignore})

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

      echo ${script_dir}/saxon_process.sh ${lang_dir}/${src_file} ${dst_file} ${src_dir}/doc/stylesheets/docbook-to-html.xsl >> ${commands}
    }
  }

  xargs -P $(nproc) -d '\n' '-I{}' zsh -c '{}' < ${commands}

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
    -s|--saxon-jar)  saxon_jar=$2;  shift; ;;
    --exe-signer)    exe_signer=$2; shift; ;;
    *)               fail "Unknown option $1" ;;
  esac

  shift
done

if [[   -z ${tgt_dir}   ]] fail "The target directory has not been set"
if [[ ! -d ${tgt_dir}   ]] fail "The target directory does not exist"
if [[   -z ${mxe_dir}   ]] fail "The MXE base directory has not been set"
if [[ ! -d ${mxe_dir}   ]] fail "The MXE base directory does not exist"
if [[   -z ${saxon_jar} ]] fail "The Saxon-HE JAR has not been set"
if [[ ! -f ${saxon_jar} ]] fail "The Saxon-HE JAR does not exist"
if [[ ( -n ${exe_signer} ) && ( ! -x ${exe_signer} ) ]] fail "The EXE signer cannot be run"

setup_variables
create_directories
copy_files
copy_dlls
copy_drmingw_dlls
strip_files
sign_exes

exit 0
