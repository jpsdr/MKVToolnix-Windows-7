#!/usr/bin/ruby -w

# T_587X_ssa_ass_shorter_non_standard_event_format
describe "mkvextract / SSA/ASS subtitles with non-standard/shorter 'Format:' line in the [Events] section"
test "extraction" do
  extract "data/subtitles/ssa-ass/shorter_non_standard_event_format.mkv", 0 => tmp
  hash_tmp
end
