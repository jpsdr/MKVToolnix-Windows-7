#!/usr/bin/ruby -w

# T_556prores
describe "mkvmerge / Apple ProRes video"

files = %w{data/mp4/_CINEC_20140130154020_CINEC_ProRes422.mov data/mp4/Afterglow_ProRes_VIDEO_3.mov}

files.each do |file|
  test_identify file

  base = tmp
  test_merge file, :keep_tmp => true, :output => "#{base}-mkv"
  test_merge "#{base}-mkv"
end
