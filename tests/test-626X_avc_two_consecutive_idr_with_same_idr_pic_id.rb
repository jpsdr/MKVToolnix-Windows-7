#!/usr/bin/ruby -w

# T_626X_avc_two_consecutive_idr_with_same_idr_pic_id
describe "mkvextract / AVC two consecutive IDR frames with same idr_pic_id"
test "extraction" do
  extract "data/mkv/avc_two_consecutive_idr_with_same_idr_pic_id.mkv", 0 => tmp
  hash_tmp
end
