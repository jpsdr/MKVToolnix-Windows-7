#!/usr/bin/ruby -w

# T_712info_use_after_free_issue2989
describe "mkvinfo / use after free issue 2989"

file = "data/mkv/sample-bug2989.mkv"

test_info file, :exit_code => :warning
test_identify file
