#!/usr/bin/ruby -w

# T_758av1_dovi_obu
describe "mkvmerge / AV1 with Dolby Vision, read from OBU streams"

Dir.glob("data/av1/dovi/*.obu").sort.each do |file_name|
  test_merge file_name
end
