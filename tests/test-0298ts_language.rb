#!/usr/bin/ruby -w

# T_298ts_language
describe "mkvmerge / MPEG transport streams: language tags"

test "detected languages" do
  identify_json("data/ts/blue_planet.ts")["tracks"].
    map { |t| t["properties"]["language"] }.
    join("+")
end
