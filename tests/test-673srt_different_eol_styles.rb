#!/usr/bin/ruby -w

# T_673srt_different_eol_styles
describe "mkvmerge / SRT text subtitles with both DOS & Unix style line endings"
test_merge "data/subtitles/srt/different_eol_styles.srt", :exit_code => :warning
