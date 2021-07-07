#!/usr/bin/ruby -w

# T_650chapter_generation_no_names
describe "mkvmerge / generate chapters without names"

test_merge "data/simple/v.mp3", args: "--generate-chapters interval:10s --generate-chapters-name-template ''", :keep_tmp => true
test :chapter_names do
  extract tmp, :args => "#{tmp}-2", :mode => :chapters
  content = IO.readlines("#{tmp}-2").join("")
  ok      = %r{ChapterAtom}.match(content)  && !%r{Chapter(Display|String|Language)}.match(content)
  fail "chapter content is bad" if !ok
  "ok"
end
