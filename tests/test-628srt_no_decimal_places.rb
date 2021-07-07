#!/usr/bin/ruby -w

# T_628srt_no_decimal_places
describe "mkvmerge / SRT with timestamps without decimal places"
test_merge "--subtitle-charset 0:CP1252 data/subtitles/srt/timestamps_without_decimal_places.srt", :exit_code => :warning
