#!/usr/bin/ruby -w

skip_if %r{i[0-9]86|armhf}.match(RUBY_PLATFORM)

# T_562segfaults_issue_1780_part_5
describe "mkvmerge / various test cases for segfaults collected in issue 1780 part 5"

dir = "data/segfaults-assertions/issue-1780"

# "MP4 reader: properly catch invalid chunk size exception"
test_merge "#{dir}/explorer:id:000081,sig:06,src:000001,op:havoc,rep:128",   :exit_code => :error
test_merge "#{dir}/explorer:id:000233,sig:06,src:000005,op:havoc,rep:16",    :exit_code => :warning
test_merge "#{dir}/explorer:id:000241,sig:11,src:000012,op:flip1,pos:13674", :exit_code => :warning, :result_type => :exit_code_string
