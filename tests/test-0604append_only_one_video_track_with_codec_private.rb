#!/usr/bin/ruby -w

# T_604append_only_one_video_track_with_codec_private
describe "mkvmerge / appending with only one video track having a CodecPrivate element"
test_merge "data/mkv/append_only_one_codecprivate_1.mkv + data/mkv/append_only_one_codecprivate_2.mkv", :exit_code => :warning
