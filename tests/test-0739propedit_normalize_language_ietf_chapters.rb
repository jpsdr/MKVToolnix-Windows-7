#!/usr/bin/ruby -w

# T_739propedit_normalize_language_ietf_chapters
describe "mkvpropedit / normalize language IETF in chapters"

def test0739 expected_languages, normalization_mode = nil
  src1 = "data/subtitles/srt/ven.srt"
  src2 = "data/chapters/ietf-normalization-test.xml"

  test_merge src1, :keep_tmp => true

  test "propedit #{expected_languages}" do
    normalization_mode = normalization_mode ? "--normalize-language-ietf #{normalization_mode}" : ""

    propedit tmp, "--chapters #{src2} #{normalization_mode}"
    hash_tmp false
  end

  compare_languages_chapters(*expected_languages)
end

test0739 [ [ "fre", "fr-FX" ], [ "chi", "zh-yue" ], [ "chi", "yue" ] ]
test0739 [ [ "fre", "fr-FR" ], [ "chi", "yue" ],    [ "chi", "yue" ] ],    :canonical
test0739 [ [ "fre", "fr-FR" ], [ "chi", "zh-yue" ], [ "chi", "zh-yue" ] ], :extlang
