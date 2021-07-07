#!/usr/bin/ruby -w

# T_621propedit_remove_date
describe "mkvpropedit / remove 'date' header field"

test_merge "data/subtitles/srt/ven.srt", :keep_tmp => true
test "remove date header field" do
  propedit tmp, "--edit info --delete date"

  info = identify_json(tmp)["container"]["properties"]
  info.key?("date_utc") || info.key?("date_local") ? "failed" : "ok"
end
