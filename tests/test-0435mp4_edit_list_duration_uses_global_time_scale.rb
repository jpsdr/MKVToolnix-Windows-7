#!/usr/bin/ruby -w

describe "mkvmerge / QuickTime/MP4 files with edit lists whose duration must be used with global time scale"
test_merge "data/mp4/fixed_sample_size2.mp4", :exit_code => :warning
