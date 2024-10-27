#!/usr/bin/ruby -w

# T_770timestamps_v1
describe "mkvmerge / timestamps format v1"

file = "data/av1/av1.obu"

test_merge file
test_merge file, :args => "--timestamps 0:data/text/timestamps_v1.txt"
test_merge file, :args => "--timestamps 0:data/text/timestamps_v1.json"
