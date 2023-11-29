#!/usr/bin/ruby -w

# T_762dovi_combining_bl_and_el
describe "mkvmerge / combining Dolby Vision layers from two different tracks read from MPEG transport streams"

Dir.glob("data/h265/dolby-vision/two-tracks/*.m2ts").sort.each do |file|
  test_merge file, :args => "-A"
end
