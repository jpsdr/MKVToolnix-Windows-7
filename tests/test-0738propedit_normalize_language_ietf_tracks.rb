#!/usr/bin/ruby -w

# T_738propedit_normalize_language_ietf_tracks
describe "mkvpropedit / normalize language IETF in tracks"

def test0738 expected_languages, normalization_mode = nil
  src = "data/subtitles/srt/ven.srt"

  test_merge src, :args => "#{src} #{src}", :keep_tmp => true

  test "propedit #{expected_languages}" do
    normalization_mode = normalization_mode ? "--normalize-language-ietf #{normalization_mode}" : ""

    propedit tmp, "--edit track:1 --set language=fr-fx --edit track:2 --set language=zh-yue --edit track:3 --set language=yue #{normalization_mode}"
    hash_tmp false
  end

  compare_languages_tracks(*expected_languages)
end

test0738 [ [ "fre", "fr-FX" ], [ "chi", "zh-yue" ], [ "chi", "yue" ] ]
test0738 [ [ "fre", "fr-FR" ], [ "chi", "yue" ],    [ "chi", "yue" ] ],    :canonical
test0738 [ [ "fre", "fr-FR" ], [ "chi", "zh-yue" ], [ "chi", "zh-yue" ] ], :extlang
