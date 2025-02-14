#!/usr/bin/ruby -w

# T_773timestamps_v3
describe "mkvmerge / timestamps format v3"

test_merge "data/avi/v.avi", :args => "--disable-lacing -A --timestamps 0:data/text/timestamps_v3.txt"
