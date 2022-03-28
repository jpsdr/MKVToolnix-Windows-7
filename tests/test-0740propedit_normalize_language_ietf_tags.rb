#!/usr/bin/ruby -w

# T_740propedit_normalize_language_ietf_tags
describe "mkvpropedit / normalize language IETF in tags"

def test0740 expected_languages, normalization_mode = nil
  src1 = "data/subtitles/srt/ven.srt"
  src2 = "data/tags/one-tag-fr-FX.xml"
  src3 = "data/tags/ietf-normalization-test.xml"

  test_merge src1, :args => "--normalize-language-ietf off --tags 0:#{src2}", :keep_tmp => true

  test "propedit #{expected_languages}" do
    normalization_mode = normalization_mode ? "--normalize-language-ietf #{normalization_mode}" : ""

    propedit tmp, "--tags global:#{src3} #{normalization_mode}"
    hash_tmp false
  end

  compare_languages_tags(*expected_languages)
end

test0740 [ [ "fre", "fr-FX" ], [ "fre", "fr-FX" ], [ "chi", "zh-yue" ], [ "chi", "yue" ] ],    :off
test0740 [ [ "fre", "fr-FX" ], [ "fre", "fr-FR" ], [ "chi", "yue" ],    [ "chi", "yue" ] ]
test0740 [ [ "fre", "fr-FX" ], [ "fre", "fr-FR" ], [ "chi", "zh-yue" ], [ "chi", "zh-yue" ] ], :extlang
