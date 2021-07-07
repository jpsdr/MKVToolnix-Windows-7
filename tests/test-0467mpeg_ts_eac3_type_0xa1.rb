#!/usr/bin/ruby -w

# T_467mpeg_ts_eac3_type_0xa1
describe "mkvmerge / MPEG TS, E-AC-3 with type 0xa1"

test_merge "data/ts/eac3-type-0xa1.m2ts"
