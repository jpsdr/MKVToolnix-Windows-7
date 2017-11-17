#!/usr/bin/ruby -w

file = "data/aac/7_1_channels_no_pce.aac"

# T_622aac_adts_8_channels_no_pce
describe "mkvmerge / ADTS AAC files with 7.1 channels without a program config element"

test_merge file

test "identification" do
  fail unless identify_json(file)["tracks"][0]["properties"]["audio_channels"] == 8
  "ok"
end
