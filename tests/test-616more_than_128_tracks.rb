#!/usr/bin/ruby -w

# T_616more_than_128_tracks
describe "mkvmerge / more than 128 tracks"

input = (1..130).map { |i| "data/subtitles/srt/ven.srt" }.join(" ")
test_merge input, :output => "#{tmp}-1", :keep_tmp => true
test_merge "#{tmp}-1", :output => "#{tmp}-2"
