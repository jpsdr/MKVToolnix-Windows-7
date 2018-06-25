#!/bin/zsh

setopt nullglob

script_dir=${${0:a}:h}
src_dir=${script_dir}/../..
src_dir=${src_dir:a}

if [[ -f ${script_dir}/conf.sh ]] source ${script_dir}/conf.sh

function fail {
  print -- $@
  exit 1
}

tmp_dir=$(mktemp -d)

if ! ${script_dir}/populate_installer_dir.sh -t ${tmp_dir}; then
  rm -rf ${tmp_dir}
  echo "populate_installer_dir.sh failed"
  exit 1
fi

files=$(mktemp)

cd ${tmp_dir}
find -type f | sort > ${files}
cd - &> /dev/null

if [[ -z $1 ]]; then
  cat ${files}
else
  cat ${files} > ${file_list_dir}/${1}.txt
  echo "Saved as ${file_list_dir}/${1}.txt"
fi

rm -rf ${tmp_dir} ${files}
