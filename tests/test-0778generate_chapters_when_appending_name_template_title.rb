#!/usr/bin/ruby -w

# T_778generate_chapters_when_appending_name_template_title
describe "mkvmerge / generating chapters when appending, placeholder <TITLE> in chapter name template"

src    = "data/simple/v.mp3"
title1 = "one title"
title2 = "oh YEAH another title"

test_merge src, :output => "#{tmp}1", :args => "--title '#{title1}'", :keep_tmp => true
test_merge src, :output => "#{tmp}2", :args => "--title '#{title2}'", :keep_tmp => true
test_merge "#{tmp}1 + #{tmp}2", :output => "#{tmp}3", :args => "--generate-chapters when-appending --generate-chapters-name-template '<NUM>:<TITLE>' > /dev/null", :keep_tmp => true

test "chapter titles" do
  expected_names = [ "1:#{title1}", "2:#{title2}" ]
  actual_names   = extract("#{tmp}3", :mode => :chapters, :args => "--simple -")[0].
    select { |line| %r{NAME=}.match(line) }.
    map    { |line| line.gsub(%r{.*NAME=}, '').chomp }

  fail "expected names do not match actual names" if expected_names != actual_names

  :ok
end
