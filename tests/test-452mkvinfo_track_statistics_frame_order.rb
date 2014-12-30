#!/usr/bin/ruby -w

# T_452mkvinfo_track_statistics_frame_order
describe "mkvinfo / track statistics for highest timecode not last frame"

output = "#{tmp}-merge"
test_merge "data/h264/profile_high_444_predictive_@l3.1.h264", :output => output, :keep_tmp => true
test_info output, :args => "-t"
