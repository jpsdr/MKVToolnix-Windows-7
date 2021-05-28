#!/usr/bin/ruby -w

# T_722simple_chapters_nanosecond_precision
describe "mkvmerge / OGM-style chapters with up to nanosecond precision"

test_merge "data/subtitles/srt/ven.srt", :args => "--chapters data/chapters/nanoseconds.txt"
