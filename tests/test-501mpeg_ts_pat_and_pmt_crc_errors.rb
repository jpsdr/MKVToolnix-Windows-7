#!/usr/bin/ruby -w

# T_501mpeg_ts_pat_and_pmt_crc_errors
describe "mkvmerge / MPEG TS files, all PATs and PMTs with CRC errors"
test_identify "data/ts/pat_pmt_crc_errors.m2ts"
