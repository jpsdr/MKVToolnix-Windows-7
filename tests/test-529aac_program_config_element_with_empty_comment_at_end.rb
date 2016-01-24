#!/usr/bin/ruby -w

# T_529aac_program_config_element_with_empty_comment_at_end
describe "mkvmerge / AAC in MP4 with program config element with empty comment field"
test_merge "data/mp4/aac_pce_empty_comment_field_at_end.m4a"
