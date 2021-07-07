#!/usr/bin/ruby -w

# T_668flush_on_close
describe "mkvmerge, mkvextract / flushing on close"
test_merge "data/subtitles/srt/ven.srt", :args => "--flush-on-close"
test "extraction" do
  extract "data/mkv/complex.mkv", 8 => tmp, :args => "--flush-on-close"
  hash_tmp
end
