#!/usr/bin/ruby -w

# T_742keep_colour_properties
describe "mkvmerge / keep all colour poperties when remuxing"
test_merge "data/mkv/colour_properties.mkv"
test_identify "data/mkv/colour_properties.mkv"
