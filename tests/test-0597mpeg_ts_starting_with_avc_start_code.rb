#!/usr/bin/ruby -w

# T_597mpeg_ts_starting_with_avc_start_code
describe "mkvmerge / MPEG TS starting with an H.264/H.265 start code not identified at all"
test_identify "data/ts/mpeg_ts_starting_with_avc_start_code.ts"
