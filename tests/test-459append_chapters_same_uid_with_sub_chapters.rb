#!/usr/bin/ruby -w

# T_459append_chapters_same_uid_with_sub_chapters
describe "mkvmerge / append chapters with the same UIDs containing sub-chapters"
test_merge "data/mkv/split-with-sub-chapters-001.mka + data/mkv/split-with-sub-chapters-002.mka"
