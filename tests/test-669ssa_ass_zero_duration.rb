#!/usr/bin/ruby -w

# T_669ssa_ass_zero_duration
describe "mkvmerge / SSA/ASS with entries with duration = 0"

test_merge "data/subtitles/ssa-ass/zero_duration.ass", :keep_tmp => true
test "extraction" do
  extract tmp, 0 => "#{tmp}-x"
  hash_file "#{tmp}-x"
end
