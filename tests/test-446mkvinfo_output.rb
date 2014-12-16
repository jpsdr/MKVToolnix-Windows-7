#!/usr/bin/ruby -w

# T_446mkvinfo_output
describe "mkvinfo / output of various modes"

test_info "data/mkv/complex.mkv"
test_info "data/mkv/complex.mkv", :args => "-v -v"
test_info "data/mkv/complex.mkv", :args => "-v -v -z"
test_info "data/mkv/complex.mkv", :args => "-s -t"
