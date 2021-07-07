#!/usr/bin/ruby -w

file = "data/mkv/segment_size_0.mkv"

describe "mkvinfo, mkvmerge / segment size 0"
test_info file
test_identify file
