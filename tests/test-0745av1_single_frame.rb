#!/usr/bin/ruby -w

# T_745av1_single_frame
describe "mkvmerge / AV1 OBUs with only a single frame"
test_merge "data/av1/single_frame.obu"
