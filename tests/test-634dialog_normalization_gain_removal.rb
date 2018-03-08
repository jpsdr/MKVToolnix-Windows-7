#!/usr/bin/ruby -w

# T_634dialog_normalization_gain_removal
describe "mkvmerge / dialog normalization gain removal"

%w{ac3/dialog_normalization.ac3 dts/dialog_normalization.dts}.each do |file_name|
  test_merge "data/#{file_name}"
  test_merge "data/#{file_name}", :args => "--remove-dialog-normalization-gain 0", :output => "#{tmp}-1", :keep_tmp => true
  test_merge "#{tmp}-1",          :args => "--remove-dialog-normalization-gain 0", :output => "#{tmp}-2", :keep_tmp => true
  test_merge "#{tmp}-2",                                                           :output => "#{tmp}-3"
end
