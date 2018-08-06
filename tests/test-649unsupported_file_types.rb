#!/usr/bin/ruby -w

# T_649unsupported_file_types
describe "mkvmerge / various file formats that are recognized but not supported"

[ 'asf-wmv/new_fileformat.asf',
  'asf-wmv/welcome3.wmv',
  'aac/aac_adif.aac',
  'subtitles/hddvd/1.sup',
].each do |file|
  test_merge_unsupported "data/#{file}"
end
