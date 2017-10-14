#!/usr/bin/ruby -w

# T_620ac3_incomplete_frame_with_timestamp_from_matroska
describe "mkvmerge / incomplete AC-3 frames in Matroska whose timestamps must not be used"
test_merge "data/ac3/incomplete_frame_with_timestamp.mka", :exit_code => :warning
