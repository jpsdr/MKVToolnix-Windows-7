#!/usr/bin/ruby -w
base = "data/simple/Datei didödeldü "

# T_552chapter_generation_appending_file_name_variables
describe "mkvmerge / chapter generation when appending; testing file name variables"
test_merge "'#{base}1.mp3' + '#{base}2.mp3' + '#{base}3.mp3' --generate-chapters when-appending --generate-chapters-name-template 'File name: <FILE_NAME>; with extension: <FILE_NAME_WITH_EXT>' > /dev/null"
