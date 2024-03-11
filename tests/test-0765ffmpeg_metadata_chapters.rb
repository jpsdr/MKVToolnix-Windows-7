#!/usr/bin/ruby -w

# T_765ffmpeg_metadata_chapters
describe "mkvmerge / reading chapters from ffmpeg metadata files"

Dir.glob("data/chapters/ffmpeg-metadata/*.meta").sort.each do |file_name|
  test_merge "data/subtitles/srt/ven.srt", :args => "--chapters #{file_name}"
end
