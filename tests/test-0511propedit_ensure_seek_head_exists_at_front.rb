#!/usr/bin/ruby -w

# T_511propedit_ensure_seek_head_exists_at_front
describe "mkvpropedit / ensuring that a seek head always exists at the front even if it's removed in between"

dir = "data/bugs/1513"
src = "#{dir}/src.mkv"

test src do
  sys "cp #{src} #{tmp}"
  propedit tmp, "--attachment-name 1234.jpg --replace-attachment 1:#{dir}/picture.jpg"
  hash_tmp
end
