#!/usr/bin/ruby -w

describe "mkvpropedit / editing files without a track UID"
test "data/mkv/no-track-uid.mks" do
  hashes = []

  sys "cp data/mkv/no-track-uid.mks #{tmp}"
  hashes << hash_tmp(false)

  propedit "#{tmp} --edit track:s1 --set name=chunky-bacon"
  hashes << hash_tmp(false)

  hashes.join '-'
end
