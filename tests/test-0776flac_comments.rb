#!/usr/bin/ruby -w

# T_776flac_comments
describe "mkvmerge / FLAC: handling of comments"

file_name = "data/flac/comments.flac"

test_identify file_name
test_merge file_name
test_merge file_name, :args => "--no-track-tags"
