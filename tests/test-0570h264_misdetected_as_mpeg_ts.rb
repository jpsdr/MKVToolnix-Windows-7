#!/usr/bin/ruby -w

# T_570h264_misdetected_as_mpeg_ts
describe "mkvmerge / H.264 mis-detected as MPEG TS"

test_identify "data/h264/misdetected_as_mpeg_ts_manolito_test.h264"
test_identify "data/h264/misdetected_as_mpeg_ts_sneaker_ger_128.h264"
