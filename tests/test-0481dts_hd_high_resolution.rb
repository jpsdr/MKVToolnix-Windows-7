#!/usr/bin/ruby -w

# T_481dts_hd_high_resolution
describe "mkvmerge / DTS-HD High Resolution"
test_merge "data/dts/dts-hd-hr-5.1.dts"
test_merge "data/dts/dts-hd-hr-6.1.dts"
