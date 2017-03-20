#!/usr/bin/ruby -w

# T_436extract_ssa_extradata_after_events
describe "mkvextract / SSA/ASS with extradata after events"
test "extraction" do
  merge "data/subtitles/ssa-ass/extradata-after-events.ssa", :keep_tmp => true
  extract tmp, 0 => "#{tmp}-x1"
  hash_file "#{tmp}-x1"
end
