#!/usr/bin/ruby -w

# T_679wavpack_from_matroska
describe "mkvmerge / reading WavPack from Matroska"

test_merge "data/wavpack4/v.wv", :output => "#{tmp}-1", :keep_tmp => true
test_merge "#{tmp}-1",           :output => "#{tmp}-2"
