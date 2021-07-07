#!/usr/bin/ruby -w

# T_560segfaults_issue_1780_part_3
describe "mkvmerge / various test cases for segfaults collected in issue 1780 part 3"

skip_if $is_windows

dir = "data/segfaults-assertions/issue-1780"

# "MP4 reader: fix access beyond end of vector"

exit_codes = {
  'explorer:id:000097,sig:06,src:000005,op:flip1,pos:13730'         => :warning,
  'explorer:id:000261,sig:11,src:000013,op:arith8,pos:13647,val:+7' => :warning,
  'explorer:id:000323,sig:06,src:000314,op:flip1,pos:13731'         => :warning,
}

%w{
explorer:id:000011,sig:11,src:000001,op:flip1,pos:42
explorer:id:000012,sig:11,src:000001,op:flip1,pos:13438
explorer:id:000014,sig:11,src:000001,op:flip1,pos:13457
explorer:id:000030,sig:11,src:000001,op:flip1,pos:13798
explorer:id:000031,sig:11,src:000001,op:flip1,pos:13798
explorer:id:000032,sig:11,src:000001,op:flip1,pos:13798
explorer:id:000063,sig:06,src:000001,op:arith8,pos:13794,val:-13
explorer:id:000083,sig:06,src:000005,op:flip1,pos:3
explorer:id:000097,sig:06,src:000005,op:flip1,pos:13730
explorer:id:000129,sig:06,src:000005,op:flip1,pos:13801
explorer:id:000144,sig:11,src:000005,op:flip2,pos:13866
explorer:id:000231,sig:11,src:000005,op:havoc,rep:16
explorer:id:000261,sig:11,src:000013,op:arith8,pos:13647,val:+7
explorer:id:000318,sig:11,src:000287,op:flip2,pos:13797
explorer:id:000324,sig:06,src:000314,op:havoc,rep:64
explorer:id:000347,sig:06,src:000426,op:flip1,pos:13797
explorer:id:000353,sig:11,src:000426,op:flip1,pos:13823
explorer:id:000354,sig:11,src:000426,op:flip1,pos:13825
explorer:id:000357,sig:11,src:000426,op:flip2,pos:13825
explorer:id:000358,sig:06,src:000426,op:flip2,pos:13826
}.each do |file|
  test_merge "#{dir}/#{file}", :exit_code => exit_codes[file] || :ok
end

# The following files cause uninitialized memory to be written and
# consecutive tests to fail:
%w{
explorer:id:000323,sig:06,src:000314,op:flip1,pos:13731
}.each do |file|
  test_merge "#{dir}/#{file}", :result_type => :exit_code_string, :exit_code => exit_codes[file] || :ok
end
