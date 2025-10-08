#!/usr/bin/ruby -w

# T_781no_bom
describe "mkvextract / disabling writing byte order markers"

file = "data/mkv/complex.mkv"

test "extraction with BOM" do
  extract file, 8 => tmp
  hash_file tmp
end

test "extraction without BOM" do
  extract file, 8 => tmp, :args => "--no-bom"
  hash_file tmp
end
