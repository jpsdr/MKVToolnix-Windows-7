#!/usr/bin/ruby -w

# T_486m2ts_eac3_with_extension_in_own_packet
describe "mkvmerge / M2TS with E-AC-3 with core/extension each in their own packets"

test_merge "data/ts/hd_dolby_digital_plus_lossless_E-AC3_7.1.m2ts", :args => "--no-video --disable-lacing"
