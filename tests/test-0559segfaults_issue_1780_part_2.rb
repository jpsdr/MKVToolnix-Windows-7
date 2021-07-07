#!/usr/bin/ruby -w

# T_559segfaults_issue_1780_part_2
describe "mkvmerge / various test cases for segfaults collected in issue 1780 part 2"

dir = "data/segfaults-assertions/issue-1780"

# "HEVC parser: fix invalid memory access beyond the end of allocated space"
%w{
explorer:id:000494,sig:11,src:001249,op:flip1,pos:63
explorer:id:000496,sig:06,src:001249,op:flip1,pos:92
explorer:id:000502,sig:06,src:001249,op:int8,pos:100,val:+32
explorer:id:000605,sig:11,src:001741,op:int32,pos:29,val:+0
explorer:id:000676,sig:06,src:002253,op:ext_AO,pos:101
explorer:id:000784,sig:11,src:002818,op:ext_AO,pos:103
explorer:id:000830,sig:11,src:003020,op:flip1,pos:103
explorer:id:000831,sig:11,src:003020,op:flip1,pos:104
explorer:id:000834,sig:11,src:003020,op:havoc,rep:2
explorer:id:000882,sig:11,src:003246,op:flip1,pos:123
explorer:id:000884,sig:11,src:003246,op:int8,pos:121,val:-128
explorer:id:000886,sig:06,src:003248,op:flip1,pos:106
explorer:id:000935,sig:11,src:003528,op:flip4,pos:130
explorer:id:000936,sig:11,src:003528,op:flip32,pos:127
explorer:id:000937,sig:11,src:003528,op:arith8,pos:130,val:+5
explorer:id:000938,sig:11,src:003528,op:int32,pos:127,val:+100
explorer:id:000939,sig:11,src:003528,op:int32,pos:128,val:+1
explorer:id:000974,sig:11,src:003742,op:flip1,pos:123
explorer:id:000975,sig:11,src:003746,op:flip1,pos:130
explorer:id:000976,sig:11,src:003746,op:flip1,pos:130
explorer:id:000977,sig:11,src:003746,op:flip1,pos:133
explorer:id:000978,sig:11,src:003746,op:flip1,pos:133
explorer:id:000979,sig:11,src:003746,op:flip2,pos:134
explorer:id:000980,sig:11,src:003746,op:arith8,pos:133,val:-3
explorer:id:001003,sig:11,src:003976,op:flip1,pos:127
explorer:id:001019,sig:11,src:004180,op:flip1,pos:9
explorer:id:001020,sig:11,src:004180,op:int32,pos:143,val:be:+1
explorer:id:001021,sig:11,src:004180,op:havoc,rep:2
}.each do |file|
  test_merge "#{dir}/#{file}", :exit_code => :error
end
