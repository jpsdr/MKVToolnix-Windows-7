#!/usr/bin/ruby -w

# T_524mpeg2_misdetected_as_truehd
describe "mkvmerge / MPEG-2 video elementary stream mis-detected as TrueHD"
test_identify "data/mpeg12/misdetected_as_truehd.m2v"
