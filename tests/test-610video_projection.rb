#!/usr/bin/ruby -w

# T_610video_projection
describe "mkvmerge / video projection track header attributes"

test_merge "data/avi/v.avi", :args => "--no-audio --projection-type 0:2 --projection-private 0:0123456789abcdef --projection-pose-yaw 0:42.123 --projection-pose-pitch 0:77.88 --projection-pose-roll 0:22.33", :keep_tmp => true
test_merge tmp, :output => "#{tmp}-2", :keep_tmp => true
test_merge "#{tmp}-2", :args => "--projection-private 0: --projection-type 0:1", :output => "#{tmp}-3"
