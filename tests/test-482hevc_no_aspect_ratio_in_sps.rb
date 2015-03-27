#!/usr/bin/ruby -w

# T_482hevc_no_aspect_ratio_in_sps
describe "mkvmerge / HEVC no aspect ratio in sequence parameter set"
(1..3).each { |idx| test_merge "data/h265/no-aspect-ratio-#{idx}.hevc" }
