#!/usr/bin/ruby -w

# T_561segfaults_issue_1780_part_4
describe "mkvmerge / various test cases for segfaults collected in issue 1780 part 4"

dir = "data/segfaults-assertions/issue-1780"

# "AAC reader: keep reading until frame with sample rate > 0 is found"
test_merge "#{dir}/explorer:id:000019,sig:06,src:000000,op:flip2,pos:2", :exit_code => :error
