#!/usr/bin/ruby -w

# T_735normalize_language_ietf_chapters
describe "mkvmerge / normalize language IETF in chapters"

src = "--chapters data/chapters/ietf-normalization-test.xml"

test_merge src, :keep_tmp => true
compare_languages_chapters [ "fre", "fr-FX" ], [ "chi", "zh-yue" ], [ "chi", "yue" ]

test_merge src, :args => "--normalize-language-ietf canonical", :keep_tmp => true
compare_languages_chapters [ "fre", "fr-FR" ], [ "chi", "yue" ], [ "chi", "yue" ]


test_merge src, :args => "--normalize-language-ietf extlang", :keep_tmp => true
compare_languages_chapters [ "fre", "fr-FR" ], [ "chi", "zh-yue" ], [ "chi", "zh-yue" ]
