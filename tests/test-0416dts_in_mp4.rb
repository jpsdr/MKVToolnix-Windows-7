#!/usr/bin/ruby -w

# T_416dts_in_mp4
describe "mkvmerge / reading DTS from MP4"

%w{480p-DTS-5.1 1080p-DTS-HD-7.1}.each do |file_name|
  test_merge "data/mp4/#{file_name}.mp4"
end

test_merge "data/mp4/1080p-DTS-HD-7.1.mp4", :args => "--track-enabled-flag 1:0 --track-enabled-flag 2:1 --track-enabled-flag 3:1", :keep_tmp => true
test "track enabled flags" do
  identify_json(tmp)["tracks"].map { |track| track["properties"]["enabled_track"].to_s }.join("+")
end
