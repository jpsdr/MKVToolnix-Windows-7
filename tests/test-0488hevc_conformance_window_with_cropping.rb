#!/usr/bin/ruby -w

# T_488hevc_conformance_window_with_cropping
describe "mkvmerge / HEVC with conformance_window in SPS present and cropping applied"
test_merge "data/h265/hevc_1080_lines_deteced_as_1088.265"
