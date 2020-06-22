#!/usr/bin/ruby -w

# T_697dts_es_xch
describe "mkvmerge / channel count of DTS-ES with XCh extension"
test "identification" do
  identify_json("data/dts/dts-es-xch.dts")["tracks"][0]["properties"]["audio_channels"] == 7
end
