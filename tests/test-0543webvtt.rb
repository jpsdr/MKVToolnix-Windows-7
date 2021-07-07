#!/usr/bin/ruby -w

# T_543webvtt
describe "mkvmerge / WebVTT subtitles"

dir = "data/subtitles/webvtt"

(1..6).each { |idx| test_merge sprintf("%s/w3c-%03d.vtt", dir, idx) }
(1..2).each { |idx| test_merge sprintf("%s/mo-%03d.vtt", dir, idx) }
