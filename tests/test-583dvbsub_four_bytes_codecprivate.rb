#!/usr/bin/ruby -w

# T_583dvbsub_four_bytes_codecprivate
describe "mkvmerge / DVB subtitles in Matroska with CodecPrivate missing the subtitling type byte"

file = "data/subtitles/dvbsub/codecprivate_four_bytes.mkv"

test_identify file
test_merge file
