#!/usr/bin/ruby -w

# T_594hevc_split_parts_discarding_start_endless_loop
describe "mkvmerge / splitting by parts discarding the first part leads to an endless loop"
test_merge "data/h265/split_parts_endless_loop.hevc", :args => "--split parts:00:01-00:05"
