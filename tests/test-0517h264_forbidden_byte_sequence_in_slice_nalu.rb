#!/usr/bin/ruby -w

# T_517h264_forbidden_byte_sequence_in_slice_nalu
describe "mkvmerge / H.264 slices with forbidden byte value requiring NALU-to-RBSP conversion"
test_merge "data/h264/forbidden_byte_sequence_in_slice_header.h264"
