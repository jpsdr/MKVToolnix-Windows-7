#!/usr/bin/ruby -w

# T_614white_colour_coordinates_from_mkv
describe "mkvmerge / copy white colour coordinates header attributes from Matroska files"

test_merge "data/avi/v.avi", :args => "--no-audio --white-colour-coordinates 0:1,2", :keep_tmp => true
test_merge tmp, :output => "#{tmp}-2"
