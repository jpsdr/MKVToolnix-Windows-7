#!/usr/bin/ruby -w

# T_584ac_3_misdetected_as_mp3
describe "mkvmerge / AC-3 misdetected as MP3"

file = "data/ac3/misdetected_as_mp3.ac3"

test "identification" do
  identify_json(file)["container"]["type"] == "AC-3"
end

test_merge file
