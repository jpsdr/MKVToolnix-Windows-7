#!/usr/bin/ruby -w

# T_709bcp47_mkvmerge_tags
describe "mkvmerge / BCP 47/RFC 5646 language tags: tags"

test_merge "data/subtitles/srt/vde-utf-8-bom.srt", :args => "--tags 0:data/text/tags-language-ietf.xml"
