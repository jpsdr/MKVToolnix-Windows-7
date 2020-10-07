#!/usr/bin/ruby -w
# T_671emojis
describe "mkvmerge / Emojis"

emojis = "😀😁😁😂😂😃😃😃😄😄😄😅😅😅😆😆😆😉😊😊😋😋😎😎😎😍😍😍😘😘😗😗😙😙😚😚😘😘😘😍😍😍"

test_merge "data/subtitles/srt/ven.srt", :args => "--title #{emojis}", :keep_tmp => true
test "identification" do
  identify_json(tmp)["container"]["properties"]["title"] == emojis
end
