#!/usr/bin/ruby -w

# T_682invalid_memory_access_in_frame_assembly
describe "mkvmerge / RealMedia invalid memory access during frame assembly"
test_merge "data/rm/invalid_memory_access_in_frame_assembly.rm", :exit_code => :warning
