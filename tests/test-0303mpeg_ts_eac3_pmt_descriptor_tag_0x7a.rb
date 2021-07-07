#!/usr/bin/ruby -w

# T_303mpeg_ts_eac3_pmt_descriptor_tag_0x7a
describe "mkvmerge / MPEG TS with E-AC-3 stored with PMT descriptor tag 0x7a"
test "detected tracks" do
  json_summarize_tracks(identify_json("data/ts/eac3_pmt_descriptor_tag_0x7a.ts"))
end
