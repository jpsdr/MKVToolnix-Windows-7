#!/usr/bin/ruby -w

# T_748doc_type_version_handler_create_void_in_ebml_head
describe "mkvpropedit / doc type version handler should create void inside EBLM Head, not on top level"

work = "#{tmp}.work"

test "propedit" do
  cp "data/mkv/bug3355.mkv", work
  propedit work, "--tags track:1:data/tags/bug3355.xml --edit info --set title=Test"

  hash_file work
end

test_identify work
