#!/usr/bin/ruby -w

# T_686container_wrong_num_channels
describe "mkvmerge / ALAC in MP4 with wrong number of channels in `stsd` atom"
test "identification" do
  identify_json("data/alac/container_wrong_num_channels.m4a")["tracks"][0]["properties"]["audio_channels"] == 6
end
