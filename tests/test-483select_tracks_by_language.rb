#!/usr/bin/ruby -w

# T_483select_tracks_by_language
describe "mkvmerge / select tracks by language"
test "select-tracks-by-language" do
  cs = "--sub-charset 0:iso-8859-15"
  merge "--language 0:ger #{cs} data/subtitles/srt/vde.srt --language 0:eng #{cs} data/subtitles/srt/ven.srt --language 0:fre #{cs} data/subtitles/srt/vde.srt ", :keep_tmp => true, :output => "#{tmp}-0"
  merge "--stracks 0,2 #{tmp}-0", :output => "#{tmp}-1"
  merge "--stracks ger,fre #{tmp}-0", :output => "#{tmp}-2"
  merge "--stracks 0,fre #{tmp}-0", :output => "#{tmp}-3"
  merge "--stracks 0,ger,eng #{tmp}-0", :output => "#{tmp}-4"

  (0..4).collect { |idx| hash_file("#{tmp}-#{idx}") }.join("+")
end
