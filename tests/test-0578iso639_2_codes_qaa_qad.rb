#!/usr/bin/ruby -w

# T_578iso639_2_codes_qaa_qad
describe "mkvmerge / ISO 639-2 codes qaa and qad"

test "set and keep languages with mkvmerge" do
  result = []

  merge "--language 0:qaa data/aac/v.aac --language 0:qad data/aac/v.aac"
  result += identify_json(tmp)["tracks"].map { |t| t["properties"]["language"] }

  merge tmp, :output => "#{tmp}-2"
  result += identify_json("#{tmp}-2")["tracks"].map { |t| t["properties"]["language"] }

  result.join('+')
end

test "set languages with mkvpropedit" do
  result = []

  merge "data/aac/v.aac data/aac/v.aac"
  result += identify_json(tmp)["tracks"].map { |t| t["properties"]["language"] }

  propedit tmp, "--edit track:a1 --set language=qaa --edit track:a2 --set language=qad"
  result += identify_json(tmp)["tracks"].map { |t| t["properties"]["language"] }

  result.join('+')
end
