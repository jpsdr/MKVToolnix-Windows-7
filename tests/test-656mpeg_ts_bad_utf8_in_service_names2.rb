#!/usr/bin/ruby -w

# T_656mpeg_ts_bad_utf8_in_service_names2
describe "mkvmerge / bad UTF-8 encoded service names in MPEG transport streams"
test_identify "data/ts/bad_utf-8_service_names2.ts"
