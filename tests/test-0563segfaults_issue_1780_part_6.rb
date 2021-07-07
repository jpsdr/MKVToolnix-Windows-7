#!/usr/bin/ruby -w

# T_563segfaults_issue_1780_part_6
describe "mkvmerge / various test cases for segfaults collected in issue 1780 part 6"

dir = "data/segfaults-assertions/issue-1780"

# "HEVC parser: prevent access to beyond the end of fixed-size arrays"
%w{
explorer:id:000110,sig:11,src:000003,op:flip1,pos:0
explorer:id:000699,sig:11,src:002364,op:flip1,pos:138
}.each do |file|
  test_merge "#{dir}/#{file}", :exit_code => :error
end
