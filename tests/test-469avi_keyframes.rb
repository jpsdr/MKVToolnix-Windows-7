#!/usr/bin/ruby -w

# T_469avi_keyframes
describe "mkvmerge / AVI with key frames whose dwFlags index field equals 0x12"
test_merge "data/avi/dwflags-0x12.avi"
