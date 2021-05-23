#!/usr/bin/ruby -w

describe "mkvmerge / default track flag / in(MKV)"
test_merge "data/mkv/complex.mkv", :args => "--default-track 0 --default-track 1:0 --default-track 4:1 --default-track 9"
