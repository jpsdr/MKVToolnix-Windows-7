#!/usr/bin/ruby -w

file = "data/mkv/invalid_track_language_elements.mkv"

# T_590invalid_track_language_elements
describe "mkvmerge / Matroska files with invalid track language elements"

test_merge file
test_merge file, :args => "--language 0:ger --language 1:fre"
test_identify file
