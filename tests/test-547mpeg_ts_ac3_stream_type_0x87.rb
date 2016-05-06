#!/usr/bin/ruby -w

# T_547mpeg_ts_ac3_stream_type_0x87
describe "mkvmerge / MPEG TS with (E-)AC-3 with a stream type of 0x87"

file = "data/ts/ac3_stream_type_0x87.ts"

test_identify file
test "number of (E-)AC-3 tracks" do
  identify_json(file)["tracks"].select { |track| (track["type"] == "audio") && %r{AC-3}.match(track["codec"]) }.count
end
