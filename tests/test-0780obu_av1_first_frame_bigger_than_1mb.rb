#!/usr/bin/ruby -w

# T_780obu_av1_first_frame_bigger_than_1mb
describe "mkvmerge / OBU with AV1, first frame is bigger than probe buffer of 1MB"

test_merge "data/av1/issue_6165_first_frame_bigger_than_1mb.av1"
