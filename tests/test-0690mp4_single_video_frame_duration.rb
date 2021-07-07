#!/usr/bin/ruby -w

# T_690mp4_single_video_frame_duration
describe "mkvmerge / MP4/MOV with a single video frame, calculating the frame duration"
test_merge "data/mp4/single_video_frame.mov", :args => "-A"
