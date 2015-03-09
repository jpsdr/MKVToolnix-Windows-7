#!/usr/bin/ruby -w

# T_472flv_headers_signal_no_tracks
describe "mkvmerge / FlashVideo files whose header signal no tracks"

(1..2).each { |idx| test_merge "data/flv/headers-signal-no-tracks-#{idx}.flv" }
