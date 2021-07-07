#!/usr/bin/ruby -w

# T_449segfaults_assertions
describe "mkvmerge & mkvinfo / segfaults and assertions"

skip_if c?(:USE_ADDRSAN)
skip_if $is_windows

dir = "data/segfaults-assertions"

test_merge "#{dir}/1089-1.mkv", :exit_code => :error
test_merge "#{dir}/1089-2.mkv", :exit_code => :warning
test_merge "#{dir}/1089-3.mkv", :result_type => :exit_code

test_info "#{dir}/1089-1.mkv", :args => "-v -v", :exit_code => :error
test "1089-2.mkv" do
  out = info "#{dir}/1089-2.mkv", :args => "-v -v", :output => :return, :exit_code => :warning
  out.join("\n").gsub(/\(0x.*?\)/, 'xxx').md5
end
test_info "#{dir}/1089-3.mkv", :args => "-v -v"

test_merge "#{dir}/1096-id:000000,sig:06,src:000000,op:flip1,pos:0.mkv", :exit_code => :error
test_merge "#{dir}/1096-id:000001,sig:06,src:000000,op:flip1,pos:0.mkv", :exit_code => :error
test_merge "#{dir}/1096-id:000002,sig:06,src:000000,op:flip2,pos:582.mkv", :result_type => :exit_code
test_merge "#{dir}/1096-id:000003,sig:06,src:000000,op:flip2,pos:606.mkv"
test_merge "#{dir}/1096-id:000004,sig:06,src:000000,op:flip4,pos:582.mkv"

test_info "#{dir}/1096-id:000000,sig:06,src:000000,op:flip1,pos:0.mkv", :args => "-v -v", :exit_code => :error
test_info "#{dir}/1096-id:000001,sig:06,src:000000,op:flip1,pos:0.mkv", :args => "-v -v", :exit_code => :error
test_info "#{dir}/1096-id:000002,sig:06,src:000000,op:flip2,pos:582.mkv", :args => "-v -v"
test_info "#{dir}/1096-id:000003,sig:06,src:000000,op:flip2,pos:606.mkv", :args => "-v -v"
test_info "#{dir}/1096-id:000004,sig:06,src:000000,op:flip4,pos:582.mkv", :args => "-v -v"
