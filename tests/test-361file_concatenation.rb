#!/usr/bin/ruby

# T_361file_concatenation
describe "mkvmerge / file concatenation, multi file I/O"

vob_files = (1..3).map { |i| "data/vob/VTS_01_#{i}.VOB" }

test "additional vob_files listed" do
  identify_json(vob_files[0])["container"]["properties"]["other_file"].map { |f| f.gsub(%r{.*/}, '') }.join('+')
end

# These two must be equal.
test_merge vob_files[0],                     :exit_code => 1
test_merge "'(' #{vob_files.join(' ')} ')'", :exit_code => 1

# These three must be equal.
test_merge "'=' #{vob_files[0]}"
test_merge "'=#{vob_files[0]}'"
test_merge "'(' #{vob_files[0]} ')'"

# Separate case.
test_merge "'(' #{vob_files[0]} #{vob_files[1]} ')'"

m2ts_files = (0..3).map { |i| "data/ts/0000#{i}.m2ts" }

test_identify m2ts_files[0], :verbose => true

# These four must be equal.
test_merge m2ts_files[0]
test_merge "'=' #{m2ts_files[0]}"
test_merge "'=#{m2ts_files[0]}'"
test_merge "'(' #{m2ts_files[0]} ')'"

# Two separate cases.
test_merge "'(' #{m2ts_files[0..1].join(' ')} ')'"
test_merge "'(' #{m2ts_files[0..2].join(' ')} ')'"

# These two must be equal.
test_merge "'(' #{m2ts_files.join(' ')} ')'"
test_merge "data/ts/hd_distributor_regency.m2ts"
