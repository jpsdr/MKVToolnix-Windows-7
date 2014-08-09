#!/usr/bin/ruby -w

# T_433matroska_no_track_uid
describe "mkvmerge / Matroska files missing the track UID element"
test_merge "data/mkv/no-track-uid.mks", :exit => :warning
