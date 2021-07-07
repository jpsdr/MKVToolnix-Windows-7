#!/usr/bin/ruby -w

def doit file_name
  test file_name do
    merge file_name, output: "#{tmp}-1", keep_tmp: true
    extract "#{tmp}-1", 0 => tmp
    hash_tmp
  end
end

# T_544X_webvtt
describe "mkvextract / WebVTT subtitles"

dir = "data/subtitles/webvtt"

(1..6).each { |idx| doit sprintf("%s/w3c-%03d.vtt", dir, idx) }
(1..2).each { |idx| doit sprintf("%s/mo-%03d.vtt", dir, idx) }
