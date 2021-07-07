#!/usr/bin/ruby -w

# T_603mpeg_ps_ac3_not_enough_data_in_first_packet
describe "mkvmerge / MPEG PS, AC-3 track with too little data in first PS packet"

file = "data/vob/ac3_not_enough_data_in_first_packet.vob"

test "types" do
  json = identify_json file

  json["tracks"].
    select { |t| t["type"] == "audio" }.
    map    { |t| [ t["id"], t["properties"]["stream_id"], t["properties"]["sub_stream_id"], t["codec"].gsub(/-/, '_') ].map(&:to_s).join('+') }.
    join('-')
end
