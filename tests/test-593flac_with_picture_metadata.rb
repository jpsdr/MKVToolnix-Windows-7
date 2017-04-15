#!/usr/bin/ruby -w

# T_593flac_with_picture_metadata
describe "mkvmerge / FLAC with picture blocks treated as attachments"

file = "data/simple/v-pictures.flac"

test_merge file
test_merge file, :args => "-m 0"
test_merge file, :args => "-m 1"
test_merge file, :args => "-M"
test_identify file
