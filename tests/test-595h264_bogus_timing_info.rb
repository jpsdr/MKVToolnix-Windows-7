#!/usr/bin/ruby -w

# T_595h264_bogus_timing_info
describe "mkvmerge / h.264 files created by x264 with bogus timing information values"
test_merge "data/h264/bogus_timing_info.h264"
