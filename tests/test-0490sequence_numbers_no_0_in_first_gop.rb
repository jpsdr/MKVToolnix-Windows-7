#!/usr/bin/ruby -w

# T_490sequence_numbers_no_0_in_first_gop
describe "mkvmerge / MPEG-2 ES without a sequence number == 0 in the first GOP"
test_merge "data/mpeg12/sequence_numbers_no_0_in_first_gop.m2v"
