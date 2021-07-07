#!/usr/bin/ruby -w

# T_565hevc_detection_failed_due_to_width_height
describe "mkvmerge / HEVC detection failed due to width/height not being said at the wrong point in time"

file = "data/h265/x265_v2.0+24_crf12_aqmode0_threads1_hightier.h265"

test_identify file
test_merge file
