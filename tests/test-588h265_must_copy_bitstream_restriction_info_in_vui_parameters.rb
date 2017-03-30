#!/usr/bin/ruby -w

# T_588h265_must_copy_bitstream_restriction_info_in_vui_parameters
describe "mkvmerge / HEVC/h.265: don't forget to parse bitstream restriction info in VUI parameters"
test_merge "data/h265/issue_1924.hevc"
