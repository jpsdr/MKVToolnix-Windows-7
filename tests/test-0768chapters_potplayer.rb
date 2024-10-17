#!/usr/bin/ruby -w

# T_768chapters_potplayer
describe "mkvmerge / chapters from PotPlayer bookmark files"

Dir.glob("data/chapters/potplayer-bookmarks/*.pbf").sort.each do |file_name|
  test_merge "data/subtitles/srt/ven.srt", :args => "--chapters #{file_name}"
end
