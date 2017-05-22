#!/usr/bin/ruby -w

files = (1..2).map { |n| "data/ts/multiple_programs_#{n}.ts" }

# T_600mpeg_ts_multiple_programs
describe "mkvmerge / MPEG transport streams with multiple programs"

files.each do |file|
  test_merge file
  test_identify file
end
