#!/usr/bin/ruby -w

# T_700X_usf
describe "mkextract / USF subtitles"

test_merge "data/subtitles/usf/u.usf", :keep_tmp => true

test "USF extraction" do
  extract tmp, 0 => "#{tmp}-1"
  hash_file "#{tmp}-1"
end
