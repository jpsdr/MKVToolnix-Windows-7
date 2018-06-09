#!/usr/bin/ruby -w

# T_641keep_display_unit
describe "mkvmerge / keep display unit if set"

test_merge "data/mkv/display_unit_aspect_ratio.mkv", :keep_tmp => true
test "display unit case 1" do
  props = identify_json(tmp)["tracks"][0]["properties"]
  props["display_unit"].to_s + "+" + props["display_dimensions"]
end

test_merge "data/mkv/display_unit_aspect_ratio.mkv", :keep_tmp => true, :args => "--display-dimensions 0:123x456"
test "display unit case 2" do
  props = identify_json(tmp)["tracks"][0]["properties"]
  props["display_unit"].to_s + "+" + props["display_dimensions"]
end
