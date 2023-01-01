#!/usr/bin/ruby -w

src = "data/flv/issue_3466_hevc_aac.flv"

# T_750issue_3466_hevc_in_flv
describe "mkvmerge / issue 3466: HEVC in FLV"

test "identification" do
  track = identify_json(src)["tracks"][0]
  if (track["type"] == "video") && %r{hevc}i.match(track["codec"])
    "ok"
  else
    fail "identification failed"
  end
end

test_merge src, :args => "--no-audio"
