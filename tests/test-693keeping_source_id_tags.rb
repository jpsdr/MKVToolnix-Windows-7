#!/usr/bin/ruby -w

# T_693keeping_source_id_tags
describe "mkvmerge / keeping existing SOURCE_ID tags"

test "keeping SOURCE_ID tags" do
  merge "data/mkv/source_id_tags.mkv", :keep_tmp => true

  tags, _ = sys("../src/mkvextract #{tmp} tags -", :no_result => true)

  tags.join('').md5
end
