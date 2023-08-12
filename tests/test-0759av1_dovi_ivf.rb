#!/usr/bin/ruby -w

# T_758av1_dovi_obu
describe "mkvmerge / AV1 with Dolby Vision, read from IVF streams"

Dir.glob("data/av1/dovi/*.ivf").sort.each do |file_name|
  test_merge file_name
end
