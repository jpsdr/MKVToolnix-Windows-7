#!/usr/bin/ruby -w

# T_571bluray_pcm_odd_number_of_channels
describe "mkvmerge / Blu-ray PCM with odd number of channels"
test_merge "data/pcm/bluray_pcm_1_channel.m2ts", :args => "--disable-lacing -D -S -a 1"
