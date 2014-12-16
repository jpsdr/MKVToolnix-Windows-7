#!/usr/bin/ruby -w

# T_448mpeg_ts_with_hevc
describe "mkvmerge / MPEG TS containing HEVC"
test_merge "data/h265/bbb_360p_c_short.ts"
