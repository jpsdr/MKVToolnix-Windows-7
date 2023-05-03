#!/usr/bin/ruby -w

# T_756cropping_in_json_output_if_values_0
describe "mkvmerge / cropping values should be reported in JSON identification mode even if some of their values are 0 (issue 3534)"

test_merge "data/avi/v.avi", :args => "-A --cropping 0:0,42,0,54", :keep_tmp => true

test "identification" do
  properties = identify_json(tmp)["tracks"][0]["properties"]

  properties.include?("cropping") && (properties["cropping"] == "0,42,0,54") ? "ok" : "bad"
end
