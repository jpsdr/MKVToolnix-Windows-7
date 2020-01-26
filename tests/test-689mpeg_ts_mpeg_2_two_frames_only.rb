#!/usr/bin/ruby -w

# T_689mpeg_ts_mpeg_2_two_frames_only
describe "mkvmerge / MPEG transport stream with MPEG-2 video with two frames only"
test_merge "data/ts/mpeg_2_two_frames.m2ts", :args => "--no-audio --no-subtitles -d 0"
