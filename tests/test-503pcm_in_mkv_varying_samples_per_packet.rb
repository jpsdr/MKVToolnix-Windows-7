#!/usr/bin/ruby -w

# T_503pcm_in_mkv_varying_samples_per_packet
describe "mkvmerge / PCM in Matroska using a varying number of samples per packet"
test_merge "data/pcm/varying-number-of-samples-per-packet.mkv"
