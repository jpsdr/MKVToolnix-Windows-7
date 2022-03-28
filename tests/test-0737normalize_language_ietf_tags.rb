#!/usr/bin/ruby -w

# T_735normalize_language_ietf_tags
describe "mkvmerge / normalize language IETF in tags"

src = "--global-tags data/tags/ietf-normalization-test.xml"

test_merge src, :args => "--normalize-language-ietf off", :keep_tmp => true
compare_languages_tags [ "fre", "fr-FX" ], [ "chi", "zh-yue" ], [ "chi", "yue" ]

test_merge src, :keep_tmp => true
compare_languages_tags [ "fre", "fr-FR" ], [ "chi", "yue" ], [ "chi", "yue" ]


test_merge src, :args => "--normalize-language-ietf extlang", :keep_tmp => true
compare_languages_tags [ "fre", "fr-FR" ], [ "chi", "zh-yue" ], [ "chi", "zh-yue" ]
