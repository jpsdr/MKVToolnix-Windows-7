#!/usr/bin/ruby -w

# T_654text_subtitles_without_duration
describe "mkvmerge / text subtitle tracks without BlockDuration elements"

test_merge "data/mkv/text-subtitles-without-duration.mkv", :args => "--no-audio --no-video"
