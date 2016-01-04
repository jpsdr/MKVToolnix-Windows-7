#!/usr/bin/ruby -w

# T_525truehd_doesnt_start_with_sync_frame
describe "mkvmerge / TrueHD recognition if the stream doesn't start with a TrueHD sync frame"
test_identify "data/truehd/truehd-not-starting-with-sync-frame.thd"
