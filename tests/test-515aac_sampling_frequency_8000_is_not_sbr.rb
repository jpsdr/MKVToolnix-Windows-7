#!/usr/bin/ruby -w

# T_515aac_sampling_frequency_8000_is_not_sbr
describe "mkvmerge / AAC with 2 bytes codec-specific config, 8 KHz sampling frequency, is not SBR"
test_merge "data/mkv/bug1540-1.mkv + data/mkv/bug1540-2.mkv"
