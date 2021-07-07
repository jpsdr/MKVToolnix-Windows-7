#!/usr/bin/ruby -w

# T_470avi_idx1_video_not_00db
describe "mkvmerge / video tracks which use tags in idx1 other than 00db"

(1..6).each { |idx| test_merge "data/avi/idx1-video-not-00db-#{idx}.avi" }
