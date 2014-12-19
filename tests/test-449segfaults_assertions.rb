#!/usr/bin/ruby -w

# T_449segfaults_assertions
describe "mkvmerge & mkvinfo / segfaults and assertions"

dir = "data/segfaults-assertions"

test_merge "#{dir}/1089-1.mkv", :exit_code => :error
test_merge "#{dir}/1089-2.mkv", :exit_code => :error
test_merge "#{dir}/1089-3.mkv"

test_info "#{dir}/1089-1.mkv", :args => "-v -v"

# commented out because currently memory is allocated by not read to
# fully, therefore its content is still random. Has to be fixed in
# libmatroska/libebml.
#test_info "#{dir}/1089-2.mkv", :args => "-v -v"

test_info "#{dir}/1089-3.mkv", :args => "-v -v"
