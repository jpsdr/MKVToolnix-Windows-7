#!/usr/bin/ruby -w

# T_725hevc_invalid_vps
describe "FIXTHIS"

file = "data/h265/hevc_invalid_vps.mkv"

test_merge file
test "extraction" do
  extract file, 0 => tmp
  hash_tmp
end
