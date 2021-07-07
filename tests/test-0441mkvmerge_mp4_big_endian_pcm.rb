#!/usr/bin/ruby -w

# T_441mkvmerge_mp4_big_endian_pcm
describe "mkvmerge / MP4 with Big Endian PCM audio tracks"
test_merge "data/mp4/big_endian_pcm.mp4"
