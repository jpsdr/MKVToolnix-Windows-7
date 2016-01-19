#!/usr/bin/ruby -w

# T_528handbrake_chapter_uids
describe "mkvmerge / re-generate edition & chapter UIDs for files created by HandBrake"
test_merge "data/mkv/handbrake-chapters-1.mkv + data/mkv/handbrake-chapters-2.mkv"
