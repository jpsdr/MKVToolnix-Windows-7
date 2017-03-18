#!/usr/bin/ruby -w

# T_585h264_wrong_mapping_of_timestamps_to_frames
describe "mkvmerge / h.264 in MPEG transport stream resulting in wrong mapping of timestamps to frames"
test_merge "data/ts/h264_wrong_mapping_of_timestamps_to_frames.ts", :args => "--no-audio"
