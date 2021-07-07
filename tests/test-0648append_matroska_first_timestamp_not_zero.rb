#!/usr/bin/ruby -w

# T_648append_matroska_first_timestamp_not_zero
describe "mkvmerge / appending Matroska files where the second file's first timestamp is bigger than 0"
test_merge "data/mkv/bug2345-1.mkv + data/mkv/bug2345-2.mkv"
