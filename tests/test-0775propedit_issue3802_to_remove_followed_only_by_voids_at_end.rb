#!/usr/bin/ruby -w

# T_775propedit_issue3802_to_remove_followed_only_by_voids_at_end
describe "mkvpropedit / issue 3802: failing to replace element at end with only void elements following"

test "replace existing chapters in chapters-only file" do
  cp "data/mkv/issue3802.mkv", tmp
  propedit tmp, "--chapters data/chapters/issue3802.xml"
  hash_tmp
end
