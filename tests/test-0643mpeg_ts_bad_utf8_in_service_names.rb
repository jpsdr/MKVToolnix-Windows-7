#!/usr/bin/ruby -w

# T_643mpeg_ts_bad_utf8_in_service_names
describe "mkvmerge / identification of MPEG transport streams with invalid UTF-8 strings for service names"
test_identify "data/ts/bad_utf-8_service_names.ts"
