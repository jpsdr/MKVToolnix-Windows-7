#!/usr/bin/ruby -w

# T_591hevc_wrong_number_of_parameter_sets
describe "mkvmerge / HEVC/h.265 wrong number of parameter sets in HEVCC calculated"
test_merge "data/h265/wrong_number_of_parameter_sets_in_hevcc.ts", :args => "--no-audio"
