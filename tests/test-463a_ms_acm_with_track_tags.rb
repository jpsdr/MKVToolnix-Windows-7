#!/usr/bin/ruby -w

file = "data/mkv/a_ms_acm_with_track_tags.mka"

# T_463a_ms_acm_with_track_tags
describe "mkvmerge / Matroska Codec ID A_MS/ACM with track-specific tags"
test_merge file
test_merge file, :args => "--language 0:ger --track-name 0:chunkybacon"
