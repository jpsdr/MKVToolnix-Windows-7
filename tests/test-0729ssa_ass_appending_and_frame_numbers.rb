#!/usr/bin/ruby -w

# T_729ssa_ass_appending_and_frame_numbers
describe "mkvmerge / SSA/ASS: re-calculating frame numbers when appending"

base_name = "data/subtitles/ssa-ass/assconcatsample"

test_merge "#{base_name}1.mkv + #{base_name}2.mkv", :exit_code => :warning
