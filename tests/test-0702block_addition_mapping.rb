#!/usr/bin/ruby -w

# T_702block_addition_mapping

path  = "data/mkv/block_addition_mapping"
files = %w{test01_mvcc test02_dv_fel}

describe "mkvmerge / block addition mappings"

files.each { |file_name| test_merge "#{path}/#{file_name}.mkv" }
