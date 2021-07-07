#!/usr/bin/ruby -w

# T_454mp4_dash
describe "mkvmerge / MP4 DASH files"

dir = "data/mp4/dash"
test_merge "#{dir}/car-20120827-85.mp4 #{dir}/car-20120827-8c.mp4"
test_merge "#{dir}/dragon-age-inquisition-H1LkM6IVlm4-video.mp4 #{dir}/dragon-age-inquisition-H1LkM6IVlm4-audio.mp4"
