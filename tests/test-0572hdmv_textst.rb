#!/usr/bin/ruby -w

# T_572hdmv_textst
describe "mkvmerge & mkvetract / HDMV TextST subtitles"

test_merge "data/subtitles/hdmv-textst/vanhelsing-00147.m2ts", :output => "#{tmp}-1", :keep_tmp => true
test_merge "#{tmp}-1",                                         :output => "#{tmp}-2", :keep_tmp => true

test "extraction & re-mux" do
  extract "#{tmp}-1", 0 => "#{tmp}-3"
  merge "#{tmp}-3", :output => "#{tmp}-4"

  hash_file("#{tmp}-3") + "+" + hash_file("#{tmp}-4")
end
