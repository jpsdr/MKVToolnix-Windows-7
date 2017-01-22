#!/usr/bin/ruby -w

# T_579vobsub_in_matroska_without_codecprivate
describe "mkvmerge / VobSub in Matroska lacking CodecPrivate"

file = "data/vobsub/ffmpeg_vobsub_no_codecprivate.mkv"

test_identify file
test_merge file, :exit_code => :warning
