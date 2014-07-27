#!/usr/bin/ruby -w

# T_432concatenate_two_ac3_files
describe "mkvmerge / read from same file twice via concatenation"
test_merge "'(' data/ac3/misdetected_as_mp2.ac3 data/ac3/misdetected_as_mp2.ac3  ')'"
