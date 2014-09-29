#!/usr/bin/ruby -w
# -*- coding: utf-8 -*-

# T_440chapter_display_language_default_value
describe "mkvmerge / Default value for »language« element in chapter XML files"
test_merge "data/subtitles/srt/ven.srt", :args => "--chapters data/text/chaperts-no-language-element.xml"
