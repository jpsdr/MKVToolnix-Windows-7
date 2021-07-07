#!/usr/bin/ruby -w

describe "mkvmerge / MP3 in MP4, sample rate == 0 in track headers"

file = "data/mp4/mp3-samplerate-0.mp4"

test_identify file
test_merge file, :exit_code => :warning
