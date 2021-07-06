#!/usr/bin/ruby -w

# T_348srt_negative_timecodes2
describe "mkvmerge / SRT subitles with negative timecodes"

test_merge "--subtitle-charset -1:ISO-8859-15 data/subtitles/srt/negative_timecodes2.srt", :exit_code => 1
