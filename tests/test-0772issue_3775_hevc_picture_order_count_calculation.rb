#!/usr/bin/ruby -w

# T_772issue_3775_hevc_picture_order_count_calculation
describe "mkvmerge / issue 3775 HEVC picture order calculation wrong"

test_merge "data/h265/issue-3775-wrong-picture-order-calculation.hevc"
