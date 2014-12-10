#!/usr/bin/ruby -w

# T_445teletext_subs_missing_second_line
describe "mkvmerge / teletext subtitles from MPEG TS missing second lines"
test_merge "data/ts/teletext_subs_missing_second_line.ts"
