#!/usr/bin/ruby -w

# T_513vp9_10bit_key_frame_detection
describe "mkvmerge / key frame detection for 10bit VP9 profiles"
test_merge "data/webm/vp9-10bit.mkv", :args => "--no-audio"
