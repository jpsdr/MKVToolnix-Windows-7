#!/usr/bin/ruby -w

# T_635propedit_update_doc_type_version
describe "mkvpropedit / update 'doc type (read) version' header fields appropriately"

test_merge "data/avi/v.avi", :args => "--no-audio --engage no_cue_duration,no_simpleblocks,no_cue_relative_position", :keep_tmp => true
test "modification" do
  propedit tmp, "--edit track:1 --set name=42"
  m1_version, m1_read_version = get_doc_type_versions tmp

  propedit tmp, "--edit track:1 --set min-luminance=42"
  m2_version, m2_read_version = get_doc_type_versions tmp

  "#{m1_version}+#{m1_read_version}-#{m2_version}+#{m2_read_version}"
end
