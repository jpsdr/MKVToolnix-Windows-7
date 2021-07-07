#!/usr/bin/ruby -w

# T_708bcp47_propedit_language_ietf_disable_language_ietf
describe "mkvpropedit / language-ietf (--disable-language-ietf)"

def verify_languages language, language_ietf
  props  = identify_json(tmp)["tracks"][0]["properties"]
  result = (props["language"] == language) && (props["language_ietf"] == language_ietf) ? "ok" : "bad"

  [ props["language"], (props["language_ietf"] || '').gsub(%r{-}, '_'), result ]
end

test_merge "data/subtitles/srt/vde-utf-8-bom.srt", :keep_tmp => true

test "language-ietf" do
  result = []
  result += verify_languages("und", "und")

  propedit tmp, "--disable-language-ietf --edit track:1 --set language=de-CH"
  result += verify_languages("ger", "und")

  propedit tmp, "--disable-language-ietf --edit track:1 --set language-ietf=pt-BR"
  result += verify_languages("ger", "pt-BR")

  propedit tmp, "--disable-language-ietf --edit track:1 --set language=es-MX"
  result += verify_languages("spa", "pt-BR")

  propedit tmp, "--disable-language-ietf --edit track:1 --delete language"
  result += verify_languages("eng", "pt-BR")

  propedit tmp, "--disable-language-ietf --edit track:1 --delete language-ietf"
  result += verify_languages("eng", nil)

  result.join('+')
end
