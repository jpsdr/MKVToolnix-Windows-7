#!/usr/bin/ruby -w

# T_582reading_mp4_with_and_without_track_order
describe "mkvmerge / reading MP4 with and without the --track-order parameter"

file = "data/mp4/text-track-subtitles.mp4"

test_merge file
test_merge file, :args => "--track-order 0:0,0:1"
