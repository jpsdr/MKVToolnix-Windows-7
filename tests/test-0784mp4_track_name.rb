#!/usr/bin/ruby -w

# T_784mp4_track_name
describe "mkvmerge / issue 6274: reading track names from MP4 files"

files = [
  [ "data/mp4/480p-DTS-5.1.mp4", 1, "Stereo" ],
  [ "data/mp4/edit_list_constant_offset_segment_duration_not_0.mp4", 1, "Stereo" ],
  [ "data/mp4/1080p-DTS-HD-7.1.mp4", 1, "2.0 AAC" ],
]

files.each do |file|
  test_merge(file[0], :args => "-D -S -a #{file[1]}")

  test "identification" do
    fail if identify_json(file[0])["tracks"][0]["properties"]["track_name"] == file[2]

    "OK"
  end
end
