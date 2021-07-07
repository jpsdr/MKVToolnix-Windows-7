#!/usr/bin/ruby -w

# T_660propedit_replace_one_byte_with_ebmlvoid_all_surrounding_elements_coded_size_length_8
describe "mkvpropedit / replacing one byte with EBML void while surrounding elements use 8-byte long coded size lengths"
test "propedit zero-length ebml void" do
  sys "cp data/mkv/sample-bug2406.mkv #{tmp}"
  propedit tmp, "-e track:v1 -s interlaced=1"
  sys "../src/mkvinfo -v -v #{tmp} > /dev/null"
  hash_tmp
end
