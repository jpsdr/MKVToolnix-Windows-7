#!/usr/bin/ruby -w

# T_694fix_tags_when_regenerating_chapter_uids
describe "mkvmerge / regenerate chapter UIDs but fix tag chapter UIDs in the process"
test_merge "data/mkv/chapters_tags_max_size_field_size.mkv"
