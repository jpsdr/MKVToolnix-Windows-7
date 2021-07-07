#!/usr/bin/ruby -w

# T_653X_text_subtitles_without_duration
describe "mkvextract / text subtitle tracks without BlockDuration elements"

test "extraction" do
  extract "data/mkv/text-subtitles-without-duration.mkv", 2 => tmp
  hash_tmp
end
