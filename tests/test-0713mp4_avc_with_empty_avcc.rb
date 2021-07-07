#!/usr/bin/ruby -w

# T_713mp4_avc_with_empty_avcc
describe "mkvmerge / MP4 with h.264/AVC track with an empty avcC"
test_merge "data/mp4/avc_with_empty_avcc.mp4"
