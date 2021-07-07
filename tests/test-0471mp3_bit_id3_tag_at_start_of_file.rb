#!/usr/bin/ruby -w

# T_471mp3_bit_id3_tag_at_start_of_file
describe "mkvmerge / MP3 files with big ID3 tags at the start of the file"
test_merge "data/mp3/big-id3-tag-at-start-of-file-1.mp3", :exit_code => :warning
test_merge "data/mp3/big-id3-tag-at-start-of-file-2.mp3"
