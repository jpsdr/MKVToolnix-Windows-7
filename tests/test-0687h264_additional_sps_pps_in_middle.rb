#!/usr/bin/ruby -w

# T_687h264_additional_sps_pps_in_middle
describe "mkvmerge / h.264 additional SPS & PPS with so-far unused IDs in the middle"
test_merge "data/h264/additional_sps_pps_in_middle.h264", :args => "--default-duration 0:25fps"
