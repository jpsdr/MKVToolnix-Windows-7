#!/usr/bin/ruby -w

# T_491auto_additional_files_only_with_vts_prefix
describe "mkvmerge / only read additional MPEG files if the file has a VTS_ prefix"
test_merge "data/vob/video_1.mpg"
