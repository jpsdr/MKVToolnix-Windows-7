#!/usr/bin/ruby -w

# T_623text_files_utf16le_different_line_endings
describe "mkvmerge / text files encoded in UTF-16LE with different line ending styles"
test_merge "data/subtitles/srt/utf16le_different_line_ending_styles.srt"
