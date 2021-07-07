#!/usr/bin/ruby -w

# T_521mp4_edit_list_constant_offset_with_segment_duration_not_0
describe "mkvmerge / MP4, edit list with two entries, first's media_time == -1, second's segment_duration != 0"
test_merge "data/mp4/edit_list_constant_offset_segment_duration_not_0.mp4", :args => "--no-audio"
