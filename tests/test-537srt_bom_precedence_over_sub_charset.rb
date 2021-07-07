#!/usr/bin/ruby -w

# T_537srt_bom_precedence_over_sub_charset
describe "mkvmerge / SRT: BOMs have precedence over --sub-charset"

test_merge "--sub-charset 0:ISO-8859-15 data/subtitles/srt/vde-utf-8-bom.srt"
test_merge "                            data/subtitles/srt/vde-utf-8-bom.srt"
test_merge "--sub-charset 0:ISO-8859-15 data/subtitles/srt/vde.srt"
test_merge "                            data/subtitles/srt/vde.srt", :exit_code => :warning, :skip_if => $is_windows
