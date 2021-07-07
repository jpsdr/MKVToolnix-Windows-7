#!/usr/bin/ruby -w

# T_526propedit_bibliographic_and_terminology_iso_639_2
describe "mkvpropedit / using bibliographic and terminology variants of ISO 639-2 codes"

test_merge "data/avi/v.avi", :keep_tmp => true
test "bibliographic" do
  propedit tmp, "--edit track:a1 --set language=ger"
  hash_tmp
end

test_merge "data/avi/v.avi", :keep_tmp => true
test "terminology" do
  propedit tmp, "--edit track:a1 --set language=deu"
  hash_tmp
end
