#!/usr/bin/ruby -w

# T_640no_date
describe "mkvmerge / don't write the date"
test_merge "data/subtitles/srt/ven.srt", :args => "--no-date"
