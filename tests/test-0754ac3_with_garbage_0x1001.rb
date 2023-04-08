#!/usr/bin/ruby -w

file = "data/ac3/issue_3484_garbage_0x0110.eb3"

# T_754ac3_with_garbage_0x1001
describe "mkvmerge / AC-3 with 16 bytes of garbage starting with 0x01 10 before sync frames"

test_identify file
test_merge file
