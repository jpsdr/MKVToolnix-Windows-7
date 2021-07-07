#!/usr/bin/ruby -w

# T_541dts_hd_ma_reduce_to_core_channels
describe "mkvmerge / channel count when reducing DTS HD MA to its core"
test_merge "data/dts/dts-hd-ma.mka", args: "--reduce-to-core 0", keep_tmp: true
test "channel count" do
  identify_json(tmp)["tracks"][0]["properties"]["audio_channels"]
end
