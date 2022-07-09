#!/usr/bin/ruby -w

# T_614white_color_coordinates_from_mkv
describe "mkvmerge / copy white color coordinates header attributes from Matroska files"

test_merge "data/avi/v.avi", :args => "--no-audio --white-color-coordinates 0:1,2", :keep_tmp => true
test_merge tmp, :output => "#{tmp}-2"
