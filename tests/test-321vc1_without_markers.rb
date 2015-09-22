#!/usr/bin/ruby -w

describe "mkvmerge / VC-1 without start markers (0x00 0x00 0x01)"
test_merge "-A data/mkv/vc1-without-markers.mkv", :exit_code => :warning
