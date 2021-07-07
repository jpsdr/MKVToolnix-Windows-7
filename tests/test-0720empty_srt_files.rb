#!/usr/bin/ruby -w

# T_720empty_srt_files
describe "mkvmerge / empty SRT files"

%w{empty empty-bom}.each do |file_name|
  test_merge "data/subtitles/srt/#{file_name}.srt"
end
