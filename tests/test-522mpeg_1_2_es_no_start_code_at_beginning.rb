#!/usr/bin/ruby -w

# T_522mpeg_1_2_es_no_start_code_at_beginning
describe "mkvmerge / MPEG 1/2 video elementary streams without a start code at the beginning"
test_identify "data/mpeg12/no_start_code_at_beginning.m2v"
