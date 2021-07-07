#!/usr/bin/ruby -w

# T_480dts_express
describe "mkvmerge / DTS Express"
test_merge "data/dts/dts-express-1.dts"
test_merge "data/ts/dts-express-1.m2ts", :args => "-D -S"
