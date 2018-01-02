#!/usr/bin/ruby -w

# T_628srt_no_decimal_places
describe "mkvmerge / SRT with timestamps without decimal places"
test_merge "data/subtitles/srt/timestamps_without_decimal_places.srt", :exit_code => :warning
