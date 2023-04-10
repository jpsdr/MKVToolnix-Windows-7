#!/usr/bin/ruby -w

# T_755mp4_title
describe "mkvmerge / reading 'title' meta data from MP4 files"

file = "data/mp4/with_title.mp4"

test_merge file
test_merge file, :args => "--title 'Overwrite this'"
test_identify file
