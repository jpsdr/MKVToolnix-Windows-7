#!/usr/bin/ruby -w

# T_636flac_with_id3_tags
describe "mkvmerge / read FLAC files with ID3 tags"
test_merge "data/flac/id3-tags.flac"
