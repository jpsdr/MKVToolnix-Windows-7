#!/usr/bin/ruby -w

# T_714avi_video_aspect_ratio
file = "data/avi/video_aspect_ratio.avi"

describe "mkvmerge / AVI with video aspect ratio in video properties header"
test_merge file
test_identify file
