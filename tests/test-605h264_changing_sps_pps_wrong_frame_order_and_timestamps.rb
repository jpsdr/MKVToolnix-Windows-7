#!/usr/bin/ruby -w

# T_605h264_changing_sps_pps_wrong_frame_order_and_timestamps
describe "mkvmerge / AVC/h.264 wrong frame order & timestamps due to changing SPS/PPS"
test_merge "data/h264/changing-sps-pps2.h264"
