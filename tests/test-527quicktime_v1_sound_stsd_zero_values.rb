#!/usr/bin/ruby -w

# T_527quicktime_v1_sound_stsd_zero_values
describe "mkvmerge / QuickTime files, v1 sound STSD atom with values set to zero"
test_merge "data/mp4/v1_sound_stsd_zero_values.mov", args: "--no-video"
