#!/usr/bin/ruby -w

# T_542h264_interlaced_bottom_field_first
describe "mkvmerge / AVC/H.264 interlaced with bottom field first"
test_merge "data/h264/interlaced-bottom-field-first.h264"
