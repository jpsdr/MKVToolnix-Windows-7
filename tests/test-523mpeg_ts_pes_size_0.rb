#!/usr/bin/ruby -w

file = "data/ts/pes_size_0.ts"

# T_523mpeg_ts_pes_size_0
describe "mkvmerge / MPEG TS with a video track with all packets having a PES size of 0"
test_merge file, exit_code: :warning
test_identify file
