#!/usr/bin/ruby -w

describe "mkvmerge / default track flag / in(MKV)"
test_merge "data/mkv/complex.mkv", :args => "--default-track-flag 0 --default-track-flag 1:0 --default-track-flag 4:1 --default-track-flag 9"
