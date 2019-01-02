#!/bin/zsh

if [[ -z $RUN_CLANG_TIDY_PARALLEL ]]; then
  export RUN_CLANG_TIDY_PARALLEL=1

  typeset -a args files
  while [[ -n $@ ]]; do
    if [[ -f $1 ]]; then
      files+=($1)
    else
      args+=($1)
    fi
    shift
  done

  print -l $files | xargs -P $(nproc) -n 1 -d '\n' -I{} $0 {} $args
  exit
fi

SOURCE=${1}
OUT=${SOURCE}.tidy.out.txt
ERR=${SOURCE}.tidy.err.txt
shift

clang-tidy $@ ${SOURCE} > ${OUT} 2> ${ERR}
RESULT=$?

sed -E -i \
    -e '/^[0-9]+ warnings generated/d' \
    -e '/^Suppressed [0-9]+ warnings/d' \
    -e '/^Use -header-filter=/d' \
    ${ERR}

if [[ ! -s ${OUT} ]] rm ${OUT}
if [[ ! -s ${ERR} ]] rm ${ERR}

exit ${RESULT}
