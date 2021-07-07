#!/usr/bin/ruby -w

# T_647recode_textsubs_from_matroska
describe "mkvmerge / recode text subtitles read from Matroska files"

file = "data/mkv/attachments.mkv"

test_merge file, :args => "--no-attachments", :exit_code => :warning
test_merge file, :args => "--no-attachments --sub-charset 0:iso-8859-1"
