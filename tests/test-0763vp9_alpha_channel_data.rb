#!/usr/bin/ruby -w

# T_763vp9_alpha_channel_data
describe "mkvmerge / VP9 with alpha channel data in BlockAdditions & 'video alpha mode' track header element"

file = "data/vp9/alpha_channel_data.mkv"

test_merge file
test "track header attribute" do
  identify_json(file)["tracks"][0]["properties"]["alpha_mode"] == true ? "OK" : "failed"
end
