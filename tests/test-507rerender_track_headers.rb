#!/usr/bin/ruby -w

# T_507rerender_track_headers
describe "mkvmerge / rerender track headers"

test_merge "data/h265/rerender-track-headers-broken.hevc"
test_merge "data/mkv/complex.mkv", :args => "--debug textsubs_force_rerender=8:16"
