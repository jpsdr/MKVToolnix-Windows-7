#!/usr/bin/ruby -w

# T_642avc_es_clear_internal_buffers_after_reading_headers
describe "mkvmerge / AVC/H.264 identification failure due to remaining data in parser's buffers"
test_merge "data/h264/failure-due-to-internal-buffers.h264"
