#!/usr/bin/ruby -w

# T_664generate_chapters_interval_first_video_timestamp_bigger_than_0
describe "mkvmerge / generating chapters in intervals with video tracks which start at timestamp > 0"
test_merge "data/mkv/minimum_video_timestamp_bigger_than_0.mkv", :args => "--generate-chapters interval:60s", :keep_tmp => true

test "start timestamps" do
  extract tmp, :mode => :chapters, :args => "#{tmp}-chapters"

  expected = ["00:00:00.000000000", "00:01:00.000000000", "00:02:00.000000000", "00:03:00.000000000"]
  document = REXML::Document.new(IO.read("#{tmp}-chapters"))
  actual   = REXML::XPath.match(document, "/Chapters/EditionEntry/ChapterAtom/ChapterTimeStart").map(&:text)

  expected == actual
end
