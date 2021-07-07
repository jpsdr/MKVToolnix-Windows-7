#!/usr/bin/ruby -w

# T_510propedit_add_attachments_without_meta_seek_present
describe "mkvpropedit / adding attachments to a file that doesn't contain a meta seek element"

src = "data/mkv/x264.mkv"

test src do
  sys "cp #{src} #{tmp}"
  propedit tmp, "--add-attachment 'data/text/chap1.txt'"
  hash_tmp
end
