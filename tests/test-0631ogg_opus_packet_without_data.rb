#!/usr/bin/ruby -w

# T_631ogg_opus_packet_without_data
describe "mkvmerge / Ogg Opus, a packet without data"
test_merge "data/opus/packet_without_data.ogg", :exit_code => :warning
