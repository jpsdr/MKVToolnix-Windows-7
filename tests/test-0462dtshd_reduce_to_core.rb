#!/usr/bin/ruby -w

# T_462dtshd_reduce_to_core
describe "mkvmerge / reducing DTS HD tracks to their core"
test_merge "data/dts/dts-hd.dts"
test_merge "data/dts/dts-hd.dts", :args => "--reduce-to-core 0"
test_merge "data/dts/dts-hd.dts", :args => "--reduce-to-core -1"
