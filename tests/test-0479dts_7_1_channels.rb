#!/usr/bin/ruby -w

# T_479dts_7_1_channels
describe "mkvmerge / DTS with more than six channels"
test_merge "data/dts/channel_ext_7_1.dts"
test_merge "data/ts/dts_channel_ext_7_1.m2ts", :args => "-D -S -a 1"
