#!/usr/bin/ruby -w

# T_558segfaults_issue_1780_part_1
describe "mkvmerge / various test cases for segfaults collected in issue 1780 part 1"

dir = "data/segfaults-assertions/issue-1780"

# "mkvmerge: make global destruction phase more deterministic"
%w{
explorer:id:000000,sig:11,src:000000,op:flip1,pos:1
explorer:id:000001,sig:11,src:000000,op:flip1,pos:2
explorer:id:000608,sig:06,src:001761,op:int8,pos:135,val:-128
explorer:id:000738,sig:11,src:002531,op:int32,pos:126,val:be:+1
explorer:id:000740,sig:06,src:002531,op:ext_AO,pos:99
}.each do |file|
  test_merge "#{dir}/#{file}", :exit_code => :error
end
