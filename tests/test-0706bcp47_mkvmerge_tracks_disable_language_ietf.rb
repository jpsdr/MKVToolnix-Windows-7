#!/usr/bin/ruby -w

# T_706bcp47_mkvmerge_tracks_disable_language_ietf
describe "mkvmerge / BCP 47/RFC 5646 language tags: tracks (--disable-language-ietf)"

def compare_languages *expected_languages
  do_unlink = true

  test "#{expected_languages}" do
    if expected_languages[0].is_a?(String)
      file_name = expected_languages.shift
    else
      file_name = tmp
    end

    if expected_languages[0] == false
      do_unlink = false
      expected_languages.shift
    end

    tracks = identify_json(file_name)["tracks"]

    fail "track number different: actual #{tracks.size} expected #{expected_languages.size}" if tracks.size != expected_languages.size

    (0..tracks.size - 1).each do |idx|
      props = tracks[idx]["properties"]

      if props["language"] != expected_languages[idx][0]
        fail "track #{idx} actual language #{props["language"]} != expected #{expected_languages[idx][0]}"
      end

      if props["language_ietf"] != expected_languages[idx][1]
        fail "track #{idx} actual language_ietf #{props["language_ietf"]} != expected #{expected_languages[idx][1]}"
      end
    end

    "ok"
  end

  unlink_tmp_files if do_unlink
end

src1 = "data/subtitles/srt/vde-utf-8-bom.srt"

test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf"
compare_languages ["und", nil]

test_merge src1, :keep_tmp => true
compare_languages ["und", "und"]

test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf --language 0:de-latn-de"
compare_languages [ "ger", nil ]

test_merge src1, :keep_tmp => true, :args => "--language 0:de-latn-de"
test_merge tmp,  :keep_tmp => true, :args => "--disable-language-ietf", :output => "#{tmp}-2"
test_merge tmp,  :keep_tmp => true, :args => "--disable-language-ietf --language 0:es-MX", :output => "#{tmp}-3"

compare_languages tmp,        false, [ "ger", "de-Latn-DE" ]
compare_languages "#{tmp}-2", false, [ "ger", nil ]
compare_languages "#{tmp}-3",        [ "spa", nil ]

unlink_tmp_files

test_merge src1, :keep_tmp => true, :post_args => "--disable-language-ietf"
compare_languages ["und", nil]

test_merge src1, :keep_tmp => true
compare_languages ["und", "und"]

test_merge src1, :keep_tmp => true, :args => "--language 0:de-latn-de", :post_args => "--disable-language-ietf"
compare_languages [ "ger", nil ]

test_merge src1, :keep_tmp => true, :args => "--language 0:de-latn-de"
test_merge tmp,  :keep_tmp => true, :post_args => "--disable-language-ietf", :output => "#{tmp}-2"
test_merge tmp,  :keep_tmp => true, :args => "--language 0:es-MX", :post_args => "--disable-language-ietf", :output => "#{tmp}-3"

compare_languages tmp,        false, [ "ger", "de-Latn-DE" ]
compare_languages "#{tmp}-2", false, [ "ger", nil ]
compare_languages "#{tmp}-3",        [ "spa", nil ]
