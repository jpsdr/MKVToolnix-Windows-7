#!/usr/bin/ruby -w

# T_728chapters_keep_languages_unique
describe "mkvmerge / chapter languages: ensure each value written is unique within the scope of the same display"

test_merge "--chapters data/chapters/multi-language-same-values.xml"
