#!/usr/bin/ruby -w

# T_447mkvinfo_rounded_timecodes
describe "mkvinfo / rounded timecodes"

output = "#{tmp}-merge"
test_merge "data/h264/progressive-23.976p.h264", :args => "--timecode-scale 50000", :output => output, :keep_tmp => true
test_info output, :args => "-s"
test_info output, :args => "-v -v"
