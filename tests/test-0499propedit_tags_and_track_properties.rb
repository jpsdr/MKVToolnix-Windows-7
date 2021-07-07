#!/usr/bin/ruby -w

# T_499propedit_tags_and_track_properties
describe "mkvpropedit / changing track properties and tags for the same track simultaneously"

test "tags and props" do
  merge "data/avi/v.avi", :keep_tmp => true, :no_result => true
  propedit tmp, "--tags track:v1:data/text/tags-nested-simple.xml -e track:v1 --set display-width=1234"
  hash_tmp
end
