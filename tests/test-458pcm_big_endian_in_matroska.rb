#!/usr/bin/ruby -w

# T_458pcm_big_endian_in_matroska
describe "mkvmerge / Big Endian PCM in Matroska"
test_merge 'data/pcm/big-endian.mka'
