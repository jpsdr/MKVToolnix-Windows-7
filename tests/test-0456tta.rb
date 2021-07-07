#!/usr/bin/ruby -w

# T_456tta
describe "mkvmerge, mkvextract / merge and extract TrueAudio (TTA)"

output = "#{tmp}-merge"
test_merge "data/tta/v.tta", :output => output, :keep_tmp => true
test "extraction" do
  extract output, 0 => tmp
  hash_tmp
end
