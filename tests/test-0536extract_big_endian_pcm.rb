#!/usr/bin/ruby -w

# T_536extract_big_endian_pcm
describe "mkvextract / extract Big Endian PCM"

test "extraction" do
  extract "data/mkv/big-endian-pcm.mka", 0 => tmp
  hash_tmp
end
