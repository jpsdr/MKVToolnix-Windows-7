#!/usr/bin/ruby -w

# T_670h265_interlaced
describe "mkvmerge / HEVC/H.265 with interlaced fields, video height"

test "get dimensions" do
  (identify_json("data/h265/interlaced1.265")["tracks"] +
   identify_json("data/h265/interlaced2.ts")["tracks"]).
    select { |t| t["type"] == "video" }.
    map    { |t| t["properties"]["pixel_dimensions"].to_s }.
    join("+")
end
