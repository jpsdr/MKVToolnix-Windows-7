#!/usr/bin/ruby -w

# T_717bluray_identification
describe "mkvmerge / Blu-ray directory structure identification"

file = "data/bluray/ganster_squad/BDMV/PLAYLIST/00100.mpls"

test "identification" do
  json = identify_json(file)
  json["container"]["properties"]["playlist_file"] = json["container"]["properties"]["playlist_file"].map { |pl_file| pl_file.gsub(%r{\\}, '/').gsub(%r{.*/data/}, 'data/') }
  json.delete("identification_format_version")
  json.to_json.md5
end

test_merge file
