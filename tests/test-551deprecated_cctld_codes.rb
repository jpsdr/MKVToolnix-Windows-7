#!/usr/bin/ruby -w

# T_551deprecated_cctld_codes
describe "mkvmerge, mkvextract / deprecated ccTLDs"

test_merge "data/subtitles/srt/ven.srt", :args => "--chapters data/chapters/uk-and-gb.xml"
test_merge "data/mkv/chapters-uk-and-gb.mks"

test "fix during extraction" do
  extract "data/mkv/chapters-uk-and-gb.mks", :args => tmp, :mode => :chapters
  hash_tmp
end
