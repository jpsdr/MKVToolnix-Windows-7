#!/usr/bin/ruby -w

# T_721mutli_chapter_language
src      = "data/subtitles/srt/ven.srt"
chapters = "data/chapters/multi-language-in-display.xml"

describe "mkvmerge & mkvpropedit / chapter display with multiple chapter language, one of them 'eng'"

test_merge src, :args => "--chapters #{chapters}"
test_merge src, :keep_tmp => true

test "mkvpropedit" do
  propedit tmp, "--chapters #{chapters}"
  hash_tmp
end
