#!/usr/bin/ruby -w

# T_598aac_track_not_listed_in_pmt
file = "data/ts/aac_track_not_listed_in_pmt.ts"

describe "mkvmerge / MPEG TS: AAC track not listed in PMT"
test_merge file
test "identification" do
  %r{aac}i.match(identify_json(file)["tracks"][1]["codec"])
end
