#!/usr/bin/ruby -w

# T_624h264_fix_bitstream_timing_info_exactly
describe "mkvmerge / H.264 fixing bitstream timing information with exact values"
test_merge "data/h264/10i.264", :args => "--default-duration 0:10i --fix-bitstream-timing-information 0:1"
