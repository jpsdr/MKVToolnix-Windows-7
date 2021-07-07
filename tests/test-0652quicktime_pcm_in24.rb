#!/usr/bin/ruby -w

# T_652quicktime_pcm_in24
describe "mkvmerge / QuickTime with PCM with FourCC 'in24'"

file = "data/mp4/prores_apch-and-pcm_in24-short.mov"

test_merge file, :args => "--no-video", :keep_tmp => true
test "bitdepth is 24" do
  identify_json(file)["tracks"][1]["properties"]["audio_bits_per_sample"] == 24
end
