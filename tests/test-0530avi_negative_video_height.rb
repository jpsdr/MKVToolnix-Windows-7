#!/usr/bin/ruby -w

# T_530avi_negative_video_height
describe "mkvmerge / AVIs with negative video height"
test_merge "data/avi/negative-height.avi"
