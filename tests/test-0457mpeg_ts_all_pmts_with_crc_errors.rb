#!/usr/bin/ruby -w

# T_457mpeg_ts_all_pmts_with_crc_errors
describe "mkvmerge / MPEG TS all PMTs with CRC errors"
test_merge "data/ts/pmt_crc_errors.ts"
