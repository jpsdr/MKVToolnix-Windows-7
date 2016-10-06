#!/usr/bin/ruby -w

# T_567propedit_muxing_writing_application
describe "mkvpropedit / modifying muxing & writing application"

test_merge "data/subtitles/srt/ven.srt", :keep_tmp => true
test "set and verify properties" do
  propedit tmp, "--edit info --set 'muxing-application=chunky bacon' --set writing-application="

  info = identify_json(tmp)["container"]["properties"]
  info["muxing_application"] + "/" + info["writing_application"] + "/" + hash_tmp
end
