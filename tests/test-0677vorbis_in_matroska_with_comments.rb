#!/usr/bin/ruby -w

# T_677vorbis_in_matroska_with_comments
describe "mkvmerge / Vorbis in Matroska with Vorbis stream comments"

skip_if $is_windows

test_merge "data/mkv/vorbis-with-comments-and-cover-image.mka"
test_merge "data/mkv/vorbis-with-comments-and-cover-image.mka", :args => "--no-attachments"
test_merge "data/mkv/vorbis-with-comments-and-cover-image.mka", :args => "--no-track-tags"
test_merge "data/mkv/vorbis-with-comments-and-cover-image.mka", :args => "--attachments 1"
test_merge "data/mkv/vorbis-with-comments-and-cover-image.mka", :args => "--attachments 2"
