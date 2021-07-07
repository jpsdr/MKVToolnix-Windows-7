#!/usr/bin/ruby -w

# T_428mkv_misdetected_as_ass
describe "mkvmerge / Matroska file mis-detected as ASS subtitles"
test_identify "data/mkv/detected-as-ass.mkv"
