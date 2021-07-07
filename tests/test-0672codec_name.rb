#!/usr/bin/ruby -w

# T_672codec_name
describe "mkvmerge / codec name track property"

test_merge "data/mkv/codec_name.mkv", :keep_tmp => true
test "code name is kept" do
  identify_json(tmp)["tracks"][0]["properties"]["codec_name"] == "not a real codec"
end
