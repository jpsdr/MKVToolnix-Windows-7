#!/bin/zsh

src_dir=${${0:a}:h}/../..
src_dir=${src_dir:a}
no_strip=0

if [[ -f ${src_dir}/tools/windows/conf.sh ]] source ${src_dir}/tools/windows/conf.sh

tmpi=$(mktemp)
src_file=${1}
dst_file=${2}
stylesheet=${3}

shift 3

perl -pe 's/PUBLIC.*OASIS.*dtd"//' < ${src_file} > ${tmpi}

java -classpath ${saxon_dir}/saxon9he.jar net.sf.saxon.Transform -o:${dst_file} -xsl:${stylesheet} ${tmpi} $@ |& \
  perl -pe '
    s{^Error\s+(at.*?)?\s*on\s+line\s+(\d+)\s+column\s+(\d+)\s+of\s+(.+?):}{\4:\2:\3: error: \1};
    s{(.+?)\s+on\s+line\s+(\d+)\s+of\s+(.+)}{\3:\2: warning: \1};'

rm -f $tmpi

if [[ ! -f ${dst_file} ]] exit 1
