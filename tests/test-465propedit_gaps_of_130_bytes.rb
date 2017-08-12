#!/usr/bin/ruby -w

describe "mkvpropedit / create voids for 130 bytes long gaps"

test "propedit" do
  cp "data/mkv/propedit-gaps-130-bytes.mkv", tmp
  propedit tmp, "--edit track:s1 --set flag-default=0"
  hash_tmp
end
