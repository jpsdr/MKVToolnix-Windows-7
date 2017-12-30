#!/usr/bin/ruby -w

# T_446mkvinfo_output
describe "mkvinfo / output of various modes"

%w{complex.mkv attachments.mkv subbed.mkv vobsubs.mks chapters-uk-and-gb.mks chapters-with-ebmlvoid.mkv}.each do |file|
  test_info "data/mkv/#{file}"
  test_info "data/mkv/#{file}", :args => "-v -v"
  test_info "data/mkv/#{file}", :args => "-v -v -z"
  test_info "data/mkv/#{file}", :args => "-s -t"
end
