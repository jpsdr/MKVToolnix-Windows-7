#!/usr/bin/ruby -w

# T_576codec_delay_in_identification
describe "mkvmerge / codec delay in JSON identification results"

test_merge "data/aac/v.aac", :keep_tmp => true

test "set and verify properties" do
  propedit tmp, "--edit track:a1 --set codec-delay=123000456"

  identify_json(tmp)["tracks"][0]["properties"]["codec_delay"]
end
