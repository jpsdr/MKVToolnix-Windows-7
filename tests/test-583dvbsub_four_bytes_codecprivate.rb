#!/usr/bin/ruby -w

# T_583dvbsub_four_bytes_codecprivate
describe "mkvmerge / DVB subtitles in Matroska with CodecPrivate missing the subtitling type byte"

file = "data/subtitles/dvbsub/codecprivate_four_bytes.mkv"

test "codec private length" do
  identify_json(file)["tracks"].
    select { |t| t["codec"] == "DVBSUB" }.
    map    { |t| t["properties"]["codec_private_length"].to_s }.
    join('+')
end

test_merge file, :args => "-A -D"
