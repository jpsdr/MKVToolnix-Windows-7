#!/usr/bin/ruby -w

# T_659av1_timing_info_in_bitstream
describe "mkvmerge / AV1 using timing information from the bitstream"

test_merge "data/av1/av1-timing-info-constant.ivf"
test_merge "data/av1/av1-timing-info-constant.obu"
