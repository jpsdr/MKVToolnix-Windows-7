#!/usr/bin/ruby -w

# T_645ogg_opus_first_timestamp_negative
describe "mkvmerge / Ogg Opus with first granulepos being smaller than number of samples in packet = negative first timestamp"
test_merge "data/opus/first-timestamp-negative.opus", :args => "--timestamp-scale 1000000"
test_merge "data/opus/first-timestamp-negative.opus", :args => "--timestamp-scale 1000000 --disable-lacing"
