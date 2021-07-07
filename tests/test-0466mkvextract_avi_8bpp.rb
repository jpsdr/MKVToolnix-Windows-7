#!/usr/bin/ruby -w

# T_466mkvextract_avi_8bpp
describe "mkvextract / writing AVIs with 8 bits/pixel in BITMAPINFOHEADER"

test "extraction" do
  extract "data/mkv/cram.mkv", 0 => tmp
  hash_tmp
end
