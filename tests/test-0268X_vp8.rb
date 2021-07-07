#!/usr/bin/ruby -w

# T_268X_vp8
describe "mkvextract / VP8 from Matroska and WebM"

test "extraction" do
  extract "data/webm/live-stream.webm", 0 => tmp, :exit_code => :warning
  hash_tmp
end
