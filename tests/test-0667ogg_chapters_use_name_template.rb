#!/usr/bin/ruby -w

# T_667ogg_chapters_use_name_template
describe "mkvmerge / Ogg chapters without names should use the name template"
test_merge "", :args => "--chapters data/chapters/no-names.txt"
test_merge "", :args => "--chapters data/chapters/no-names.txt --generate-chapters-name-template 'Muh <NUM>' > /dev/null"
