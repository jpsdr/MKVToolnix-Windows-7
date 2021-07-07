#!/usr/bin/ruby -w

# T_468extract_cues
describe "mkvextract / extract cues"
test_merge "data/avi/v-h264-aac.avi data/subtitles/srt/ven.srt", :keep_tmp => true
test "extraction" do
  extract tmp, :mode => :cues, 0 => "#{tmp}-0", 2 => "#{tmp}-2"
  [0, 2].collect { |idx| hash_file "#{tmp}-#{idx}" }.join('+')
end
