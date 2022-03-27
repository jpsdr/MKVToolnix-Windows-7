#!/usr/bin/ruby -w

# T_706bcp47_mkvmerge_tracks_disable_language_ietf
describe "mkvmerge / BCP 47/RFC 5646 language tags: tracks (--disable-language-ietf)"

src1 = "data/subtitles/srt/vde-utf-8-bom.srt"

test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf"
compare_languages_tracks ["und", nil]

test_merge src1, :keep_tmp => true
compare_languages_tracks ["und", "und"]

test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf --language 0:de-latn-de"
compare_languages_tracks [ "ger", nil ]

test_merge src1, :keep_tmp => true, :args => "--language 0:de-latn-de"
test_merge tmp,  :keep_tmp => true, :args => "--disable-language-ietf", :output => "#{tmp}-2"
test_merge tmp,  :keep_tmp => true, :args => "--disable-language-ietf --language 0:es-MX", :output => "#{tmp}-3"

compare_languages_tracks tmp,        [ "ger", "de-Latn-DE" ], :keep_tmp => true
compare_languages_tracks "#{tmp}-2", [ "ger", nil ],          :keep_tmp => true
compare_languages_tracks "#{tmp}-3", [ "spa", nil ]

unlink_tmp_files

test_merge src1, :keep_tmp => true, :post_args => "--disable-language-ietf"
compare_languages_tracks ["und", nil]

test_merge src1, :keep_tmp => true
compare_languages_tracks ["und", "und"]

test_merge src1, :keep_tmp => true, :args => "--language 0:de-latn-de", :post_args => "--disable-language-ietf"
compare_languages_tracks [ "ger", nil ]

test_merge src1, :keep_tmp => true, :args => "--language 0:de-latn-de"
test_merge tmp,  :keep_tmp => true, :post_args => "--disable-language-ietf", :output => "#{tmp}-2"
test_merge tmp,  :keep_tmp => true, :args => "--language 0:es-MX", :post_args => "--disable-language-ietf", :output => "#{tmp}-3"

compare_languages_tracks tmp,        [ "ger", "de-Latn-DE" ], :keep_tmp => true
compare_languages_tracks "#{tmp}-2", [ "ger", nil ],          :keep_tmp => true
compare_languages_tracks "#{tmp}-3", [ "spa", nil ]
