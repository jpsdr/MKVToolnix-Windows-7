#!/usr/bin/ruby -w

# T_696wavpack5
describe "mkvmerge / WavPack files created by v5 of the program"

test_merge "data/wavpack4/v5.wv", :keep_tmp => true

test "extraction" do
  extract tmp, 0 => "#{tmp}-wv"
  md5("#{tmp}-wv")
end

test_merge "#{tmp}-wv"
