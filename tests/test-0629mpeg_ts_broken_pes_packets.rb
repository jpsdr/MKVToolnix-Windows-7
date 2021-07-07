#!/usr/bin/ruby -w

# T_629mpeg_ts_broken_pes_packets
describe "mkvmerge / MPEG TS, broken PES packets"
test_merge "data/ts/broken_pes_packets.ts"
