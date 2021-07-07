#!/usr/bin/ruby -w

# T_655mpeg_ts_teletext_subs_long_gap_until_end_of_display
describe "mkvmerge / MPEG TS, teletext subtitles with big gap from start until end of display causing wrong interleaving"
test_merge "data/ts/teletext_subs_long_gap_until_end_of_display.ts"
