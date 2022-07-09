#!/usr/bin/ruby -w

# T_742keep_color_properties
describe "mkvmerge / keep all color poperties when remuxing"
test_merge "data/mkv/color_properties.mkv"
test_identify "data/mkv/color_properties.mkv"
