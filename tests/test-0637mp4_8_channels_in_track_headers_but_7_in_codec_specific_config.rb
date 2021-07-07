#!/usr/bin/ruby -w

# T_637mp4_8_channels_in_track_headers_but_7_in_codec_specific_config
describe "mkvmerge / MP4 AAC 8 channels in track headers but 7 in codec-specific config"
test "the file" do
  identify_json("data/mp4/8_channels_in_track_headers_but_7_in_codec_specific_config.m4a")["tracks"][0]["properties"]["audio_channels"] == 8
end
