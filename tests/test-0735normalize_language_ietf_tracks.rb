#!/usr/bin/ruby -w

# T_735normalize_language_ietf_tracks
describe "mkvmerge / normalize language IETF in tracks"

src = "data/avi/v.avi"

test_merge src, :args => "--language 0:fr-fx --language 1:zh-yue", :keep_tmp => true
compare_languages_tracks [ "fre", "fr-FX" ], [ "chi", "zh-yue" ]

test_merge src, :args => "--language 0:fr-fx --language 1:zh-yue --normalize-language-ietf canonical", :keep_tmp => true
compare_languages_tracks [ "fre", "fr-FR" ], [ "chi", "yue" ]


test_merge src, :args => "--language 0:fr-fx --language 1:yue --normalize-language-ietf extlang", :keep_tmp => true
compare_languages_tracks [ "fre", "fr-FR" ], [ "chi", "zh-yue" ]
