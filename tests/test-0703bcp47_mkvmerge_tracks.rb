#!/usr/bin/ruby -w

# T_703bcp47_mkvmerge_tracks
describe "mkvmerge / BCP 47/RFC 5646 language tags: tracks"

def compare_languages *expected_languages
  test "#{expected_languages}" do
    if expected_languages[0].is_a?(String)
      file_name = expected_languages.shift
    else
      file_name = tmp
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

  unlink_tmp_files
end

src1 = "data/subtitles/srt/vde-utf-8-bom.srt"
src2 = "data/avi/v.avi"
src3 = "data/mkv/complex.mkv"

# One track in file, not Matroska
test_merge src1, :keep_tmp => true
compare_languages %w{und und}

test_merge src1, :keep_tmp => true, :args => "--language 0:de-latn-de"
compare_languages %w{ger de-Latn-DE}

test_merge src1, :keep_tmp => true, :args => "--language -1:fr-latn-fr"
compare_languages %w{fre fr-Latn-FR}

test_merge src1, :keep_tmp => true, :args => "--default-language sr-CYRL-rs"
compare_languages %w{srp sr-Cyrl-RS}

test_merge src1, :keep_tmp => true, :args => "--language 0:x-foo-muh2kuh"
compare_languages %w{und x-foo-muh2kuh}

# Two tracks in file, not Matroska
test_merge src2, :keep_tmp => true
compare_languages %w{und und}, %w{und und}

test_merge src2, :keep_tmp => true, :args => "--language 0:de-latn-de --language 1:pt-br"
compare_languages %w{ger de-Latn-DE}, %w{por pt-BR}

test_merge src2, :keep_tmp => true, :args => "--language -1:fr-latn-fr"
compare_languages %w{fre fr-Latn-FR}, %w{fre fr-Latn-FR}

test_merge src2, :keep_tmp => true, :args => "--default-language sr-CYRL-rs"
compare_languages %w{srp sr-Cyrl-RS}, %w{srp sr-Cyrl-RS}

test_merge src2, :keep_tmp => true, :args => "--language 0:x-foo-muh2kuh --language 1:es-MX"
compare_languages %w{und x-foo-muh2kuh}, %w{spa es-MX}

# Matroska with legacy elements only
compare_languages(src3, *([ [ "und", nil ] ] * 10))

test_merge src3, :keep_tmp => true
compare_languages(*([ %w{und und} ] * 10))

test_merge src3, :keep_tmp => true, :args => "--language -1:eng"
compare_languages(*([ %w{eng en} ] * 10))

test_merge src3, :keep_tmp => true, :args => "--language 0:de-latn-de --language 1:pt-br"
compare_languages(*([ %w{ger de-Latn-DE}, %w{por pt-BR} ] + [ %w{und und} ] * 8))

test_merge src3, :keep_tmp => true, :args => "--language -1:fr-latn-fr"
compare_languages(*([ %w{fre fr-Latn-FR} ] * 10))

test_merge src3, :keep_tmp => true, :args => "--default-language sr-CYRL-rs"
compare_languages(*([ %w{und und} ] * 10))

test_merge src3, :keep_tmp => true, :args => "--language 0:x-foo-muh2kuh --language 1:es-MX"
compare_languages(*([ %w{und x-foo-muh2kuh}, %w{spa es-MX} ] + [ %w{und und} ] * 8))

# Matroska with both language elements
test_merge src2, :keep_tmp => true, :args => "--language 0:de-latn-de --language 1:pt-br"
test_merge tmp,  :keep_tmp => true, :output => "#{tmp}-2"
compare_languages "#{tmp}-2", %w{ger de-Latn-DE}, %w{por pt-BR}

test_merge src2, :keep_tmp => true, :args => "--language 0:de-latn-de --language 1:pt-br"
test_merge tmp,  :keep_tmp => true, :args => "--language 0:fr-ch --language 1:dan", :output => "#{tmp}-2"
compare_languages "#{tmp}-2", %w{fre fr-CH}, %w{dan da}

# Track selection
test_merge src2, :keep_tmp => true, :args => "--language 0:de-latn-de --language 1:pt-br"
test_merge tmp,  :keep_tmp => true, :args => "--atracks pt-br", :output => "#{tmp}-2"
compare_languages "#{tmp}-2", %w{ger de-Latn-DE}, %w{por pt-BR}

test_merge src2, :keep_tmp => true, :args => "--language 0:de-latn-de --language 1:pt-br"
test_merge tmp,  :keep_tmp => true, :args => "--atracks pt", :output => "#{tmp}-2"
compare_languages "#{tmp}-2", %w{ger de-Latn-DE}, %w{por pt-BR}

test_merge src2, :keep_tmp => true, :args => "--language 0:de-latn-de --language 1:pt-br"
test_merge tmp,  :keep_tmp => true, :args => "--atracks !pt-br", :output => "#{tmp}-2"
compare_languages "#{tmp}-2", %w{ger de-Latn-DE}

test_merge src2, :keep_tmp => true, :args => "--language 0:de-latn-de --language 1:pt-br"
test_merge tmp,  :keep_tmp => true, :args => "--atracks !pt", :output => "#{tmp}-2"
compare_languages "#{tmp}-2", %w{ger de-Latn-DE}
