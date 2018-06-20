#!/usr/bin/ruby -w

codes = {
  "iw"  => "heb",
  "scr" => "hrv",
  "scc" => "srp",
  "mol" => "rum",
}

describe "mkvmerge / deprecated ISO 639-1/2 codes"

codes.each do |deprecated_code, expected_code|
  test_merge "data/subtitles/srt/vde.srt", :args => "--language 0:#{deprecated_code} --sub-charset 0:iso-8859-15", :keep_tmp => true
  test "check code #{deprecated_code} => #{expected_code}" do
    identify_json(tmp)["tracks"][0]["properties"]["language"] == expected_code ? "good" : "bad"
  end
end
