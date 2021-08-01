#!/usr/bin/ruby -w

# T_727chapters_ietf_language_without_639_2
describe "mkvmerge / chapters with IETF language attributes that don't contain a valid ISO 639-2 code"

Dir.glob("data/chapters/iso639-5-*.xml").sort.each do |file_name|
  test_merge "--chapters #{file_name}"
end
