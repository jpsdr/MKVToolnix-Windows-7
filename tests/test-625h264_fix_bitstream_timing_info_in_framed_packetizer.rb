#!/usr/bin/ruby -w

# T_625h264_fix_bitstream_timing_info_in_framed_packetizer
describe "mkvmerge / H.264 framed output module, fixing bitstream timing info in all SPS"
test_merge "data/h264/10i.mkv", :args => "--default-duration 0:10i --fix-bitstream-timing-information 0:1"
