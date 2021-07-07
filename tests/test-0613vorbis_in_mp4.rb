#!/usr/bin/ruby -w

# T_613vorbis_in_mp4
describe "mkvmerge / Vorbis in MP4"
test_merge "data/mp4/vorbis.mp4", :args => "--no-video"
