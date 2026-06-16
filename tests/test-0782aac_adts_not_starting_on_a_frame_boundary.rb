#!/usr/bin/ruby -w

# T_782aac_adts_not_starting_on_a_frame_boundary
describe "mkvmerge / AAC ADTS stream not starting on a frame boundary (issue #6196)"
test_merge "data/aac/issue_6196_adts_leading_garbage.aac"
