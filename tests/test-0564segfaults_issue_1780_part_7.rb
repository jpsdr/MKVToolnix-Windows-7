#!/usr/bin/ruby -w

# T_564egfaults_issue_1780_part_7
describe "mkvmerge / various test cases for segfaults collected in issue 1780 part 7"

dir = "data/segfaults-assertions/issue-1780"

# "HEVC parser: fix invalid memory access beyond the end of allocated space"
%w{
explorer:id:000355,sig:11,src:000738,op:havoc,rep:4
explorer:id:000497,sig:11,src:001249,op:flip1,pos:101
}.each do |file|
  test_merge "#{dir}/#{file}", :exit_code => :error
end
