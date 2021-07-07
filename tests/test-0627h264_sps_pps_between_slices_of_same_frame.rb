#!/usr/bin/ruby -w

# T_627h264_sps_pps_between_slices_of_same_frame
describe "mkvmerge / AVC/H.264 with SPS/PPS between two slices that belong to the same frame"
test_merge "data/h264/sps_pps_between_slices_of_same_frame.h264"
